#include "config.h"

bool Config::Load(const char *filename) {
    // 横线
    horizontal.width = GetPrivateProfileIntA("Horizontal", "Width", horizontal.width, filename);
    horizontal.r = GetPrivateProfileIntA("Horizontal", "R", horizontal.r, filename);
    horizontal.g = GetPrivateProfileIntA("Horizontal", "G", horizontal.g, filename);
    horizontal.b = GetPrivateProfileIntA("Horizontal", "B", horizontal.b, filename);
    horizontal.alpha = GetPrivateProfileIntA("Horizontal", "Alpha", horizontal.alpha, filename);

    // 竖线
    vertical.width = GetPrivateProfileIntA("Vertical", "Width", vertical.width, filename);
    vertical.r = GetPrivateProfileIntA("Vertical", "R", vertical.r, filename);
    vertical.g = GetPrivateProfileIntA("Vertical", "G", vertical.g, filename);
    vertical.b = GetPrivateProfileIntA("Vertical", "B", vertical.b, filename);
    vertical.alpha = GetPrivateProfileIntA("Vertical", "Alpha", vertical.alpha, filename);

    // 快捷键
    char modStr[64] = {};
    GetPrivateProfileStringA("Hide_Show_Hotkey", "Mod", "", modStr, sizeof(modStr), filename);
    hotkey_h_s.mod = 0;
    std::string modStrLower = modStr;
    for (auto &c: modStrLower) c = tolower(c);
    if (modStrLower.find("ctrl") != std::string::npos) hotkey_h_s.mod |= MOD_CONTROL;
    if (modStrLower.find("alt") != std::string::npos) hotkey_h_s.mod |= MOD_ALT;
    if (modStrLower.find("win") != std::string::npos) hotkey_h_s.mod |= MOD_WIN;
    if (modStrLower.find("shift") != std::string::npos) hotkey_h_s.mod |= MOD_SHIFT;
    if (hotkey_h_s.mod < 1 || hotkey_h_s.mod > MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)
        hotkey_h_s.mod = MOD_CONTROL | MOD_WIN | MOD_ALT;

    char vkStr[64] = {};
    GetPrivateProfileStringA("Hide_Show_Hotkey", "VK", "", vkStr, sizeof(vkStr), filename);
    hotkey_h_s.vk = ParseVK(vkStr, 'h');

    // 快捷键
    char exit_modStr[64] = {};
    GetPrivateProfileStringA("Exit_Hotkey", "Mod", "", exit_modStr, sizeof(exit_modStr), filename);
    hotkey_exit.mod = 0;
    std::string exit_modStrLower = exit_modStr;
    for (auto &c: exit_modStrLower) c = tolower(c);
    if (exit_modStrLower.find("ctrl") != std::string::npos) hotkey_exit.mod |= MOD_CONTROL;
    if (exit_modStrLower.find("alt") != std::string::npos) hotkey_exit.mod |= MOD_ALT;
    if (exit_modStrLower.find("win") != std::string::npos) hotkey_exit.mod |= MOD_WIN;
    if (exit_modStrLower.find("shift") != std::string::npos) hotkey_exit.mod |= MOD_SHIFT;
    if (hotkey_exit.mod < 1 || hotkey_exit.mod > MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)
        hotkey_exit.mod = MOD_CONTROL | MOD_WIN | MOD_ALT;

    char exit_vkStr[64] = {};
    GetPrivateProfileStringA("Exit_Hotkey", "VK", "", vkStr, sizeof(vkStr), filename);
    hotkey_exit.vk = ParseVK(vkStr, 'e');

    ClampAll();
    return true;
}

void Config::AutoSetLength() {
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 简化 DPI 获取逻辑，直接使用屏幕尺寸
    horizontal.length = screenWidth;
    vertical.length = screenHeight;
}
