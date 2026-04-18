
inline RE::SpellItem* g_savedRightHandSpell = nullptr;
inline RE::SpellItem* g_savedLeftHandSpell = nullptr;
inline RE::BGSEquipSlot* RightSlot = nullptr;
inline RE::BGSEquipSlot* LeftSlot = nullptr;
inline std::uint32_t LeftKeyboard = 0;
inline std::uint32_t LeftMouse = 0;
inline std::uint32_t LeftGamepad = 0;

namespace Idles {
	inline RE::TESIdleForm* BlockStart = nullptr;
	inline RE::TESIdleForm* BlockStop = nullptr;
	inline RE::TESIdleForm* GetIdleByFormID(RE::FormID formID, const std::string& pluginName);
	void InitIdles();
	inline void PlayIdleAnimation(RE::Actor* actor, RE::TESIdleForm* idle);
}

class PlayerAnimGraphListener : public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
public:
    static PlayerAnimGraphListener* GetSingleton() {
        static PlayerAnimGraphListener singleton;
        return &singleton;
    }

	virtual RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource) override;
};


class Block_InputManagerListener : public RE::BSTEventSink<SKSE::ModCallbackEvent> {
public:
	static Block_InputManagerListener* GetSingleton() {
		static Block_InputManagerListener singleton;
		return &singleton;
	}

	void Register() {
		auto dispatcher = SKSE::GetModCallbackEventSource();
		if (dispatcher) dispatcher->AddEventSink(this);
	}

	RE::BSEventNotifyControl ProcessEvent(const SKSE::ModCallbackEvent* a_event, RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;
}; 

class AttackStateManager
	: public RE::BSTEventSink<RE::InputEvent*> {
public:

	static AttackStateManager* GetSingleton() {
		static AttackStateManager singleton;
		return &singleton;
	}
	void Register();

	RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
		RE::BSTEventSource<RE::InputEvent*>* a_source) override;

};

void GetAttackKeys();