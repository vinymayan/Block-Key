#include "Settings.h"
#include <string>
#include <vector>

namespace ImGui = ImGuiMCP;

namespace BlockModMenu {

    const char* SETTINGS_PATH = "Data/SKSE/Plugins/JusBlock_Settings.json";
    const char* LANG_PATH = "Data/SKSE/Plugins/JusBlock_Language.json";
    static std::unordered_map<std::string, std::string> LangMap;

    void LoadLanguage() {
        LangMap.clear();
        std::ifstream file(LANG_PATH, std::ios::binary);
        if (!file.is_open()) {
            SKSE::log::warn("Não foi possível carregar DodgeMod_Language.json. Usando textos padrões.");
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string jsonStr = buffer.str();
        file.close();

        if (jsonStr.size() >= 3 && (unsigned char)jsonStr[0] == 0xEF && (unsigned char)jsonStr[1] == 0xBB && (unsigned char)jsonStr[2] == 0xBF) {
            jsonStr.erase(0, 3);
        }

        rapidjson::Document doc;
        doc.Parse(jsonStr.c_str());

        if (doc.HasParseError()) return;

        if (doc.IsObject()) {
            for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
                if (itr->value.IsObject()) {
                    std::string category = itr->name.GetString();
                    for (auto jtr = itr->value.MemberBegin(); jtr != itr->value.MemberEnd(); ++jtr) {
                        if (jtr->value.IsString()) {
                            LangMap[category + "." + jtr->name.GetString()] = jtr->value.GetString();
                        }
                    }
                }
                else if (itr->value.IsString()) {
                    LangMap[itr->name.GetString()] = itr->value.GetString();
                }
            }
        }
    }

    const char* GetLoc(const std::string& key, const char* defaultVal) {
        auto it = LangMap.find(key);
        if (it != LangMap.end()) return it->second.c_str();
        return defaultVal;
    }

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
        const char* preview_value = (*current_item >= 0 && *current_item < items_count) ? items[*current_item] : GetLoc("common.none", "None");

