#include "Settings.h"
#include "InputManagerAPI.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"

// RapidJSON Includes
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

#include <string>
#include <vector>

namespace ImGui = ImGuiMCP;

namespace BlockModMenu {

    const char* SETTINGS_PATH = "Data/SKSE/Plugins/JusBlock_Settings.json";
    inline const char* actionStateNames[] = { "Ignore", "Tap", "Hold", "Press" };
    inline const char* mainActionNames[] = { "Hold", "Press" };
    constexpr uint32_t MOUSE_OFFSET = 256;
    constexpr uint32_t GAMEPAD_OFFSET = 266;

    inline const char* pcKeyNames[] = {
        "None",
        // Mouse
        "Mouse 1 (Left)", "Mouse 2 (Right)", "Mouse 3 (Middle)", "Mouse 4", "Mouse 5", "Mouse 6", "Mouse 7", "Mouse 8",
        "Mouse Wheel Up", "Mouse Wheel Down",
        // Alfabeto
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
        // Números
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        // Pontuação e Símbolos
        "Minus ( - )", "Equals ( = )", "Bracket Left ( [ )", "Bracket Right ( ] )", "Semicolon ( ; )", "Apostrophe ( ' )", "Tilde ( ~ )", "Backslash ( \\ )", "Comma ( , )", "Period ( . )", "Slash ( / )",
        // F-Keys
        "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        // Especiais e Modificadores
        "Esc", "Tab", "Caps Lock", "Shift (Left)", "Shift (Right)", "Ctrl (Left)", "Ctrl (Right)", "Alt (Left)", "Alt (Right)",
        "Space", "Enter", "Backspace", "Print Screen", "Scroll Lock", "Pause", "Num Lock",
        // Navegação
        "Up Arrow", "Down Arrow", "Left Arrow", "Right Arrow", "Insert", "Delete", "Home", "End", "Page Up", "Page Down",
        // Numpad
        "Num 0", "Num 1", "Num 2", "Num 3", "Num 4", "Num 5", "Num 6", "Num 7", "Num 8", "Num 9",
        "Num +", "Num -", "Num *", "Num /", "Num Enter", "Num Dot"
    };

