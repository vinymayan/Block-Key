#include "hooks.h"
#include "Settings.h"

void GetAttackKeys()
{
	logger::info("Getting attack keys");
	auto controlMap = RE::ControlMap::GetSingleton();
	auto userEvents = RE::UserEvents::GetSingleton();
	if (controlMap && userEvents) {
		LeftKeyboard = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kKeyboard);
		LeftMouse = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kMouse);
		LeftMouse += 256;
		LeftGamepad = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kGamepad);
	}
	logger::info("Left Attack Keys: Keyboard: {}, Mouse: {}, Gamepad: {}", LeftKeyboard, LeftMouse, LeftGamepad);
}

bool HasMagicEquipped(RE::Actor* actor) {
	if (!actor) return false;
	auto leftHand = actor->GetEquippedObject(true);
	auto rightHand = actor->GetEquippedObject(false);

	bool rightIsMagic = rightHand && rightHand->GetFormType() == RE::FormType::Spell;
	bool leftIsShield = leftHand && leftHand->GetFormType() == RE::FormType::Armor;

	return rightIsMagic && !leftIsShield;
}

namespace Idles {

	inline RE::TESIdleForm* GetIdleByFormID(RE::FormID formID, const std::string& pluginName) {
		if (auto dataHandler = RE::TESDataHandler::GetSingleton()) {
			auto form = dataHandler->LookupForm(formID, pluginName);
			if (form) return form->As<RE::TESIdleForm>();
		}
		return nullptr;
	}

	void InitIdles() {
		const std::string skyrim = "Skyrim.esm";
		BlockStart = GetIdleByFormID(0x13217, skyrim);
		BlockStop = GetIdleByFormID(0x13ACA, skyrim); 
		RightSlot = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F42);
		LeftSlot = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x13F43);
	}

	inline void PlayIdleAnimation(RE::Actor* actor, RE::TESIdleForm* idle) {
		if (actor && idle) {
			if (auto* processManager = actor->GetActorRuntimeData().currentProcess) {
				processManager->PlayIdle(actor, idle, actor);
			}
			else {
				SKSE::log::error("Não foi possível obter o AIProcess (currentProcess) do ator.");
			}
		}
	}

}

inline std::array blockedMenus = {
	RE::DialogueMenu::MENU_NAME,    RE::JournalMenu::MENU_NAME,    RE::MapMenu::MENU_NAME,
	RE::StatsMenu::MENU_NAME,       RE::ContainerMenu::MENU_NAME,  RE::InventoryMenu::MENU_NAME,
	RE::TweenMenu::MENU_NAME,       RE::TrainingMenu::MENU_NAME,   RE::TutorialMenu::MENU_NAME,
	RE::LockpickingMenu::MENU_NAME, RE::SleepWaitMenu::MENU_NAME,  RE::LevelUpMenu::MENU_NAME,
	RE::Console::MENU_NAME,         RE::BookMenu::MENU_NAME,       RE::CreditsMenu::MENU_NAME,
	RE::LoadingMenu::MENU_NAME,     RE::MessageBoxMenu::MENU_NAME, RE::MainMenu::MENU_NAME,
	RE::RaceSexMenu::MENU_NAME,     RE::FavoritesMenu::MENU_NAME
	//,  std::string_view("LootMenu"),std::string_view("LootMenuIE") 
};

bool IsAnyMenuOpen() {
	const auto ui = RE::UI::GetSingleton();
	for (const auto a_name : blockedMenus) {
		if (ui->IsMenuOpen(a_name)) {
			return true;
		}
	}
	return false;
}

void StartCustomBlock(RE::Actor* actor) {
	if (!actor) return;
	actor->SetGraphVariableBool("isBlockingCMF", true);
	actor->NotifyAnimationGraph("VVV_BlockStart");
}

void StartBlocking(RE::PlayerCharacter* player) {
	if (!player) return;

	if (Settings::CancelDodgeWithBlock) {
		player->NotifyAnimationGraph("BF_DodgeStop");
	}

	Settings::_isCurrentlyBlocking = true;

	StartCustomBlock(player);
}

