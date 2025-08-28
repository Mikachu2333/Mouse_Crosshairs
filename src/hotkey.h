#pragma once
#include <windows.h>

#include "config.h"

// 热键 ID 常量
#define HOTKEY_ID 2039166592   // 显示/隐藏热键 ID
#define HOTKEY_ID2 2078194518  // 退出程序热键 ID

class HotkeyManager {
 public:
  static void RegisterToggleHotkey(const HotkeyConfig &cfg_h_s,
                                   const HotkeyConfig &cfg_exit);

  static void UnregisterAll();
};
