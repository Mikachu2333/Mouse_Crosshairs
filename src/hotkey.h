#pragma once

#include <windows.h>

#include "config.h"

inline constexpr int kToggleHotkeyId = 0x4D43;
inline constexpr int kExitHotkeyId = 0x4D44;

class HotkeyManager {
 public:
  [[nodiscard]] static bool Register(const HotkeyConfig& toggle,
                                     const HotkeyConfig& exit);
  static void UnregisterAll() noexcept;
};
