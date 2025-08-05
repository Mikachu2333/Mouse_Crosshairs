#pragma once
#include <windows.h>
#include "config.h"

class HotkeyManager {
public:
    static void RegisterToggleHotkey(const HotkeyConfig &cfg);

    static bool IsToggleHotkey(const MSG &msg);

    static void UnregisterAll();
};
