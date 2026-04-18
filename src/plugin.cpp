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
            if (Settings::BlockActionID != -1) {
                // Define os estados válidos para a Main Key (2 = Hold, 4 = Press)
                int validMain[] = { 2, 4 };
                InputManagerAPI::_API->UpdateListener(
                    0,                          // Tipo de Input (0 = Action)
                    Settings::BlockActionID,    // ID da Ação
                    "Just a Block",             // Nome do Mod
                    "Block",                    // Propósito
                    true,                       // isRegistering
                    validMain,                  // Array com os estados permitidos para a Main Key
                    2,                          // Quantidade de estados no array validMain
                    nullptr,                    // Array de estados permitidos para o Modificador (nulo = livre)
                    0                           // Quantidade de estados no array do Modificador
                );
            }
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