    inline const int pcKeyIDs[] = {
        0,
        // Mouse (Base + 256)
        RE::BSWin32MouseDevice::Keys::kLeftButton + MOUSE_OFFSET, RE::BSWin32MouseDevice::Keys::kRightButton + MOUSE_OFFSET,
        RE::BSWin32MouseDevice::Keys::kMiddleButton + MOUSE_OFFSET, RE::BSWin32MouseDevice::Keys::kButton3 + MOUSE_OFFSET,
        RE::BSWin32MouseDevice::Keys::kButton4 + MOUSE_OFFSET, RE::BSWin32MouseDevice::Keys::kButton5 + MOUSE_OFFSET,
        RE::BSWin32MouseDevice::Keys::kButton6 + MOUSE_OFFSET, RE::BSWin32MouseDevice::Keys::kButton7 + MOUSE_OFFSET,
        RE::BSWin32MouseDevice::Keys::kWheelUp + MOUSE_OFFSET, RE::BSWin32MouseDevice::Keys::kWheelDown + MOUSE_OFFSET,
        // Alfabeto
        RE::BSKeyboardDevice::Keys::kA, RE::BSKeyboardDevice::Keys::kB, RE::BSKeyboardDevice::Keys::kC, RE::BSKeyboardDevice::Keys::kD,
        RE::BSKeyboardDevice::Keys::kE, RE::BSKeyboardDevice::Keys::kF, RE::BSKeyboardDevice::Keys::kG, RE::BSKeyboardDevice::Keys::kH,
        RE::BSKeyboardDevice::Keys::kI, RE::BSKeyboardDevice::Keys::kJ, RE::BSKeyboardDevice::Keys::kK, RE::BSKeyboardDevice::Keys::kL,
        RE::BSKeyboardDevice::Keys::kM, RE::BSKeyboardDevice::Keys::kN, RE::BSKeyboardDevice::Keys::kO, RE::BSKeyboardDevice::Keys::kP,
        RE::BSKeyboardDevice::Keys::kQ, RE::BSKeyboardDevice::Keys::kR, RE::BSKeyboardDevice::Keys::kS, RE::BSKeyboardDevice::Keys::kT,
        RE::BSKeyboardDevice::Keys::kU, RE::BSKeyboardDevice::Keys::kV, RE::BSKeyboardDevice::Keys::kW, RE::BSKeyboardDevice::Keys::kX,
        RE::BSKeyboardDevice::Keys::kY, RE::BSKeyboardDevice::Keys::kZ,
        // Números
        RE::BSKeyboardDevice::Keys::kNum1, RE::BSKeyboardDevice::Keys::kNum2, RE::BSKeyboardDevice::Keys::kNum3, RE::BSKeyboardDevice::Keys::kNum4,
        RE::BSKeyboardDevice::Keys::kNum5, RE::BSKeyboardDevice::Keys::kNum6, RE::BSKeyboardDevice::Keys::kNum7, RE::BSKeyboardDevice::Keys::kNum8,
        RE::BSKeyboardDevice::Keys::kNum9, RE::BSKeyboardDevice::Keys::kNum0,
        // Pontuação e Símbolos
        RE::BSKeyboardDevice::Keys::kMinus, RE::BSKeyboardDevice::Keys::kEquals, RE::BSKeyboardDevice::Keys::kBracketLeft,
        RE::BSKeyboardDevice::Keys::kBracketRight, RE::BSKeyboardDevice::Keys::kSemicolon, RE::BSKeyboardDevice::Keys::kApostrophe,
        RE::BSKeyboardDevice::Keys::kTilde, RE::BSKeyboardDevice::Keys::kBackslash, RE::BSKeyboardDevice::Keys::kComma,
        RE::BSKeyboardDevice::Keys::kPeriod, RE::BSKeyboardDevice::Keys::kSlash,
        // F-Keys
        RE::BSKeyboardDevice::Keys::kF1, RE::BSKeyboardDevice::Keys::kF2, RE::BSKeyboardDevice::Keys::kF3, RE::BSKeyboardDevice::Keys::kF4,
        RE::BSKeyboardDevice::Keys::kF5, RE::BSKeyboardDevice::Keys::kF6, RE::BSKeyboardDevice::Keys::kF7, RE::BSKeyboardDevice::Keys::kF8,
        RE::BSKeyboardDevice::Keys::kF9, RE::BSKeyboardDevice::Keys::kF10, RE::BSKeyboardDevice::Keys::kF11, RE::BSKeyboardDevice::Keys::kF12,
        // Especiais e Modificadores
        RE::BSKeyboardDevice::Keys::kEscape, RE::BSKeyboardDevice::Keys::kTab, RE::BSKeyboardDevice::Keys::kCapsLock,
        RE::BSKeyboardDevice::Keys::kLeftShift, RE::BSKeyboardDevice::Keys::kRightShift,
        RE::BSKeyboardDevice::Keys::kLeftControl, RE::BSKeyboardDevice::Keys::kRightControl,
        RE::BSKeyboardDevice::Keys::kLeftAlt, RE::BSKeyboardDevice::Keys::kRightAlt,
        RE::BSKeyboardDevice::Keys::kSpacebar, RE::BSKeyboardDevice::Keys::kEnter, RE::BSKeyboardDevice::Keys::kBackspace,
        RE::BSKeyboardDevice::Keys::kPrintScreen, RE::BSKeyboardDevice::Keys::kScrollLock, RE::BSKeyboardDevice::Keys::kPause,
        RE::BSKeyboardDevice::Keys::kNumLock,
        // Navegação
        RE::BSKeyboardDevice::Keys::kUp, RE::BSKeyboardDevice::Keys::kDown, RE::BSKeyboardDevice::Keys::kLeft, RE::BSKeyboardDevice::Keys::kRight,
        RE::BSKeyboardDevice::Keys::kInsert, RE::BSKeyboardDevice::Keys::kDelete, RE::BSKeyboardDevice::Keys::kHome, RE::BSKeyboardDevice::Keys::kEnd,
        RE::BSKeyboardDevice::Keys::kPageUp, RE::BSKeyboardDevice::Keys::kPageDown,
        // Numpad
        RE::BSKeyboardDevice::Keys::kKP_0, RE::BSKeyboardDevice::Keys::kKP_1, RE::BSKeyboardDevice::Keys::kKP_2, RE::BSKeyboardDevice::Keys::kKP_3,
        RE::BSKeyboardDevice::Keys::kKP_4, RE::BSKeyboardDevice::Keys::kKP_5, RE::BSKeyboardDevice::Keys::kKP_6, RE::BSKeyboardDevice::Keys::kKP_7,
        RE::BSKeyboardDevice::Keys::kKP_8, RE::BSKeyboardDevice::Keys::kKP_9,
        RE::BSKeyboardDevice::Keys::kKP_Plus, RE::BSKeyboardDevice::Keys::kKP_Subtract, RE::BSKeyboardDevice::Keys::kKP_Multiply,
        RE::BSKeyboardDevice::Keys::kKP_Divide, RE::BSKeyboardDevice::Keys::kKP_Enter, RE::BSKeyboardDevice::Keys::kKP_Decimal
    };

