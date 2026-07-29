#pragma once

#include <string>

inline constexpr char kDefaultIni[] = R"([Crosshair]
# The gap (empty space) around the mouse cursor; valid range: 0-400
Gap = 20

[Horizontal]
# Valid range: 1-200
Width = 2
# R, G, B, Alpha: 0-255
R = 255
G = 0
B = 0
Alpha = 255

[Vertical]
# Valid range: 1-200
Width = 2
# R, G, B, Alpha: 0-255
R = 255
G = 0
B = 0
Alpha = 255

[Hide_Show_Hotkey]
# Supported modifiers: Ctrl, Alt, Win, Shift (comma-separated)
# F1-F24 may be used without modifiers by leaving Mod empty.
# Invalid modifiers fall back to Ctrl+Alt+Win.
Mod = Ctrl,Alt,Win

# Letters A-Z; numbers 0-9/Numpad0-Numpad9; F1-F24;
# Space, Tab, Backspace, Escape, Enter, Delete, Insert; XButton1/2;
# and common OEM punctuation keys are supported.
# An invalid value falls back to H.
VK = H

[Exit_Hotkey]
# Same rules as Hide_Show_Hotkey. An invalid modifier falls back to
# Ctrl+Alt+Win; an invalid key falls back to E.
Mod = Ctrl,Alt,Win
VK = E
)";

[[nodiscard]] std::wstring GetConfigPath();
[[nodiscard]] bool EnsureConfigExists(const std::wstring& path);