        if (ImGui::BeginCombo(label, preview_value)) {
            static char searchBuf[128] = "";
            if (ImGui::IsWindowAppearing()) {
                searchBuf[0] = '\0';
                ImGui::SetKeyboardFocusHere();
            }
            std::string searchLabel = std::string(GetLoc("common.search_placeholder", "Filter...")) + "##Search";
            ImGui::InputText(searchLabel.c_str(), searchBuf, sizeof(searchBuf));
            ImGui::Separator();

            std::string searchLower = ToLower(searchBuf);

            for (int i = 0; i < items_count; i++) {
                if (searchLower.empty() || ToLower(items[i]).find(searchLower) != std::string::npos) {
                    bool is_selected = (*current_item == i);
                    if (ImGui::Selectable(items[i], is_selected)) {
                        *current_item = i;
                        changed = true;
                    }
                    if (is_selected && ImGui::IsWindowAppearing()) {
                        ImGui::SetScrollHereY();
                    }
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }


    void ReadJSON(rapidjson::Document& doc, const char* nome, std::vector<int>& lista) {
        if (doc.HasMember(nome) && doc[nome].IsArray()) {
            lista.clear();
            for (auto& v : doc[nome].GetArray()) {
                if (v.IsInt()) lista.push_back(v.GetInt());
            }
        }
    }

    void WriteJSON(rapidjson::Document& doc, rapidjson::Document::AllocatorType& alloc, const char* nome, const std::vector<int>& lista) {
        rapidjson::Value array(rapidjson::kArrayType);
        for (int id : lista) {
            array.PushBack(id, alloc);
        }
        rapidjson::Value chave; chave.SetString(nome, alloc);
        doc.AddMember(chave, array, alloc);
    }

    // --- SISTEMA DE INPUT LIST ---
    void EditInputVectors(const char* label, const std::string& actionIdStr, std::vector<int>& actionsRef, std::vector<int>& motionsRef) {
        ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "%s", label);

        if (!InputManagerAPI::_API) {
            ImGui::TextDisabled("%s", GetLoc("menu.input_manager_missing", "[Input Manager not found in memory]"));
            return;
        }

        bool changed = false;
        static int editingActionId = -1;
        static InputManagerAPI::ActionInfo editStagingInfo{};
        static bool showEditError = false;

        bool openEditPopup = false;
        std::string editPopupId = "EditActionPopup_" + actionIdStr;

        auto DrawSelectedList = [&](std::vector<int>& list, int type, const char* typeName) {
            const char* localizedType = (type == 0) ? GetLoc("common.action", "Action") : GetLoc("common.motion", "Motion");
            for (size_t i = 0; i < list.size(); i++) {
                ImGui::PushID((std::string(typeName) + "_" + std::to_string(i) + actionIdStr).c_str());

                const char* name = InputManagerAPI::_API->GetInputName(type, list[i]);
                std::string displayName = std::string("[") + localizedType + "] [" + std::to_string(list[i]) + "] " + (name ? name : GetLoc("common.unnamed", "Unnamed"));

                ImGui::Text("%s", displayName.c_str());

                if (type == 0) {
                    ImGui::SameLine();
                    if (ImGui::Button(GetLoc("common.edit", "Edit"))) {
                        editingActionId = list[i];
                        editStagingInfo = InputManagerAPI::_API->GetActionInfo(editingActionId);
                        showEditError = false;
                        openEditPopup = true;
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("X")) {
                    list.erase(list.begin() + i);
                    changed = true;
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            };

        DrawSelectedList(actionsRef, 0, "Action");
        DrawSelectedList(motionsRef, 1, "Motion");

        if (openEditPopup) ImGui::OpenPopup(editPopupId.c_str());

        // POPUP EDIÇÃO DE AÇÃO
        if (ImGui::BeginPopup(editPopupId.c_str())) {
            if (editingActionId != -1 && editStagingInfo.isValid) {
                ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "%s: %s", GetLoc("menu.editing_action", "Editing Action"), editStagingInfo.name ? editStagingInfo.name : GetLoc("common.unnamed", "Unnamed"));
                ImGui::Separator();

                int pcKeySize = sizeof(pcKeyIDs) / sizeof(pcKeyIDs[0]);
                int padKeySize = sizeof(gamepadKeyIDs) / sizeof(gamepadKeyIDs[0]);

                auto DrawMainActionCombo = [](const char* label, int& current_action) {
                    if (ImGui::BeginCombo(label, actionStateNames[current_action])) {
                        for (int n = 0; n < 5; n++) {
                            if (n == 3) continue;
                            bool is_selected = (current_action == n);
                            if (ImGui::Selectable(actionStateNames[n], is_selected)) current_action = n;
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    };

                auto DrawModActionCombo = [](const char* label, int& current_action, int main_action) {
                    if (ImGui::BeginCombo(label, actionStateNames[current_action])) {
                        for (int n = 0; n < 5; n++) {
                            if (n == 3 && main_action != 2 && main_action != 4) continue;
                            bool is_selected = (current_action == n);
                            if (ImGui::Selectable(actionStateNames[n], is_selected)) current_action = n;
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    };

                auto DrawGestureCombo = [](const char* label, uint32_t& current_gesture) {
                    size_t gestCount = InputManagerAPI::_API->GetInputCount(2);
                    int current = static_cast<int>(current_gesture);
                    const char* preview = (current >= 0 && current < gestCount) ? InputManagerAPI::_API->GetInputName(2, current) : GetLoc("common.none", "None");

                    if (ImGui::BeginCombo(label, preview)) {
                        for (int i = 0; i < gestCount; ++i) {
                            bool is_selected = (current == i);
                            if (ImGui::Selectable(InputManagerAPI::_API->GetInputName(2, i), is_selected)) {
                                current_gesture = static_cast<uint32_t>(i);
                            }
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    };

                auto DrawStickCombo = [](const char* label, int& current_stick) {
                    const char* sticks[] = { GetLoc("menu.left_stick", "Left Stick"), GetLoc("menu.right_stick", "Right Stick") };
                    const char* preview = (current_stick >= 0 && current_stick < 2) ? sticks[current_stick] : GetLoc("common.unnamed", "Unknown");
                    if (ImGui::BeginCombo(label, preview)) {
                        for (int i = 0; i < 2; i++) {
                            bool is_selected = (current_stick == i);
                            if (ImGui::Selectable(sticks[i], is_selected)) current_stick = i;
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                    };

                // PC
                ImGui::TextColored({ 0.7f, 0.7f, 1.0f, 1.0f }, "%s", GetLoc("menu.pc_settings_header", "--- PC Settings ---"));
                int pcMainIdx = GetIndexFromID(editStagingInfo.pcMainKey, pcKeyIDs, pcKeySize);
                if (SearchableCombo(GetLoc("menu.pc_main_key", "PC Main Key"), &pcMainIdx, pcKeyNames, pcKeySize)) editStagingInfo.pcMainKey = pcKeyIDs[pcMainIdx];
                DrawMainActionCombo(GetLoc("menu.pc_main_action", "PC Main Action"), editStagingInfo.pcMainAction);
                if (editStagingInfo.pcMainAction == 1) ImGui::InputInt(GetLoc("menu.pc_main_taps", "PC Main Taps"), &editStagingInfo.pcMainTapCount);

                if (editStagingInfo.pcModAction == 3 && editStagingInfo.pcMainAction != 2 && editStagingInfo.pcMainAction != 4) editStagingInfo.pcModAction = 0;

                DrawModActionCombo(GetLoc("menu.pc_mod_action", "PC Mod Action"), editStagingInfo.pcModAction, editStagingInfo.pcMainAction);
                if (editStagingInfo.pcModAction == 3) {
                    DrawGestureCombo(GetLoc("menu.pc_gesture", "PC Gesture"), editStagingInfo.pcModifierKey);
                }
                else {
                    int pcModIdx = GetIndexFromID(editStagingInfo.pcModifierKey, pcKeyIDs, pcKeySize);
                    if (SearchableCombo(GetLoc("menu.pc_mod_key", "PC Mod Key"), &pcModIdx, pcKeyNames, pcKeySize)) editStagingInfo.pcModifierKey = pcKeyIDs[pcModIdx];
                    if (editStagingInfo.pcModAction == 1) ImGui::InputInt(GetLoc("menu.pc_mod_taps", "PC Mod Taps"), &editStagingInfo.pcModTapCount);
                }

                // GAMEPAD
                ImGui::Spacing();
                ImGui::TextColored({ 0.7f, 1.0f, 0.7f, 1.0f }, "%s", GetLoc("menu.pad_settings_header", "--- Gamepad Settings ---"));
                int padMainIdx = GetIndexFromID(editStagingInfo.gamepadMainKey, gamepadKeyIDs, padKeySize);
                if (SearchableCombo(GetLoc("menu.pad_main_key", "Pad Main Key"), &padMainIdx, gamepadKeyNames, padKeySize)) editStagingInfo.gamepadMainKey = gamepadKeyIDs[padMainIdx];
                DrawMainActionCombo(GetLoc("menu.pad_main_action", "Pad Main Action"), editStagingInfo.gamepadMainAction);
                if (editStagingInfo.gamepadMainAction == 1) ImGui::InputInt(GetLoc("menu.pad_main_taps", "Pad Main Taps"), &editStagingInfo.gamepadMainTapCount);

                if (editStagingInfo.gamepadModAction == 3 && editStagingInfo.gamepadMainAction != 2 && editStagingInfo.gamepadMainAction != 4) editStagingInfo.gamepadModAction = 0;

                DrawModActionCombo(GetLoc("menu.pad_mod_action", "Pad Mod Action"), editStagingInfo.gamepadModAction, editStagingInfo.gamepadMainAction);
                if (editStagingInfo.gamepadModAction == 3) {
                    DrawGestureCombo(GetLoc("menu.pad_gesture", "Pad Gesture"), editStagingInfo.gamepadModifierKey);
                    DrawStickCombo(GetLoc("menu.pad_gesture_stick", "Gesture Stick"), editStagingInfo.gamepadGestureStick);
                }
                else {
                    int padModIdx = GetIndexFromID(editStagingInfo.gamepadModifierKey, gamepadKeyIDs, padKeySize);
                    if (SearchableCombo(GetLoc("menu.pad_mod_key", "Pad Mod Key"), &padModIdx, gamepadKeyNames, padKeySize)) editStagingInfo.gamepadModifierKey = gamepadKeyIDs[padModIdx];
                    if (editStagingInfo.gamepadModAction == 1) ImGui::InputInt(GetLoc("menu.pad_mod_taps", "Pad Mod Taps"), &editStagingInfo.gamepadModTapCount);
                }

                ImGui::Separator();
                if (showEditError) {
                    ImGui::TextColored({ 1.0f, 0.2f, 0.2f, 1.0f }, "%s", GetLoc("menu.save_error", "Error: Conflict detected or invalid input!"));
                }

                if (ImGui::Button(GetLoc("common.save", "Save"), { 120, 0 })) {
                    bool success = InputManagerAPI::_API->UpdateActionMapping(editingActionId, editStagingInfo);
                    if (success) {
                        ImGui::CloseCurrentPopup();
                        editingActionId = -1;
                    }
                    else {
                        showEditError = true;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(GetLoc("common.cancel", "Cancel"), { 120, 0 })) {
                    ImGui::CloseCurrentPopup();
                    editingActionId = -1;
                }
            }
            ImGui::EndPopup();
        }

        std::string popupId = "AddInputPopup_" + actionIdStr;
        std::string addInputLabel = std::string(GetLoc("menu.add_input", "+ Add Input")) + "##" + actionIdStr;
        if (ImGui::Button(addInputLabel.c_str())) {
            ImGui::OpenPopup(popupId.c_str());
        }

        // POPUP ADD INPUT
        if (ImGui::BeginPopup(popupId.c_str())) {
            static char searchBuf[128] = "";
            if (ImGui::IsWindowAppearing()) {
                searchBuf[0] = '\0';
                ImGui::SetKeyboardFocusHere();
            }

            if (ImGui::BeginTabBar(("InputTabs_" + actionIdStr).c_str())) {
                int selectedType = -1;
                if (ImGui::BeginTabItem(GetLoc("common.actions", "Actions"))) {
                    selectedType = 0;
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem(GetLoc("common.motions", "Motions"))) {
                    selectedType = 1;
                    ImGui::EndTabItem();
                }

                if (selectedType != -1) {
                    std::string searchLabel = std::string(GetLoc("common.search_placeholder", "Filter...")) + "##SearchInput";
                    ImGui::InputText(searchLabel.c_str(), searchBuf, sizeof(searchBuf));
                    ImGui::Separator();

                    std::string searchLower = ToLower(searchBuf);

                    ImGui::BeginChild(("ChildList_" + actionIdStr).c_str(), { 300, 200 }, true);
                    size_t count = InputManagerAPI::_API->GetInputCount(selectedType);

                    for (int i = 0; i < count; i++) {
                        const char* name = InputManagerAPI::_API->GetInputName(selectedType, i);
                        std::string itemLabel = "[" + std::to_string(i) + "] " + (name ? name : GetLoc("common.unnamed", "Unnamed"));

                        bool matches = searchLower.empty();
                        if (!matches) matches = (ToLower(itemLabel).find(searchLower) != std::string::npos);

                        if (matches) {
                            if (ImGui::Selectable(itemLabel.c_str(), false)) {
                                if (selectedType == 0) {
                                    if (std::find(actionsRef.begin(), actionsRef.end(), i) == actionsRef.end()) {
                                        actionsRef.push_back(i);
                                        changed = true;
                                    }
                                }
                                else {
                                    if (std::find(motionsRef.begin(), motionsRef.end(), i) == motionsRef.end()) {
                                        motionsRef.push_back(i);
                                        changed = true;
                                    }
                                }
                                ImGui::CloseCurrentPopup();
                            }

                            if (ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::TextColored({ 0.4f, 1.0f, 0.4f, 1.0f }, "%s", GetLoc("menu.input_details", "Input Details"));
                                ImGui::Separator();

                                const char* idLoc = GetLoc("common.id", "ID");
                                const char* nameLoc = GetLoc("common.name", "Name");
                                const char* unnamedLoc = GetLoc("common.unnamed", "Unnamed");
                                const char* actionLoc = GetLoc("common.action", "Action");
                                const char* noInfoLoc = GetLoc("menu.no_info", "No information available.");

                                auto getActionName = [](int actionId) -> const char* {
                                    if (actionId >= 0 && actionId < 5) return actionStateNames[actionId];
                                    return "Unknown";
                                    };

                                auto formatAction = [&](int actionId, int tapCount) -> std::string {
                                    std::string n = getActionName(actionId);
                                    if (actionId == 1) { // 1 = Tap
                                        n += " x" + std::to_string(tapCount);
                                    }
                                    return n;
                                    };

                                auto getDirectionalName = [](int keyId) -> const char* {
                                    switch (keyId) {
                                    case InputManagerAPI::VKEY_DIR_UP:        return GetLoc("direction.up", "Up");
                                    case InputManagerAPI::VKEY_DIR_DOWN:      return GetLoc("direction.down", "Down");
                                    case InputManagerAPI::VKEY_DIR_LEFT:      return GetLoc("direction.left", "Left");
                                    case InputManagerAPI::VKEY_DIR_RIGHT:     return GetLoc("direction.right", "Right");
                                    case InputManagerAPI::VKEY_DIR_UPRIGHT:   return GetLoc("direction.up_right", "Up-Right");
                                    case InputManagerAPI::VKEY_DIR_UPLEFT:    return GetLoc("direction.up_left", "Up-Left");
                                    case InputManagerAPI::VKEY_DIR_DOWNRIGHT: return GetLoc("direction.down_right", "Down-Right");
                                    case InputManagerAPI::VKEY_DIR_DOWNLEFT:  return GetLoc("direction.down_left", "Down-Left");
                                    default: return nullptr;
                                    }
                                    };

                                auto getPcKeyName = [&](int keyId) -> const char* {
                                    if (const char* dirName = getDirectionalName(keyId)) return dirName;
                                    int size = sizeof(pcKeyIDs) / sizeof(pcKeyIDs[0]);
                                    int idx = GetIndexFromID(keyId, pcKeyIDs, size);
                                    return pcKeyNames[idx];
                                    };

                                auto getPadKeyName = [&](int keyId) -> const char* {
                                    if (const char* dirName = getDirectionalName(keyId)) return dirName;
                                    int size = sizeof(gamepadKeyIDs) / sizeof(gamepadKeyIDs[0]);
                                    int idx = GetIndexFromID(keyId, gamepadKeyIDs, size);
                                    return gamepadKeyNames[idx];
                                    };

                                if (selectedType == 0) { // Action
                                    auto info = InputManagerAPI::_API->GetActionInfo(i);

                                    if (info.isValid) {
                                        ImGui::Text("%s: %d | %s: %s", idLoc, info.id, nameLoc, info.name ? info.name : unnamedLoc);

                                        ImGui::Text("%s: %s (%s: %s)", GetLoc("menu.pc_main_key", "PC Main Key"),
                                            getPcKeyName(info.pcMainKey), actionLoc, formatAction(info.pcMainAction, info.pcMainTapCount).c_str());

                                        if (info.pcModifierKey != 0) {
                                            ImGui::Text("%s: %s (%s: %s)", GetLoc("menu.pc_mod_key", "PC Mod Key"),
                                                (info.pcModAction == 3) ? InputManagerAPI::_API->GetInputName(2, info.pcModifierKey) : getPcKeyName(info.pcModifierKey),
                                                actionLoc, formatAction(info.pcModAction, info.pcModTapCount).c_str());
                                        }

                                        ImGui::Text("%s: %s (%s: %s)", GetLoc("menu.pad_main_key", "Gamepad Main Key"),
                                            getPadKeyName(info.gamepadMainKey), actionLoc, formatAction(info.gamepadMainAction, info.gamepadMainTapCount).c_str());

                                        if (info.gamepadModifierKey != 0) {
                                            ImGui::Text("%s: %s (%s: %s)", GetLoc("menu.pad_mod_key", "Gamepad Mod Key"),
                                                (info.gamepadModAction == 3) ? InputManagerAPI::_API->GetInputName(2, info.gamepadModifierKey) : getPadKeyName(info.gamepadModifierKey),
                                                actionLoc, formatAction(info.gamepadModAction, info.gamepadModTapCount).c_str());
                                        }

                                        if (info.useCustomTimings) {
                                            ImGui::TextColored({ 0.8f, 0.8f, 0.4f, 1.0f }, "%s - %s: %.2fs | %s: %.2fs",
                                                GetLoc("menu.custom_timings", "Custom Timings"),
                                                GetLoc("menu.tap_window", "Tap Window"), info.tapWindow,
                                                GetLoc("menu.hold", "Hold"), info.holdDuration);
                                        }
                                    }
                                    else {
                                        ImGui::TextDisabled("%s", noInfoLoc);
                                    }
                                }
                                else { // Motion
                                    auto info = InputManagerAPI::_API->GetMotionInfo(i);

                                    if (info.isValid) {
                                        ImGui::Text("%s: %d | %s: %s", idLoc, info.id, nameLoc, info.name ? info.name : unnamedLoc);
                                        ImGui::Text("%s: %.2fs", GetLoc("menu.time_window", "Time Window"), info.timeWindow);

                                        std::string pcSeq = "";
                                        for (int k = 0; k < info.pcSequenceLength; k++) {
                                            if (k > 0) pcSeq += ", ";
                                            pcSeq += getPcKeyName(info.pcSequence[k]);
                                        }
                                        ImGui::Text("%s: %d [%s]", GetLoc("menu.pc_seq_size", "PC Sequence"), info.pcSequenceLength, pcSeq.c_str());

                                        std::string padSeq = "";
                                        for (int k = 0; k < info.padSequenceLength; k++) {
                                            if (k > 0) padSeq += ", ";
                                            padSeq += getPadKeyName(info.padSequence[k]);
                                        }
                                        ImGui::Text("%s: %d [%s]", GetLoc("menu.pad_seq_size", "Gamepad Sequence"), info.padSequenceLength, padSeq.c_str());
                                    }
                                    else {
                                        ImGui::TextDisabled("%s", noInfoLoc);
                                    }
                                }
                                ImGui::EndTooltip();
                            }
                        }
                    }
                    ImGui::EndChild();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndPopup();
        }

        if (changed) {
            UnregisterInputCategory(actionIdStr);
            RegisterAllInputs();
            SaveSettings();
            TweenPauseRegister();
        }
    }

    void UnregisterInputCategory(const std::string& actionId) {
        if (!InputManagerAPI::_API) return;

        if (actionId == "Block") {
            for (int id : Settings::BlockActionIDs) InputManagerAPI::_API->UpdateListener(0, id, "Just a block", "Block", false, nullptr, 0, nullptr, 0);
            for (int id : Settings::BlockMotionIDs) InputManagerAPI::_API->UpdateListener(1, id, "Just a block", "Block", false, nullptr, 0, nullptr, 0);
        }
    }

    void RegisterAllInputs() {
        if (!InputManagerAPI::_API) return;

        for (int id : Settings::BlockActionIDs) InputManagerAPI::_API->UpdateListener(0, id, "Just a block", "Block", true, nullptr, 0, nullptr, 0);
        for (int id : Settings::BlockMotionIDs) InputManagerAPI::_API->UpdateListener(1, id, "Just a block", "Block", true, nullptr, 0, nullptr, 0);
    }

    void TweenPauseRegister() {
        auto dispatcher = SKSE::GetModCallbackEventSource();
        if (!dispatcher) return;

        auto EnviarPayload = [&](const char* actionIdStr, const char* actionLabelStr, const std::vector<int>& actions, const std::vector<int>& motions) {
            rapidjson::Document doc;
            doc.SetObject();
            auto& alloc = doc.GetAllocator();

            doc.AddMember("tabId", rapidjson::StringRef("gameplay"), alloc);
            doc.AddMember("tabLabel", rapidjson::StringRef("Mods"), alloc);
            doc.AddMember("categoryId", rapidjson::StringRef("combat"), alloc);
            doc.AddMember("categoryLabel", rapidjson::StringRef("Combat"), alloc);

            rapidjson::Value actionIdVal; actionIdVal.SetString(actionIdStr, alloc);
            doc.AddMember("actionId", actionIdVal, alloc);

            rapidjson::Value actionLabelVal; actionLabelVal.SetString(actionLabelStr, alloc);
            doc.AddMember("actionLabel", actionLabelVal, alloc);

            doc.AddMember("acceptsMotion", true, alloc);

            rapidjson::Value mappedIds(rapidjson::kArrayType);

            for (int id : actions) {
                rapidjson::Value bind(rapidjson::kObjectType);
                bind.AddMember("id", id, alloc);
                bind.AddMember("type", rapidjson::StringRef("action"), alloc);
                mappedIds.PushBack(bind, alloc);
            }
            for (int id : motions) {
                rapidjson::Value bind(rapidjson::kObjectType);
                bind.AddMember("id", id, alloc);
                bind.AddMember("type", rapidjson::StringRef("motion"), alloc);
                mappedIds.PushBack(bind, alloc);
            }

            doc.AddMember("mappedIds", mappedIds, alloc);

            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            SKSE::ModCallbackEvent modEvent{ "TweenPause_RegisterControl", RE::BSFixedString(buffer.GetString()), 0.0f, nullptr };
            dispatcher->SendEvent(&modEvent);
            };

        EnviarPayload("Block", GetLoc("menu.block", "Block"), Settings::BlockActionIDs, Settings::BlockMotionIDs);
    }

    void SaveSettings() {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

        WriteJSON(doc, allocator, "BlockActionIDs", Settings::BlockActionIDs);
        WriteJSON(doc, allocator, "BlockMotionIDs", Settings::BlockMotionIDs);
        doc.AddMember("EnableMagicBlock", Settings::EnableMagicBlock, allocator);
        doc.AddMember("DisableBlockLeft", Settings::DisableBlockLeft, allocator);
        doc.AddMember("CancelDodgeWithBlock", Settings::CancelDodgeWithBlock, allocator); // <-- SALVAR NO JSON

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
                ReadJSON(doc, "BlockActionIDs", Settings::BlockActionIDs);
                ReadJSON(doc, "BlockMotionIDs", Settings::BlockMotionIDs);
                if (doc.HasMember("EnableMagicBlock")) Settings::EnableMagicBlock = doc["EnableMagicBlock"].GetBool();
                if (doc.HasMember("DisableBlockLeft")) Settings::DisableBlockLeft = doc["DisableBlockLeft"].GetBool();
                if (doc.HasMember("CancelDodgeWithBlock")) Settings::CancelDodgeWithBlock = doc["CancelDodgeWithBlock"].GetBool(); // <-- CARREGAR DO JSON
            }
        }
    }

    void __stdcall Render() {
        bool settings_changed = false;

        ImGui::Text("%s", GetLoc("menu.block_settings_title", "Block Settings"));
        ImGui::Separator();
        ImGui::Spacing();

        EditInputVectors(GetLoc("menu.block_config", "Block Config"), "Block", Settings::BlockActionIDs, Settings::BlockMotionIDs);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGuiMCP::Checkbox(GetLoc("menu.enable_magic_block", "Enable Magic Block"), &Settings::EnableMagicBlock)) {
            settings_changed = true;
        }
        if (ImGuiMCP::Checkbox(GetLoc("menu.disable_block_left", "Disable Block on Left Hand"), &Settings::DisableBlockLeft)) {
            settings_changed = true;
        }
        if (ImGuiMCP::Checkbox(GetLoc("menu.cancel_dodge_with_block", "Cancel Dodge With Block"), &Settings::CancelDodgeWithBlock)) {
            settings_changed = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", GetLoc("menu.cancel_dodge_with_block_hover", "This functionality only works with 'Dodge for all'."));
            ImGui::EndTooltip();
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
            LoadLanguage();
            LoadSettings();
            SKSEMenuFramework::SetSection("Just a Block");
            SKSEMenuFramework::AddSectionItem("Settings", Render);
            SKSE::log::info("Block Mod Menu registered successfully!");
        }
        else {
            SKSE::log::warn("SKSE Menu Framework not found, the Menu will not be rendered.");
        }
    }
}