    inline const char* gamepadKeyNames[] = {
        "None",
        "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
        "Start / Options", "Back / Share / Select", "LS / L3 (Left Stick)", "RS / R3 (Right Stick)",
        "LB / L1 (Left Bumper)", "RB / R1 (Right Bumper)",
        "LT / L2 (Left Trigger)", "RT / R2 (Right Trigger)",
        "A / Cross", "B / Circle", "X / Square", "Y / Triangle"
    };

    inline const int gamepadKeyIDs[] = {
        0,
        RE::BSWin32GamepadDevice::Keys::kUp + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kDown + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kLeft + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kRight + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kStart + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kBack + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kLeftThumb + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kRightThumb + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kLeftShoulder + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kRightShoulder + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kLeftTrigger + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kRightTrigger + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kA + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kB + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kX + GAMEPAD_OFFSET,
        RE::BSWin32GamepadDevice::Keys::kY + GAMEPAD_OFFSET
    };

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
    static int ui_pcMainActIdx = 0;
    // Gamepad
    static int ui_padMainIdx = 0;
    static int ui_padModIdx = 0;
    static int current_padModAct = 0;
    static int ui_padMainActIdx = 0;
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
                        ui_pcMainActIdx = (edit_info.pcMainAction == 4) ? 1 : 0;
                        ui_pcModIdx = GetIndexFromID(edit_info.pcModifierKey, pcKeyIDs, std::size(pcKeyIDs));

                        ui_padMainIdx = GetIndexFromID(edit_info.gamepadMainKey, gamepadKeyIDs, std::size(gamepadKeyIDs));
                        current_padModAct = edit_info.gamepadModAction;
                        ui_padMainActIdx = (edit_info.gamepadMainAction == 4) ? 1 : 0;
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

                    if (ImGuiMCP::Combo("PC Mod Action", &current_pcModAct, actionStateNames, 5)) {
                        if (current_pcModAct == 3) current_pcModAct = 0;
                    }

                    ImGuiMCP::Combo("PC Mod Action", &current_pcModAct, actionStateNames, std::size(actionStateNames));

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

                    if (ImGuiMCP::Combo("Pad Main Action", &ui_padMainActIdx, mainActionNames, 2)) {
                        edit_info.gamepadMainAction = (ui_padMainActIdx == 1) ? 4 : 2;
                    }

                    if (ImGuiMCP::Combo("Pad Mod Action", &current_padModAct, actionStateNames, 5)) {
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
