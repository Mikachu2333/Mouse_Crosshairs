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

// 虚拟键码枚举
enum VKs {
  VK_0 = 0x30,
  VK_1 = 0x31,
  VK_2 = 0x32,
  VK_3 = 0x33,
  VK_4 = 0x34,
  VK_5 = 0x35,
  VK_6 = 0x36,
  VK_7 = 0x37,
  VK_8 = 0x38,
  VK_9 = 0x39,
  VK_A = 0x41,
  VK_B = 0x42,
  VK_C = 0x43,
  VK_D = 0x44,
  VK_E = 0x45,
  VK_F = 0x46,
  VK_G = 0x47,
  VK_H = 0x48,
  VK_I = 0x49,
  VK_J = 0x4A,
  VK_K = 0x4B,
  VK_L = 0x4C,
  VK_M = 0x4D,
  VK_N = 0x4E,
  VK_O = 0x4F,
  VK_P = 0x50,
  VK_Q = 0x51,
  VK_R = 0x52,
  VK_S = 0x53,
  VK_T = 0x54,
  VK_U = 0x55,
  VK_V = 0x56,
  VK_W = 0x57,
  VK_X = 0x58,
  VK_Y = 0x59,
  VK_Z = 0x5A,
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

  // 限制值在有效范围内
  void Clamp_VK_MOD(char mode);
};

// 主配置类
struct Config {
  unsigned int gap = 0;      // 鼠标中心留出的空隙大小
  LineConfig horizontal;     // 水平线配置
  LineConfig vertical;       // 垂直线配置
  HotkeyConfig hotkey_h_s;   // 显示/隐藏热键
  HotkeyConfig hotkey_exit;  // 退出热键

  // 从文件加载配置
  bool Load(const char* filename);

  // 根据屏幕尺寸自动设置线条长度
  void AutoSetLength();

  // 解析Mod键字符串
  static unsigned int ParseMod(const char* unchecked_str, bool only_fn);

  // 解析虚拟键码字符串
  static std::pair<unsigned int, bool> ParseVK(const char* str, char mode);

  // 限制所有配置值在有效范围内
  void ClampAll() {
    if (gap > 400) gap = 0;  // 防止越界
    horizontal.Clamp();
    vertical.Clamp();
    hotkey_h_s.Clamp_VK_MOD('h');
    hotkey_exit.Clamp_VK_MOD('e');
  }
};
