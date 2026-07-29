#include "./hotkey.h"

bool HotkeyManager::Register(const HotkeyConfig& toggle,
                             const HotkeyConfig& exit) {
  if (toggle.mod == exit.mod && toggle.vk == exit.vk) return false;

  UnregisterAll();
  if (!RegisterHotKey(nullptr, kToggleHotkeyId, toggle.mod | MOD_NOREPEAT,
                      toggle.vk)) {
    return false;
  }
  if (!RegisterHotKey(nullptr, kExitHotkeyId, exit.mod | MOD_NOREPEAT,
                      exit.vk)) {
    UnregisterHotKey(nullptr, kToggleHotkeyId);
    return false;
  }
  return true;
}

void HotkeyManager::UnregisterAll() noexcept {
  UnregisterHotKey(nullptr, kToggleHotkeyId);
  UnregisterHotKey(nullptr, kExitHotkeyId);
}
