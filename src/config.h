#pragma once

#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string>
#include <windows.h>

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
    unsigned int length = 2222; // 线条长度
    unsigned int width = 21; // 线条宽度
    unsigned int r = 233, g = 233, b = 233; // RGB 颜色值
    unsigned int alpha = 199; // 透明度

    // 限制值在有效范围内
    void Clamp() {
        if (length < 1) length = 1;
        if (width < 1) width = 1;
        if (width > 100) width = 20;
        r = std::clamp(r, 0u, 255u);
        g = std::clamp(g, 0u, 255u);
        b = std::clamp(b, 0u, 255u);
        alpha = std::clamp(alpha, 0u, 255u);
    }
};

// 热键配置
struct HotkeyConfig {
    unsigned int mod = MOD_WIN | MOD_CONTROL | MOD_ALT; // 修饰键组合
    unsigned int vk = VK_H; // 虚拟键码

    // 限制值在有效范围内
    void Clamp(char mode) {
        if (mod < 1 || mod > MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT) mod = MOD_CONTROL | MOD_WIN | MOD_ALT;
        const bool valid =
                (vk >= VK_XBUTTON1 && vk <= VK_XBUTTON2) ||
                (vk >= VK_BACK && vk <= VK_TAB) ||
                vk == VK_SPACE ||
                (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD5) ||
                (vk >= VK_0 && vk <= VK_9) ||
                (vk >= VK_A && vk <= VK_Z);
        switch (mode) {
            case 'h':
                if (!valid) vk = VK_H;
            case 'e':
                if (!valid) vk = VK_ESCAPE;
            default:
                if (!valid) vk = VK_NONAME;
        }
    }
};

// 主配置类
struct Config {
    LineConfig horizontal; // 水平线配置
    LineConfig vertical; // 垂直线配置
    HotkeyConfig hotkey_h_s; // 显示/隐藏热键
    HotkeyConfig hotkey_exit; // 退出热键

    // 从文件加载配置
    bool Load(const char *filename);

    // 根据屏幕尺寸自动设置线条长度
    void AutoSetLength();

    // 解析虚拟键码字符串
    static int ParseVK(const char *str, char mode) {
        static const std::unordered_map<std::string, int> vkMap = {
            {"xbutton1", VK_XBUTTON1}, {"xbutton2", VK_XBUTTON2}, {"backspace", VK_BACK},
            {"tab", VK_TAB}, {"space", VK_SPACE}, {"numpad0", VK_NUMPAD0},
            {"numpad1", VK_NUMPAD1}, {"numpad2", VK_NUMPAD2}, {"numpad3", VK_NUMPAD3},
            {"numpad4", VK_NUMPAD4}, {"numpad5", VK_NUMPAD5}, {"numpad6", VK_NUMPAD6},
            {"numpad7", VK_NUMPAD7}, {"numpad8", VK_NUMPAD8}, {"numpad9", VK_NUMPAD9},
            {"0", VK_0}, {"1", VK_1}, {"2", VK_2}, {"3", VK_3}, {"4", VK_4},
            {"5", VK_5}, {"6", VK_6}, {"7", VK_7}, {"8", VK_8}, {"9", VK_9},
            {"a", VK_A}, {"b", VK_B}, {"c", VK_C}, {"d", VK_D}, {"e", VK_E},
            {"f", VK_F}, {"g", VK_G}, {"h", VK_H}, {"i", VK_I}, {"j", VK_J},
            {"k", VK_K}, {"l", VK_L}, {"m", VK_M}, {"n", VK_N}, {"o", VK_O},
            {"p", VK_P}, {"q", VK_Q}, {"r", VK_R}, {"s", VK_S}, {"t", VK_T},
            {"u", VK_U}, {"v", VK_V}, {"w", VK_W}, {"x", VK_X}, {"y", VK_Y},
            {"z", VK_Z}
        };
        std::string key = str;
        // ReSharper disable once CppUseRangeAlgorithm
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
        if (key.length() > 3 && key.substr(0, 3) == "vk_") key = key.substr(3);
        if (const auto item = vkMap.find(key); item != vkMap.end()) return item->second;
        switch (mode) {
            case 'h': return VK_H;
            case 'e': return VK_ESCAPE;
            default: return VK_NONAME;
        }
    }

    // 限制所有配置值在有效范围内
    void ClampAll() {
        horizontal.Clamp();
        vertical.Clamp();
        hotkey_h_s.Clamp('h');
        hotkey_exit.Clamp('e');
    }
};
