#include "logger.h"
#include "Hooks.h"
#include "settings.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    if (message->type == InputManagerAPI::kMessage_ProvideAPI) {
        InputManagerAPI::ReceiveAPI(message);
        logger::info("API do Input Manager recebida com sucesso!");
    }
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
		BlockModMenu::Register();
        InputManagerAPI::RequestAPIDirect();

        if (InputManagerAPI::_API) {
            logger::info("API do Input Manager conectada");
        }
        Idles::InitIdles();
        Block_InputManagerListener::GetSingleton()->Register();
        AttackStateManager::GetSingleton()->Register();
        if (InputManagerAPI::_API) {
            if (Settings::BlockActionID != -1) InputManagerAPI::_API->UpdateListener(0,Settings::BlockActionID, "Just a Block", "Block", true);
        }
    }
    if (message->type == SKSE::MessagingInterface::kNewGame || message->type == SKSE::MessagingInterface::kPostLoadGame) {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            player->AddAnimationGraphEventSink(PlayerAnimGraphListener::GetSingleton());
            logger::info("AnimGraphListener registrado no Player.");
        }
        if (Settings::DisableBlockLeft) {
			player->SetGraphVariableInt("IsBlockingCMF", 0);
        }
		player->SetGraphVariableBool("IsMagicBlockingCMF", false);
		g_savedRightHandSpell = nullptr;
		g_savedLeftHandSpell = nullptr;
        GetAttackKeys();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {

    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
