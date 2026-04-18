#pragma once
#include <string>

inline constexpr const char DEFAULT_INI[] = R"([Crosshair]
# The gap (empty space) around the mouse cursor
Gap = 20

[Horizontal]
# No more than 200
Width = 2
# R, G, B, Alpha: 0-255
R = 255
G = 0
B = 0
Alpha = 255

[Vertical]
# No more than 200
Width = 2
# R, G, B, Alpha: 0-255
R = 255
G = 0
B = 0
Alpha = 255

[Hide_Show_Hotkey]
# Modifiers Support
# Prefix: Ctrl, Alt, Win, Shift (Combine up to 4)
# Note: Fn keys (F1-F24) can be used entirely without any modifiers!
# To use single Fn key, leave Mod blank or empty like: Mod = 
Mod = Ctrl,Alt,Win

# Virtual Key Support
# - Letters: A-Z
# - Numbers: 0-9, Numpad0-Numpad9
# - Functions: F1-F24
# - Specials: Space, Tab, Backspace, Escape, Enter, Delete, Insert
# - Mouse: XButton1, XButton2
VK = H

[Exit_Hotkey]
# Same rule as Hide_Show_Hotkey
Mod = Ctrl,Alt,Win
VK = E
)";

// 获取配置文件绝对路径（与exe同目录）
std::string get_config_path();

// 确保配置文件存在；当不存在时尝试创建默认配置
bool ensure_config_exists(const std::string& path);
