#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>
#include <windows.h>

struct ColorChannel {
  std::uint8_t value = 0;

  constexpr ColorChannel() = default;
  constexpr explicit ColorChannel(int input)
      : value(static_cast<std::uint8_t>(std::clamp(input, 0, 255))) {}
};

struct LineConfig {
  unsigned int width = 4;
  ColorChannel r{233};
  ColorChannel g{233};
  ColorChannel b{233};
  ColorChannel alpha{199};

  void Clamp() noexcept { width = std::clamp(width, 1U, 200U); }
};

struct HotkeyConfig {
  unsigned int mod = 0;
  unsigned int vk = 0;

  void Clamp(char mode) noexcept;
};

struct Config {
  unsigned int gap = 0;
  LineConfig horizontal;
  LineConfig vertical;
  HotkeyConfig hotkey_h_s;
  HotkeyConfig hotkey_exit;

  // 仅在整个文件可读取且所有显式配置值有效时提交新配置。
  [[nodiscard]] bool Load(const wchar_t* filename);

  static unsigned int ParseMod(const wchar_t* text, bool function_key);
  static std::pair<unsigned int, bool> ParseVK(const wchar_t* text, char mode);

  void ClampAll() noexcept;
};
