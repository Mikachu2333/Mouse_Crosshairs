#include "./hotkey.h"

// 注册显示/隐藏和退出热键
bool HotkeyManager::RegisterToggleHotkey(const HotkeyConfig& cfg_h_s,
                                         const HotkeyConfig& cfg_exit) {
  UnregisterAll();
  if (!RegisterHotKey(nullptr, HOTKEY_ID, cfg_h_s.mod, cfg_h_s.vk)) {
    return false;
  }
  if (!RegisterHotKey(nullptr, HOTKEY_ID2, cfg_exit.mod, cfg_exit.vk)) {
    UnregisterHotKey(nullptr, HOTKEY_ID);
    return false;
  }
  return true;
}

// 注销所有热键
void HotkeyManager::UnregisterAll() {
  UnregisterHotKey(nullptr, HOTKEY_ID);
  UnregisterHotKey(nullptr, HOTKEY_ID2);
}
