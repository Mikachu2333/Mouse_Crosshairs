#include "config.h"

// 从 INI 文件加载配置
bool Config::Load(const char *filename) {
    // 加载横线配置
    horizontal.width = GetPrivateProfileIntA("Horizontal", "Width", horizontal.width, filename);
    horizontal.r = GetPrivateProfileIntA("Horizontal", "R", horizontal.r, filename);
    horizontal.g = GetPrivateProfileIntA("Horizontal", "G", horizontal.g, filename);
    horizontal.b = GetPrivateProfileIntA("Horizontal", "B", horizontal.b, filename);
    horizontal.alpha = GetPrivateProfileIntA("Horizontal", "Alpha", horizontal.alpha, filename);

    // 加载竖线配置
    vertical.width = GetPrivateProfileIntA("Vertical", "Width", vertical.width, filename);
    vertical.r = GetPrivateProfileIntA("Vertical", "R", vertical.r, filename);
    vertical.g = GetPrivateProfileIntA("Vertical", "G", vertical.g, filename);
    vertical.b = GetPrivateProfileIntA("Vertical", "B", vertical.b, filename);
    vertical.alpha = GetPrivateProfileIntA("Vertical", "Alpha", vertical.alpha, filename);

    // 加载显示/隐藏热键配置
    char modStr[64] = {};
    GetPrivateProfileStringA("Hide_Show_Hotkey", "Mod", "", modStr, sizeof(modStr), filename);
    hotkey_h_s.mod = 0;
    std::string modStrLower = modStr;
    for (char &c: modStrLower) c = tolower(c);
    // 解析修饰键组合
    if (modStrLower.find("ctrl") != std::string::npos) hotkey_h_s.mod |= MOD_CONTROL;
    if (modStrLower.find("alt") != std::string::npos) hotkey_h_s.mod |= MOD_ALT;
    if (modStrLower.find("win") != std::string::npos) hotkey_h_s.mod |= MOD_WIN;
    if (modStrLower.find("shift") != std::string::npos) hotkey_h_s.mod |= MOD_SHIFT;
    if (hotkey_h_s.mod < 1 || hotkey_h_s.mod > MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)
        hotkey_h_s.mod = MOD_CONTROL | MOD_WIN | MOD_ALT;

    char vkStr[64] = {};
    GetPrivateProfileStringA("Hide_Show_Hotkey", "VK", "", vkStr, sizeof(vkStr), filename);
    hotkey_h_s.vk = ParseVK(vkStr, 'h');

    // 加载退出热键配置
    char exit_modStr[64] = {};
    GetPrivateProfileStringA("Exit_Hotkey", "Mod", "", exit_modStr, sizeof(exit_modStr), filename);
    hotkey_exit.mod = 0;
    std::string exit_modStrLower = exit_modStr;
    for (char &c: exit_modStrLower) c = tolower(c);
    // 解析修饰键组合
    if (exit_modStrLower.find("ctrl") != std::string::npos) hotkey_exit.mod |= MOD_CONTROL;
    if (exit_modStrLower.find("alt") != std::string::npos) hotkey_exit.mod |= MOD_ALT;
    if (exit_modStrLower.find("win") != std::string::npos) hotkey_exit.mod |= MOD_WIN;
    if (exit_modStrLower.find("shift") != std::string::npos) hotkey_exit.mod |= MOD_SHIFT;
    if (hotkey_exit.mod < 1 || hotkey_exit.mod > MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)
        hotkey_exit.mod = MOD_CONTROL | MOD_WIN | MOD_ALT;

    char exit_vkStr[64] = {};
    GetPrivateProfileStringA("Exit_Hotkey", "VK", "", exit_vkStr, sizeof(exit_vkStr), filename);
    hotkey_exit.vk = ParseVK(exit_vkStr, 'e');

    ClampAll();
    return true;
}

// 根据屏幕尺寸自动设置十字准星长度
void Config::AutoSetLength() {
    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 设置为全屏幕宽度和高度
    horizontal.length = screenWidth;
    vertical.length = screenHeight;
}
