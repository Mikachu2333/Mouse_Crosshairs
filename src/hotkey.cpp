#include "./hotkey.h"

#define HOTKEY_ID 2039166592
#define HOTKEY_ID2 2078194518

void HotkeyManager::RegisterToggleHotkey(const HotkeyConfig &cfg_h_s,const HotkeyConfig &cfg_exit) {
    RegisterHotKey(nullptr, HOTKEY_ID, cfg_h_s.mod, cfg_h_s.vk);
    RegisterHotKey(nullptr, HOTKEY_ID2, cfg_exit.mod, cfg_exit.vk);
}

bool HotkeyManager::IsToggleHotkey(const MSG &msg) {
    return msg.message == WM_HOTKEY && (msg.wParam == HOTKEY_ID || msg.wParam == HOTKEY_ID2);
}

void HotkeyManager::UnregisterAll() {
    UnregisterHotKey(nullptr, HOTKEY_ID);
    UnregisterHotKey(nullptr, HOTKEY_ID2);
}
