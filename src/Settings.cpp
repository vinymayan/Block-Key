#include "Settings.h"
#include <string>
#include <vector>

namespace ImGui = ImGuiMCP;

namespace BlockModMenu {

    const char* SETTINGS_PATH = "Data/SKSE/Plugins/JusBlock_Settings.json";


    inline std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    inline int GetIndexFromID(int id, const int* idArray, int arraySize) {
        for (int i = 0; i < arraySize; i++) {
            if (idArray[i] == id) return i;
        }
        return 0;
    }

    inline bool SearchableCombo(const char* label, int* current_item, const char* const items[], int items_count) {
        bool changed = false;
        const char* preview_value = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : "None";

        if (ImGuiMCP::BeginCombo(label, preview_value)) {
            static char searchBuf[128] = "";

            // Quando a lista abre, limpa a caixa de texto e foca nela
            if (ImGuiMCP::IsWindowAppearing()) {
                searchBuf[0] = '\0';
                ImGuiMCP::SetKeyboardFocusHere();
            }

            ImGuiMCP::InputText("Filter...##Search", searchBuf, sizeof(searchBuf));
            ImGuiMCP::Separator();

            std::string searchLower = ToLower(searchBuf);

            // Popula os Selectables filtrando a array base
            for (int i = 0; i < items_count; i++) {
                if (searchLower.empty() || ToLower(items[i]).find(searchLower) != std::string::npos) {
                    bool is_selected = (*current_item == i);
                    if (ImGuiMCP::Selectable(items[i], is_selected)) {
                        *current_item = i;
                        changed = true;
                    }
                    // Desce a barra de rolagem até o item selecionado ao abrir
                    if (is_selected && ImGuiMCP::IsWindowAppearing()) {
                        ImGuiMCP::SetScrollHereY();
                    }
                }
            }
            ImGuiMCP::EndCombo();
        }
        return changed;
    }

    // Variáveis de Estado Temporário de Edição
    static int current_edit_action_id = -1;
    static InputManagerAPI::ActionInfo edit_info;
    static char edit_nameBuf[64] = "";
    // PC
    static int ui_pcMainIdx = 0;
    static int ui_pcModIdx = 0;
    static int current_pcModAct = 0;
    // Gamepad
    static int ui_padMainIdx = 0;
    static int ui_padModIdx = 0;
    static int current_padModAct = 0;
    // Feedback Visual
    static std::string updateStatusMsg = "";
    static bool updateSuccess = false;

    static int ui_selectedActionID = -2;

    void SaveSettings() {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        doc.AddMember("BlockActionID", Settings::BlockActionID, allocator);
        doc.AddMember("EnableMagicBlock", Settings::EnableMagicBlock, allocator);
        doc.AddMember("DisableBlockLeft", Settings::DisableBlockLeft, allocator);
        FILE* fp = nullptr;
        fopen_s(&fp, SETTINGS_PATH, "wb");
        if (fp) {
            char writeBuffer[65536];
            rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
            rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
            doc.Accept(writer);
            fclose(fp);
        }
    }

    void LoadSettings() {
        FILE* fp = nullptr;
        fopen_s(&fp, SETTINGS_PATH, "rb");
        if (fp) {
            char readBuffer[65536];
            rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
            rapidjson::Document doc;
            doc.ParseStream(is);
            fclose(fp);

            if (doc.IsObject()) {
                if (doc.HasMember("BlockActionID")) Settings::BlockActionID = doc["BlockActionID"].GetInt();
                if (doc.HasMember("EnableMagicBlock")) Settings::EnableMagicBlock = doc["EnableMagicBlock"].GetBool();
                if (doc.HasMember("DisableBlockLeft")) Settings::DisableBlockLeft = doc["DisableBlockLeft"].GetBool();
            }
        }
    }

