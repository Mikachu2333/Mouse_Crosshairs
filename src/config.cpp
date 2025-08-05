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
    GetPrivateProfileStringA("Hotkey", "Mod", "", modStr, sizeof(modStr), filename);
    hotkey.mod = 0;
    std::string modStrLower = modStr;
    for (auto &c: modStrLower) c = tolower(c);
    if (modStrLower.find("ctrl") != std::string::npos) hotkey.mod |= MOD_CONTROL;
    if (modStrLower.find("alt") != std::string::npos) hotkey.mod |= MOD_ALT;
    if (modStrLower.find("win") != std::string::npos) hotkey.mod |= MOD_WIN;
    if (modStrLower.find("shift") != std::string::npos) hotkey.mod |= MOD_SHIFT;
    if (hotkey.mod < 1 || hotkey.mod > MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)
        hotkey.mod = MOD_CONTROL | MOD_WIN | MOD_ALT;

    char vkStr[64] = {};
    GetPrivateProfileStringA("Hotkey", "VK", "", vkStr, sizeof(vkStr), filename);
    hotkey.vk = ParseVK(vkStr);

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
