#pragma once
#include <windows.h>
#include "config.h"

class HotkeyManager {
public:
    static void RegisterToggleHotkey(const HotkeyConfig &cfg_h_s,const HotkeyConfig &cfg_exit);

    static bool IsToggleHotkey(const MSG &msg);

    static void UnregisterAll();
};
