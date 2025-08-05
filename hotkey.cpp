#include "hotkey.h"

#define HOTKEY_ID 2039166592

void HotkeyManager::RegisterToggleHotkey(const HotkeyConfig &cfg) {
    RegisterHotKey(nullptr, HOTKEY_ID, cfg.mod, cfg.vk);
}

bool HotkeyManager::IsToggleHotkey(const MSG &msg) {
    return msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID;
}

void HotkeyManager::UnregisterAll() {
    UnregisterHotKey(nullptr, HOTKEY_ID);
}
