#include "config.h"

#include <windows.h>

#include <string>
#include <unordered_map>

// 从 INI 文件加载配置
bool Config::Load(const char* filename) {
  if (!filename || filename[0] == '\0') {
    return false;
  }

  const DWORD attributes = GetFileAttributesA(filename);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return false;
  }

  // 读取 [Crosshair] 区段中的 Gap 配置
  const unsigned int rawGap =
      GetPrivateProfileIntA("Crosshair", "Gap", 0, filename);
  gap = (rawGap > 0) ? static_cast<unsigned int>(rawGap) : 0U;

  // 加载横线配置
  const unsigned int rawHorizontalWidth = GetPrivateProfileIntA(
      "Horizontal", "Width", static_cast<int>(horizontal.width), filename);
  horizontal.width = (rawHorizontalWidth > 0)
                         ? static_cast<unsigned int>(rawHorizontalWidth)
                         : 0U;
  horizontal.r = color{GetPrivateProfileIntA(
      "Horizontal", "R", static_cast<int>(horizontal.r), filename)};
  horizontal.g = color{GetPrivateProfileIntA(
      "Horizontal", "G", static_cast<int>(horizontal.g), filename)};
  horizontal.b = color{GetPrivateProfileIntA(
      "Horizontal", "B", static_cast<int>(horizontal.b), filename)};
  horizontal.alpha = color{GetPrivateProfileIntA(
      "Horizontal", "Alpha", static_cast<int>(horizontal.alpha), filename)};

  // 加载竖线配置
  const unsigned int rawVerticalWidth = GetPrivateProfileIntA(
      "Vertical", "Width", static_cast<int>(vertical.width), filename);
  vertical.width =
      (rawVerticalWidth > 0) ? static_cast<unsigned int>(rawVerticalWidth) : 0U;
  vertical.r = color{GetPrivateProfileIntA(
      "Vertical", "R", static_cast<int>(vertical.r), filename)};
  vertical.g = color{GetPrivateProfileIntA(
      "Vertical", "G", static_cast<int>(vertical.g), filename)};
  vertical.b = color{GetPrivateProfileIntA(
      "Vertical", "B", static_cast<int>(vertical.b), filename)};
  vertical.alpha = color{GetPrivateProfileIntA(
      "Vertical", "Alpha", static_cast<int>(vertical.alpha), filename)};

  // 加载显示/隐藏热键配置
  char hide_vkStr[32] = {};
  GetPrivateProfileStringA("Hide_Show_Hotkey", "VK", "", hide_vkStr,
                           sizeof(hide_vkStr), filename);
  auto vk_result = ParseVK(hide_vkStr, 'h');
  hotkey_h_s.vk = vk_result.first;
  bool is_fn = vk_result.second;

  char hide_modStr[32] = {};
  GetPrivateProfileStringA("Hide_Show_Hotkey", "Mod", "", hide_modStr,
                           sizeof(hide_modStr), filename);
  hotkey_h_s.mod = ParseMod(hide_modStr, is_fn);

  // 加载退出热键配置
  char exit_vkStr[32] = {};
  GetPrivateProfileStringA("Exit_Hotkey", "VK", "", exit_vkStr,
                           sizeof(exit_vkStr), filename);
  vk_result = ParseVK(exit_vkStr, 'e');
  hotkey_exit.vk = vk_result.first;
  is_fn = vk_result.second;

  char exit_modStr[32] = {};
  GetPrivateProfileStringA("Exit_Hotkey", "Mod", "", exit_modStr,
                           sizeof(exit_modStr), filename);
  hotkey_exit.mod = ParseMod(exit_modStr, is_fn);

  ClampAll();
  return true;
}

// 根据屏幕尺寸自动设置十字准星长度
void Config::AutoSetLength() {
  // 获取虚拟屏幕尺寸（包含所有监视器）
  const int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  const int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  // 设置为虚拟屏幕宽度和高度，确保在多屏环境下正常工作
  horizontal.length =
      (screenWidth > 0) ? static_cast<unsigned int>(screenWidth) : 1U;
  vertical.length =
      (screenHeight > 0) ? static_cast<unsigned int>(screenHeight) : 1U;
}

