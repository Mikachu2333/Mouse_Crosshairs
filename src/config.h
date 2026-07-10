#pragma once

#include <windows.h>
#include <algorithm>

struct color {
  unsigned char value = 0;

  constexpr color() = default;
  constexpr explicit color(int v) { set(v); }
  constexpr explicit color(unsigned int v) { set(static_cast<int>(v)); }

  constexpr void set(int v) {
    const int clamped = std::clamp(v, 0, 255);
    value = static_cast<unsigned char>(clamped);
  }

  constexpr void clamp() { set(value); }

  constexpr explicit operator int() const { return value; }
  constexpr explicit operator unsigned int() const { return value; }
  constexpr explicit operator BYTE() const { return value; }

  color& operator=(int v) {
    set(v);
    return *this;
  }
};

// 单条线配置
struct LineConfig {
  unsigned int length = 1920;    // 线条长度
  unsigned int width = 4;        // 线条宽度
  color r{233}, g{233}, b{233};  // RGB 颜色值
  color alpha{199};              // 透明度

  void Clamp() {
    if (length < 1) length = 1;
    if (width < 1) width = 1;
    if (width > 200) width = 200;
    r.clamp();
    g.clamp();
    b.clamp();
    alpha.clamp();
  }
};

// 热键配置
struct HotkeyConfig {
  unsigned int mod = 0;  // 修饰键组合
  unsigned int vk = 0;   // 虚拟键码

  void Clamp_MOD();
  void Clamp_VK(char mode);
  void Clamp_VK_MOD(char mode);
};

// 主配置类
struct Config {
  unsigned int gap = 0;      // 鼠标中心留出的空隙大小
  LineConfig horizontal;     // 水平线配置
  LineConfig vertical;       // 垂直线配置
  HotkeyConfig hotkey_h_s;   // 显示/隐藏热键
  HotkeyConfig hotkey_exit;  // 退出热键

  bool Load(const wchar_t* filename);

  // 根据屏幕尺寸自动设置线条长度
  void AutoSetLength();

  static unsigned int ParseMod(const wchar_t* unchecked_str, bool only_fn);
  static std::pair<unsigned int, bool> ParseVK(const wchar_t* str, char mode);

  // 限制所有配置值在有效范围内
  void ClampAll() {
    if (gap > 400) gap = 0;
    horizontal.Clamp();
    vertical.Clamp();
    hotkey_h_s.Clamp_VK_MOD('h');
    hotkey_exit.Clamp_VK_MOD('e');
  }
};