    void __stdcall Render() {
        // Inicializa a UI com o ActionID salvo
        if (ui_selectedActionID == -2) {
            ui_selectedActionID = Settings::BlockActionID;
        }

        bool settings_changed = false;

        ImGui::Text("Block Settings");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGuiMCP::Checkbox("Enable Magic Block", &Settings::EnableMagicBlock)) {
            settings_changed = true;
        }
        if (ImGuiMCP::Checkbox("Disable Block on Left Hand", &Settings::DisableBlockLeft)) {
            settings_changed = true;
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Block Action (Input Manager):");
        if (!InputManagerAPI::_API) {
            ImGui::TextDisabled("[Input Manager not detected in current session]");
        }
        else {
            size_t actionCount = InputManagerAPI::_API->GetInputCount(0);
            std::string previewValue = "[No Action Selected]";
            bool isValidHold = true;

            // Utiliza ui_selectedActionID para o preview ao invés de Settings::BlockActionID
            if (ui_selectedActionID >= 0 && ui_selectedActionID < actionCount) {
                auto info = InputManagerAPI::_API->GetActionInfo(ui_selectedActionID);
                previewValue = "[" + std::to_string(ui_selectedActionID) + "] " + (info.name ? std::string(info.name) : "Unnamed");

                bool isPcValid = (info.pcMainAction == 2 || info.pcMainAction == 4);
                bool isPadValid = (info.gamepadMainAction == 2 || info.gamepadMainAction == 4);
                if (!isPcValid && !isPadValid) {
                    isValidHold = false;
                }
            }

            ImGui::SetNextItemWidth(250);
            if (ImGui::BeginCombo("##ActionSelector", previewValue.c_str())) {
                if (ImGui::Selectable("[Disabled]", ui_selectedActionID == -1)) {
                    ui_selectedActionID = -1;
                    Settings::BlockActionID = -1;
                    settings_changed = true;
                }

                for (int i = 0; i < actionCount; ++i) {
                    auto info = InputManagerAPI::_API->GetActionInfo(i);
                    std::string itemLabel = "[" + std::to_string(i) + "] " + (info.name ? std::string(info.name) : "Unnamed");

                    if (ImGui::Selectable(itemLabel.c_str(), ui_selectedActionID == i)) {
                        ui_selectedActionID = i;
                        current_edit_action_id = -1; // Reseta a tela de edição para o novo input

                        // Só associa e salva no settings se o input selecionado JÁ for do tipo Hold
                        if (info.pcMainAction == 2 || info.gamepadMainAction == 2) {
                            Settings::BlockActionID = i;
                            settings_changed = true;
                        }
                    }
                }
                ImGui::EndCombo();
            }

            // O aviso foi alterado para deixar claro que a alteração foi pausada
            if (!isValidHold && ui_selectedActionID != -1) {
                ImGui::TextColored(ImGui::ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                    "Warning: Block requires a HOLD or PRESS action! The selected input will not be saved until updated.");
            }

            // Utiliza ui_selectedActionID daqui pra baixo para exibir a edição
            if (ui_selectedActionID != -1) {
                ImGuiMCP::Spacing();

                if (ImGuiMCP::TreeNode("Edit Keys and Gestures for Selected Action")) {

                    if (current_edit_action_id != ui_selectedActionID) {
                        edit_info = InputManagerAPI::_API->GetActionInfo(ui_selectedActionID);
                        current_edit_action_id = ui_selectedActionID;

                        strncpy_s(edit_nameBuf, edit_info.name ? edit_info.name : "Unnamed Action", sizeof(edit_nameBuf) - 1);
                        edit_nameBuf[sizeof(edit_nameBuf) - 1] = '\0';

                        ui_pcMainIdx = GetIndexFromID(edit_info.pcMainKey, pcKeyIDs, std::size(pcKeyIDs));
                        current_pcModAct = edit_info.pcModAction;
                        ui_pcModIdx = GetIndexFromID(edit_info.pcModifierKey, pcKeyIDs, std::size(pcKeyIDs));

                        ui_padMainIdx = GetIndexFromID(edit_info.gamepadMainKey, gamepadKeyIDs, std::size(gamepadKeyIDs));
                        current_padModAct = edit_info.gamepadModAction;
                        ui_padModIdx = GetIndexFromID(edit_info.gamepadModifierKey, gamepadKeyIDs, std::size(gamepadKeyIDs)); 

                        updateStatusMsg = "";
                    }

                    ImGuiMCP::InputText("Input Name", edit_nameBuf, sizeof(edit_nameBuf));
                    ImGuiMCP::Spacing(); ImGuiMCP::Separator(); ImGuiMCP::Spacing();

                    // --- TECLADO E MOUSE ---
                    ImGuiMCP::TextColored({ 0.4f, 0.8f, 1.0f, 1.0f }, "Keyboard and Mouse");

                    if (SearchableCombo("PC Main Key", &ui_pcMainIdx, pcKeyNames, std::size(pcKeyNames))) {
                        edit_info.pcMainKey = pcKeyIDs[ui_pcMainIdx];
                    }

                    if (ImGuiMCP::Combo("PC Main Action", &edit_info.pcMainAction, actionStateNames, std::size(actionStateNames))) {
                        if (edit_info.pcMainAction != 2 && edit_info.pcMainAction != 4) {
                            edit_info.pcMainAction = 0;
                        }
                    }

                    if (ImGuiMCP::Combo("PC Mod Action", &current_pcModAct, actionStateNames, std::size(actionStateNames))) {
                        if (current_pcModAct == 3) current_pcModAct = 0;
                    }

                    if (current_pcModAct != edit_info.pcModAction) {
                        if (current_pcModAct != 0) {
                            ui_pcModIdx = 0;
                            edit_info.pcModifierKey = pcKeyIDs[ui_pcModIdx];
                        }
                        else {
                            edit_info.pcModifierKey = 0;
                        }
                        edit_info.pcModAction = current_pcModAct;
                    }

                    if (edit_info.pcModAction != 0) {
                        if (SearchableCombo("PC Mod Key", &ui_pcModIdx, pcKeyNames, std::size(pcKeyNames))) {
                            edit_info.pcModifierKey = pcKeyIDs[ui_pcModIdx];
                        }

                        if (edit_info.pcModAction == 1) { 
                            if (edit_info.pcModTapCount < 1) edit_info.pcModTapCount = 1;
                            ImGuiMCP::SliderInt("PC Mod Tap Amount", &edit_info.pcModTapCount, 1, 5);
                        }
                    }

                    ImGuiMCP::Spacing();

                    // --- GAMEPAD ---
                    ImGuiMCP::Separator();
                    ImGuiMCP::TextColored({ 0.4f, 0.8f, 1.0f, 1.0f }, "Gamepad / Controller");

                    if (SearchableCombo("Pad Main Key", &ui_padMainIdx, gamepadKeyNames, std::size(gamepadKeyNames))) {
                        edit_info.gamepadMainKey = gamepadKeyIDs[ui_padMainIdx];
                    }

                    if (ImGuiMCP::Combo("Pad Main Action", &edit_info.gamepadMainAction, actionStateNames, std::size(actionStateNames))) {
                        if (edit_info.gamepadMainAction != 2 && edit_info.gamepadMainAction != 4) {
                            edit_info.gamepadMainAction = 0;
                        }
                    }

                    if (ImGuiMCP::Combo("Pad Mod Action", &current_padModAct, actionStateNames, std::size(actionStateNames))) {
                        if (current_padModAct == 3) current_padModAct = 0;
                    }

                    if (current_padModAct != edit_info.gamepadModAction) {
                        if (current_padModAct != 0) {
                            ui_padModIdx = 0;
                            edit_info.gamepadModifierKey = gamepadKeyIDs[ui_padModIdx];
                        }
                        else {
                            edit_info.gamepadModifierKey = 0;
                        }
                        edit_info.gamepadModAction = current_padModAct;
                    }

                    if (edit_info.gamepadModAction != 0) {
                        if (SearchableCombo("Pad Mod Key", &ui_padModIdx, gamepadKeyNames, std::size(gamepadKeyNames))) {
                            edit_info.gamepadModifierKey = gamepadKeyIDs[ui_padModIdx];
                        }

                        if (edit_info.gamepadModAction == 1) { // 1 = Tap
                            if (edit_info.gamepadModTapCount < 1) edit_info.gamepadModTapCount = 1;
                            ImGuiMCP::SliderInt("Pad Mod Tap Amount", &edit_info.gamepadModTapCount, 1, 5);
                        }
                    }

                    ImGuiMCP::Spacing();

                    if (ImGuiMCP::Button("Update Mapping and Save")) {
                        edit_info.name = edit_nameBuf;

                        bool success = InputManagerAPI::_API->UpdateActionMapping(ui_selectedActionID, edit_info);
                        if (success) {
                            updateStatusMsg = "Success! Mapping updated and saved.";
                            updateSuccess = true;
                            Settings::BlockActionID = ui_selectedActionID;
                            settings_changed = true;

                            SKSE::log::info("Action {} configuration updated successfully.", ui_selectedActionID);
                        }
                        else {
                            updateStatusMsg = "ERROR! Failed to save: Duplicate name or Combo already registered.";
                            updateSuccess = false;
                            SKSE::log::error("Failed to update Action {}. Check for conflicts.", ui_selectedActionID);
                        }
                    }

                    if (!updateStatusMsg.empty()) {
                        if (updateSuccess) {
                            ImGuiMCP::TextColored({ 0.2f, 1.0f, 0.2f, 1.0f }, "%s", updateStatusMsg.c_str());
                        }
                        else {
                            ImGuiMCP::TextColored({ 1.0f, 0.2f, 0.2f, 1.0f }, "%s", updateStatusMsg.c_str());
                        }
                    }

                    ImGuiMCP::TreePop();
                }
                else {
                    current_edit_action_id = -1;
                    updateStatusMsg = "";
                }
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (settings_changed) {
            SaveSettings();
        }
    }

    void Register() {
        LoadSettings();
        if (SKSEMenuFramework::IsInstalled()) {
            SKSEMenuFramework::SetSection("Just a Block");
            SKSEMenuFramework::AddSectionItem("Settings", Render);
            SKSE::log::info("Block Mod Menu registered successfully!");
        }
        else {
            SKSE::log::warn("SKSE Menu Framework not found, the Menu will not be rendered.");
        }
    }
}
