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
    char hide_modStr[32] = {};
    GetPrivateProfileStringA("Hide_Show_Hotkey", "Mod", "", hide_modStr, sizeof(hide_modStr), filename);
    hotkey_h_s.mod = ParseMod(hide_modStr);

    char hide_vkStr[32] = {};
    GetPrivateProfileStringA("Hide_Show_Hotkey", "VK", "", hide_vkStr, sizeof(hide_vkStr), filename);
    hotkey_h_s.vk = ParseVK(hide_vkStr, 'h');

    // 加载退出热键配置
    char exit_modStr[32] = {};
    GetPrivateProfileStringA("Exit_Hotkey", "Mod", "", exit_modStr, sizeof(exit_modStr), filename);
    hotkey_exit.mod = ParseMod(exit_modStr);

    char exit_vkStr[32] = {};
    GetPrivateProfileStringA("Exit_Hotkey", "VK", "", exit_vkStr, sizeof(exit_vkStr), filename);
    hotkey_exit.vk = ParseVK(exit_vkStr, 'e');

    ClampAll();
    return true;
}

// 根据屏幕尺寸自动设置十字准星长度
void Config::AutoSetLength() {
    // 获取虚拟屏幕尺寸（包含所有监视器）
    const int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // 设置为虚拟屏幕宽度和高度，确保在多屏环境下正常工作
    horizontal.length = screenWidth;
    vertical.length = screenHeight;
}