void StopBlocking(RE::PlayerCharacter* player) {
	if (!player) return;

	Settings::_isCurrentlyBlocking = false;
	player->SetGraphVariableInt("IsBlockingCMF", 0);
	player->SetGraphVariableBool("IsMagicBlockingCMF", false);
	if (auto actorState = player->AsActorState()) {
		actorState->actorState2.wantBlocking = false;
	}

	if (Idles::BlockStop && Idles::BlockStop->conditions.IsTrue(player, player)) {
		Idles::PlayIdleAnimation(player, Idles::BlockStop);
	}
	//player->NotifyAnimationGraph("VVV_BlockStop");
}

RE::BSEventNotifyControl Block_InputManagerListener::ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*)
{
	if (!a_event) return RE::BSEventNotifyControl::kContinue;

	std::string_view eventName = a_event->eventName.c_str();
	int actionID = static_cast<int>(a_event->numArg);

	if (eventName == "TweenPauseReady") {
		InputManagerAPI::RequestAPIDirect();
		if (InputManagerAPI::_API) {
			BlockModMenu::RegisterAllInputs();
			BlockModMenu::TweenPauseRegister();
			SKSE::log::info("TweenPauseReady recebido; controles enviados ao Tween Pause.");
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	// 1. ATUALIZAÇÃO DINÂMICA VIA TWEENPAUSE (INTERFACE)
	if (eventName == "TweenPause_ControlUpdated") {
		rapidjson::Document doc;
		doc.Parse(a_event->strArg.c_str());

		if (!doc.HasParseError() && doc.IsObject()) {
			std::string actionId = doc["actionId"].GetString();

			if (actionId == "Block") {
				BlockModMenu::UnregisterInputCategory(actionId);

				std::vector<int> newActions;
				std::vector<int> newMotions;

				if (doc.HasMember("mappedIds") && doc["mappedIds"].IsArray()) {
					for (auto& bind : doc["mappedIds"].GetArray()) {
						if (bind.HasMember("actionID") && bind["actionID"].IsInt()) newActions.push_back(bind["actionID"].GetInt());
						if (bind.HasMember("motionID") && bind["motionID"].IsInt()) newMotions.push_back(bind["motionID"].GetInt());
					}
				}

				Settings::BlockActionIDs = newActions;
				Settings::BlockMotionIDs = newMotions;

				BlockModMenu::RegisterAllInputs();
				BlockModMenu::SaveSettings();
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	// Validações de Estado do Player
	auto player = RE::PlayerCharacter::GetSingleton();
	if (!player || !player->Is3DLoaded() || (IsAnyMenuOpen() && player->IsStaggering())) {
		return RE::BSEventNotifyControl::kContinue;
	}

	const auto playerState = player->AsActorState();
	if (!(!player->IsInKillMove() && playerState->GetWeaponState() == RE::WEAPON_STATE::kDrawn &&
		playerState->GetSitSleepState() == RE::SIT_SLEEP_STATE::kNormal &&
		playerState->GetFlyState() == RE::FLY_STATE::kNone)) {
		return RE::BSEventNotifyControl::kContinue;
	}

	// Lambda para verificar a existência do ID nos novos vetores
	auto HasInput = [](const std::vector<int>& list, int id) {
		return std::find(list.begin(), list.end(), id) != list.end();
		};

	// 2. LÓGICA DE PROCESSAMENTO DOS INPUTS (ACTIONS / MOTIONS)
	if (eventName == "InputManager_ActionTriggered") {
		if (HasInput(Settings::BlockActionIDs, actionID)) {
			// Se já estiver bloqueando, o novo comando desliga o bloco (comportamento Toggle para Tap)
			if (Settings::_isCurrentlyBlocking) {
				StopBlocking(player);
			}
			else {
				StartBlocking(player);
			}
		}
	}
	else if (eventName == "InputManager_MotionTriggered") {
		if (HasInput(Settings::BlockMotionIDs, actionID)) {
			// Comportamento Toggle também aplicado a movimentos/analógicos detectados
			if (Settings::_isCurrentlyBlocking) {
				StopBlocking(player);
			}
			else {
				StartBlocking(player);
			}
		}
	}
	else if (eventName == "InputManager_ActionReleased") {
		if (HasInput(Settings::BlockActionIDs, actionID)) {
			if (Settings::_isCurrentlyBlocking) {
				StopBlocking(player);
			}
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl PlayerAnimGraphListener::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
{
	if (!a_event || !a_event->holder) {
		return RE::BSEventNotifyControl::kContinue;
	}
	auto player = RE::PlayerCharacter::GetSingleton();
	if (a_event->holder == player) {
		const auto playerState = player->AsActorState();
		if (!(!player->IsInKillMove() && playerState->GetWeaponState() == RE::WEAPON_STATE::kDrawn &&
			playerState->GetSitSleepState() == RE::SIT_SLEEP_STATE::kNormal &&
			playerState->GetFlyState() == RE::FLY_STATE::kNone)) {
			return RE::BSEventNotifyControl::kContinue;
		}
		std::string_view eventName = a_event->tag.c_str();

		if (eventName == "bashStop" || eventName == "attackStop" || eventName == "staggerStop") {
			if (Settings::_isCurrentlyBlocking) {
				StartCustomBlock(player);
			}
		}
		else if (eventName == "blockStart") {
			Settings::_isCurrentlyBlocking = true;
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}

void AttackStateManager::Register() {
	auto input = RE::BSInputDeviceManager::GetSingleton();
	if (input) {
		input->AddEventSink(this);
		SKSE::log::info("SUCESSO: Listener de eventos de input registrado.");
	}
	else {
		SKSE::log::error(
			"FALHA: O gerenciador de input (BSInputDeviceManager) é nulo. O listener não pôde ser registrado.");
	}
}

RE::BSEventNotifyControl AttackStateManager::ProcessEvent(RE::InputEvent* const* a_event,
	RE::BSTEventSource<RE::InputEvent*>* a_source) {
	if (!a_event || !*a_event) {
		return RE::BSEventNotifyControl::kContinue;
	}

	auto player = RE::PlayerCharacter::GetSingleton();
	if (!player || !player->Is3DLoaded()) {
		return RE::BSEventNotifyControl::kContinue;
	}

	for (auto event = *a_event; event; event = event->next) {
		if (event->eventType != RE::INPUT_EVENT_TYPE::kButton) {
			continue;
		}

		auto buttonEvent = event->AsButtonEvent();
		if (!buttonEvent) {
			continue;
		}

		if (IsAnyMenuOpen()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto device = buttonEvent->GetDevice();
		auto rawKeyCode = buttonEvent->GetIDCode();
		auto keyCode = rawKeyCode;
		const auto playerState = player->AsActorState();


		if (device == RE::INPUT_DEVICE::kMouse) {
			keyCode += 256;
		}
		
		if (buttonEvent->IsDown()) {
			if (Settings::DisableBlockLeft) {
				if (keyCode == LeftKeyboard || keyCode == LeftMouse || keyCode == LeftGamepad) {
					auto rightHandObj = player->GetEquippedObject(false);
					auto leftHandObj = player->GetEquippedObject(true);

					// Verifica se a mão direita tem qualquer arma ou está vazia
					bool rightIsValid = !rightHandObj || rightHandObj->GetFormType() == RE::FormType::Weapon;

					// Verifica se a mão esquerda tem um escudo (Armor) ou está vazia
					bool leftIsValid = !leftHandObj || leftHandObj->GetFormType() == RE::FormType::Armor;

					if (rightHandObj && rightHandObj->GetFormType() == RE::FormType::Weapon) {
						auto weapon = rightHandObj->As<RE::TESObjectWEAP>();
						if (weapon) {
							auto weapType = weapon->GetWeaponType();
							if (weapType == RE::WEAPON_TYPE::kTwoHandSword ||
								weapType == RE::WEAPON_TYPE::kTwoHandAxe ||
								weapType == RE::WEAPON_TYPE::kBow ||
								weapType == RE::WEAPON_TYPE::kCrossbow) {
								leftIsValid = true;
							}
						}
					}

					// Se ambas as condições forem verdadeiras, aplica a variável (Cobre 1H+Escudo, 2H e Desarmado)
					if (rightIsValid && leftIsValid) {
						player->SetGraphVariableInt("IsBlockingCMF", 0);
					}
					else {
						player->SetGraphVariableInt("IsBlockingCMF", 1);
					}
				}
			}
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}