void HotkeyConfig::Clamp_VK_MOD(char mode) {
  bool isFn = (vk >= static_cast<unsigned int>(VK_F1) &&
               vk <= static_cast<unsigned int>(VK_F24));
  if (isFn && mod == 0) {
    // 允许单FN键且无修饰键
  } else if (mod < 1 || mod > (MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)) {
    mod = MOD_CONTROL | MOD_WIN | MOD_ALT;
  }
  const bool valid = isFn ||
                     (vk >= static_cast<unsigned int>(VK_XBUTTON1) &&
                      vk <= static_cast<unsigned int>(VK_XBUTTON2)) ||
                     (vk >= static_cast<unsigned int>(VK_BACK) &&
                      vk <= static_cast<unsigned int>(VK_TAB)) ||
                     vk == static_cast<unsigned int>(VK_SPACE) ||
                     (vk >= static_cast<unsigned int>(VK_NUMPAD0) &&
                      vk <= static_cast<unsigned int>(VK_NUMPAD9)) ||
                     (vk >= static_cast<unsigned int>(VK_0) &&
                      vk <= static_cast<unsigned int>(VK_9)) ||
                     (vk >= static_cast<unsigned int>(VK_A) &&
                      vk <= static_cast<unsigned int>(VK_Z));
  switch (mode) {
    case 'h':
      if (!valid) vk = VK_H;
      break;
    case 'e':
      if (!valid) vk = VK_E;
      break;
    default:
      if (!valid) vk = VK_NONAME;
      break;
  }
}

unsigned int Config::ParseMod(const char* unchecked_str, bool only_fn) {
  unsigned int mod = 0;
  if (only_fn) {
    return mod;
  }
  std::string modStrLower = (unchecked_str != nullptr) ? unchecked_str : "";
  for (char& c : modStrLower)
    c = static_cast<char>(
        tolower(static_cast<unsigned char>(c)));  // avoid narrowing
  // 解析修饰键组合
  if (modStrLower.find("ctrl") != std::string::npos ||
      modStrLower.find("control") != std::string::npos) {
    mod |= MOD_CONTROL;
  }
  if (modStrLower.find("alt") != std::string::npos) mod |= MOD_ALT;
  if (modStrLower.find("win") != std::string::npos) mod |= MOD_WIN;
  if (modStrLower.find("shift") != std::string::npos) mod |= MOD_SHIFT;
  if (mod < 1 || mod > (MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)) {
    mod = MOD_CONTROL | MOD_WIN | MOD_ALT;
  }
  return mod;
}

