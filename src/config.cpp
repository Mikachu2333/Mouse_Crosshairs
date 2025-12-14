#include "config.h"

#include <minwindef.h>

// 从 INI 文件加载配置
bool Config::Load(const char* filename) {
  // 加载横线配置
  horizontal.width = GetPrivateProfileIntA(
      "Horizontal", "Width", static_cast<INT>(horizontal.width), filename);
  horizontal.r = color{GetPrivateProfileIntA(
      "Horizontal", "R", static_cast<INT>(horizontal.r), filename)};
  horizontal.g = color{GetPrivateProfileIntA(
      "Horizontal", "G", static_cast<INT>(horizontal.g), filename)};
  horizontal.b = color{GetPrivateProfileIntA(
      "Horizontal", "B", static_cast<INT>(horizontal.b), filename)};
  horizontal.alpha = color{GetPrivateProfileIntA(
      "Horizontal", "Alpha", static_cast<INT>(horizontal.alpha), filename)};

  // 加载竖线配置
  vertical.width =
      GetPrivateProfileIntA("Vertical", "Width", (int)vertical.width, filename);
  vertical.r =
      color{GetPrivateProfileIntA("Vertical", "R", (int)vertical.r, filename)};
  vertical.g =
      color{GetPrivateProfileIntA("Vertical", "G", (int)vertical.g, filename)};
  vertical.b =
      color{GetPrivateProfileIntA("Vertical", "B", (int)vertical.b, filename)};
  vertical.alpha = color{GetPrivateProfileIntA("Vertical", "Alpha",
                                               (int)vertical.alpha, filename)};

  // 加载显示/隐藏热键配置
  char hide_modStr[32] = {};
  GetPrivateProfileStringA("Hide_Show_Hotkey", "Mod", "", hide_modStr,
                           sizeof(hide_modStr), filename);
  hotkey_h_s.mod = ParseMod(hide_modStr);

  char hide_vkStr[32] = {};
  GetPrivateProfileStringA("Hide_Show_Hotkey", "VK", "", hide_vkStr,
                           sizeof(hide_vkStr), filename);
  hotkey_h_s.vk = ParseVK(hide_vkStr, 'h');

  // 加载退出热键配置
  char exit_modStr[32] = {};
  GetPrivateProfileStringA("Exit_Hotkey", "Mod", "", exit_modStr,
                           sizeof(exit_modStr), filename);
  hotkey_exit.mod = ParseMod(exit_modStr);

  char exit_vkStr[32] = {};
  GetPrivateProfileStringA("Exit_Hotkey", "VK", "", exit_vkStr,
                           sizeof(exit_vkStr), filename);
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
