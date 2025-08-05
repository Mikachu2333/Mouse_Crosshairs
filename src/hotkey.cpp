#include "./hotkey.h"

// 热键 ID 常量
#define HOTKEY_ID 2039166592   // 显示/隐藏热键 ID
#define HOTKEY_ID2 2078194518  // 退出程序热键 ID

// 注册显示/隐藏和退出热键
void HotkeyManager::RegisterToggleHotkey(const HotkeyConfig &cfg_h_s,const HotkeyConfig &cfg_exit) {
    RegisterHotKey(nullptr, HOTKEY_ID, cfg_h_s.mod, cfg_h_s.vk);
    RegisterHotKey(nullptr, HOTKEY_ID2, cfg_exit.mod, cfg_exit.vk);
}

// 检查消息是否为显示/隐藏热键
bool HotkeyManager::IsToggleHotkey(const MSG &msg) {
    return msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID;
}

// 检查消息是否为退出热键
bool HotkeyManager::IsExitHotkey(const MSG &msg) {
    return msg.message == WM_HOTKEY && msg.wParam == HOTKEY_ID2;
}

// 注销所有热键
void HotkeyManager::UnregisterAll() {
    UnregisterHotKey(nullptr, HOTKEY_ID);
    UnregisterHotKey(nullptr, HOTKEY_ID2);
}