std::pair<unsigned int, bool> Config::ParseVK(const char* str, char mode) {
  static const std::unordered_map<std::string, unsigned int> vkMap = {
      {"xbutton1", VK_XBUTTON1},
      {"xbutton2", VK_XBUTTON2},
      {"backspace", VK_BACK},
      {"tab", VK_TAB},
      {"space", VK_SPACE},
      {"enter", VK_RETURN},
      {"delete", VK_DELETE},
      {"insert", VK_INSERT},
      {"esc", VK_ESCAPE},
      {"f1", VK_F1},
      {"f2", VK_F2},
      {"f3", VK_F3},
      {"f4", VK_F4},
      {"f5", VK_F5},
      {"f6", VK_F6},
      {"f7", VK_F7},
      {"f8", VK_F8},
      {"f9", VK_F9},
      {"f10", VK_F10},
      {"f11", VK_F11},
      {"f12", VK_F12},
      {"f13", VK_F13},
      {"f14", VK_F14},
      {"f15", VK_F15},
      {"f16", VK_F16},
      {"f17", VK_F17},
      {"f18", VK_F18},
      {"f19", VK_F19},
      {"f20", VK_F20},
      {"f21", VK_F21},
      {"f22", VK_F22},
      {"f23", VK_F23},
      {"f24", VK_F24},
      {"ctrl", VK_LCONTROL},
      {"control", VK_LCONTROL},
      {"lctrl", VK_LCONTROL},
      {"rctrl", VK_RCONTROL},
      {"shift", VK_LSHIFT},
      {"lshift", VK_LSHIFT},
      {"rshift", VK_RSHIFT},
      {"win", VK_LWIN},
      {"lwin", VK_LWIN},
      {"rwin", VK_RWIN},
      {"alt", VK_LMENU},
      {"lalt", VK_LMENU},
      {"ralt", VK_RMENU},
      {"numpad0", VK_NUMPAD0},
      {"numpad1", VK_NUMPAD1},
      {"numpad2", VK_NUMPAD2},
      {"numpad3", VK_NUMPAD3},
      {"numpad4", VK_NUMPAD4},
      {"numpad5", VK_NUMPAD5},
      {"numpad6", VK_NUMPAD6},
      {"numpad7", VK_NUMPAD7},
      {"numpad8", VK_NUMPAD8},
      {"numpad9", VK_NUMPAD9},
      {"0", VK_0},
      {"1", VK_1},
      {"2", VK_2},
      {"3", VK_3},
      {"4", VK_4},
      {"5", VK_5},
      {"6", VK_6},
      {"7", VK_7},
      {"8", VK_8},
      {"9", VK_9},
      {"a", VK_A},
      {"b", VK_B},
      {"c", VK_C},
      {"d", VK_D},
      {"e", VK_E},
      {"f", VK_F},
      {"g", VK_G},
      {"h", VK_H},
      {"i", VK_I},
      {"j", VK_J},
      {"k", VK_K},
      {"l", VK_L},
      {"m", VK_M},
      {"n", VK_N},
      {"o", VK_O},
      {"p", VK_P},
      {"q", VK_Q},
      {"r", VK_R},
      {"s", VK_S},
      {"t", VK_T},
      {"u", VK_U},
      {"v", VK_V},
      {"w", VK_W},
      {"x", VK_X},
      {"y", VK_Y},
      {"z", VK_Z},
      {"+", VK_OEM_PLUS},
      {"=", VK_OEM_PLUS},
      {"plus", VK_OEM_PLUS},
      {",", VK_OEM_COMMA},
      {"<", VK_OEM_COMMA},
      {"comma", VK_OEM_COMMA},
      {"_", VK_OEM_MINUS},
      {"-", VK_OEM_MINUS},
      {"minus", VK_OEM_MINUS},
      {".", VK_OEM_PERIOD},
      {">", VK_OEM_PERIOD},
      {"period", VK_OEM_PERIOD},
      {"/", VK_OEM_2},
      {"?", VK_OEM_2},
      {"slash", VK_OEM_2},
      {";", VK_OEM_1},
      {":", VK_OEM_1},
      {"semicolon", VK_OEM_1},
      {"'", VK_OEM_7},
      {"\"", VK_OEM_7},
      {"apostrophe", VK_OEM_7},
      {"[", VK_OEM_4},
      {"{", VK_OEM_4},
      {"bracketleft", VK_OEM_4},
      {"|", VK_OEM_5},
      {"\\", VK_OEM_5},
      {"backslash", VK_OEM_5},
      {"]", VK_OEM_6},
      {"}", VK_OEM_6},
      {"bracketright", VK_OEM_6},
      {"`", VK_OEM_3},
      {"~", VK_OEM_3},
      {"grave", VK_OEM_3}};
  std::string key = (str != nullptr) ? str : "";
  std::transform(key.begin(), key.end(), key.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (key.length() > 3 && key.substr(0, 3) == "vk_") key = key.substr(3);
  if (const auto item = vkMap.find(key); item != vkMap.end()) {
    // 检查是否为F键系列
    bool isFnKey = (item->second >= static_cast<unsigned int>(VK_F1) &&
                    item->second <= static_cast<unsigned int>(VK_F24));
    return {item->second, isFnKey};
  }
  switch (mode) {
    case 'h':
      return {static_cast<unsigned int>(VK_H), false};
    case 'e':
      return {static_cast<unsigned int>(VK_E), false};
    default:
      return {static_cast<unsigned int>(VK_NONAME), false};
  }
}
