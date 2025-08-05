#pragma once
#include <string>

static auto DEFAULT_INI = R"([Horizontal]
Width = 2
# R, G, B, Alpha: 0-255
R = 255
G = 0
B = 0
Alpha = 255

[Vertical]
Width = 2
# R, G, B, Alpha: 0-255
R = 255
G = 0
B = 0
Alpha = 255

[Hotkey]
# Mod Support
#Ctrl, Alt, Win, Shift
Mod = Ctrl,Alt,Win
# Key Support
# A-Z, 0-9, F1-F12, Numpad0-Numpad9
# Space, Tab, Backspace
# VK_XBUTTON1, VK_XBUTTON2
VK = H
)";

// 获取配置文件绝对路径（与exe同目录）
std::string get_config_path();

// 如果配置文件不存在则创建
void ensure_config_exists(const std::string &path);
