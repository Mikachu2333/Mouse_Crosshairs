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
    if (hotkey.mod < 1 || hotkey.mod > MOD_CONTROL|MOD_SHIFT|MOD_WIN|MOD_ALT) hotkey.mod = MOD_CONTROL|MOD_WIN|MOD_ALT;

    char vkStr[64] = {};
    GetPrivateProfileStringA("Hotkey", "VK", "", vkStr, sizeof(vkStr), filename);
    hotkey.vk = ParseVK(vkStr);

    ClampAll();
    return true;
}

void Config::AutoSetLength() {
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    UINT dpi = 96;
    if (const HMODULE hUser32 = LoadLibraryA("User32.dll")) {
        typedef UINT (WINAPI *GetDpiForSystem_t)();
        if (const auto pGetDpiForSystem = reinterpret_cast<GetDpiForSystem_t>(GetProcAddress(hUser32, "GetDpiForSystem"))) {
            dpi = pGetDpiForSystem();
        } else {
            const HDC hdc = GetDC(nullptr);
            dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(nullptr, hdc);
        }
        FreeLibrary(hUser32);
    }

    const double scale = dpi / 96.0;
    // 横线长度基于屏幕宽度
    horizontal.length = static_cast<int>(screenWidth * scale);
    // 竖线长度基于屏幕高度
    vertical.length = static_cast<int>(screenHeight * scale);
}
