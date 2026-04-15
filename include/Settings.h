#include "InputManagerAPI.h"

namespace Settings {
    inline bool _isCurrentlyBlocking = false;
    inline int BlockActionID = 0;
    inline bool EnableMagicBlock = true;
    inline bool DisableBlockLeft = true;
}

namespace BlockModMenu {
    void Register();
}