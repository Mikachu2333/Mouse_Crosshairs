#include "config.h"

#include <windows.h>

#include <string>
#include <unordered_map>

bool Config::Load(const wchar_t* filename) {
  if (!filename || filename[0] == L'\0') {
    return false;
  }

  const DWORD attributes = GetFileAttributesW(filename);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return false;
  }

  const unsigned int rawGap =
      GetPrivateProfileIntW(L"Crosshair", L"Gap", 0, filename);
  gap = (rawGap > 0) ? static_cast<unsigned int>(rawGap) : 0U;

  const unsigned int rawHorizontalWidth = GetPrivateProfileIntW(
      L"Horizontal", L"Width", static_cast<int>(horizontal.width), filename);
  horizontal.width = (rawHorizontalWidth > 0)
                         ? static_cast<unsigned int>(rawHorizontalWidth)
                         : 0U;
  horizontal.r = color{GetPrivateProfileIntW(
      L"Horizontal", L"R", static_cast<int>(horizontal.r), filename)};
  horizontal.g = color{GetPrivateProfileIntW(
      L"Horizontal", L"G", static_cast<int>(horizontal.g), filename)};
  horizontal.b = color{GetPrivateProfileIntW(
      L"Horizontal", L"B", static_cast<int>(horizontal.b), filename)};
  horizontal.alpha = color{GetPrivateProfileIntW(
      L"Horizontal", L"Alpha", static_cast<int>(horizontal.alpha), filename)};

  const unsigned int rawVerticalWidth = GetPrivateProfileIntW(
      L"Vertical", L"Width", static_cast<int>(vertical.width), filename);
  vertical.width =
      (rawVerticalWidth > 0) ? static_cast<unsigned int>(rawVerticalWidth) : 0U;
  vertical.r = color{GetPrivateProfileIntW(
      L"Vertical", L"R", static_cast<int>(vertical.r), filename)};
  vertical.g = color{GetPrivateProfileIntW(
      L"Vertical", L"G", static_cast<int>(vertical.g), filename)};
  vertical.b = color{GetPrivateProfileIntW(
      L"Vertical", L"B", static_cast<int>(vertical.b), filename)};
  vertical.alpha = color{GetPrivateProfileIntW(
      L"Vertical", L"Alpha", static_cast<int>(vertical.alpha), filename)};

  wchar_t hide_vkStr[32] = {};
  GetPrivateProfileStringW(L"Hide_Show_Hotkey", L"VK", L"", hide_vkStr,
                           _countof(hide_vkStr), filename);
  auto vk_result = ParseVK(hide_vkStr, 'h');
  hotkey_h_s.vk = vk_result.first;
  bool is_fn = vk_result.second;

  wchar_t hide_modStr[32] = {};
  GetPrivateProfileStringW(L"Hide_Show_Hotkey", L"Mod", L"", hide_modStr,
                           _countof(hide_modStr), filename);
  hotkey_h_s.mod = ParseMod(hide_modStr, is_fn);

  wchar_t exit_vkStr[32] = {};
  GetPrivateProfileStringW(L"Exit_Hotkey", L"VK", L"", exit_vkStr,
                           _countof(exit_vkStr), filename);
  vk_result = ParseVK(exit_vkStr, 'e');
  hotkey_exit.vk = vk_result.first;
  is_fn = vk_result.second;

  wchar_t exit_modStr[32] = {};
  GetPrivateProfileStringW(L"Exit_Hotkey", L"Mod", L"", exit_modStr,
                           _countof(exit_modStr), filename);
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

unsigned int Config::ParseMod(const wchar_t* unchecked_str, bool only_fn) {
  unsigned int mod = 0;
  if (only_fn) {
    return mod;
  }
  std::wstring modStrLower = (unchecked_str != nullptr) ? unchecked_str : L"";
  for (wchar_t& c : modStrLower) c = static_cast<wchar_t>(towlower(c));
  if (modStrLower.find(L"ctrl") != std::wstring::npos ||
      modStrLower.find(L"control") != std::wstring::npos) {
    mod |= MOD_CONTROL;
  }
  if (modStrLower.find(L"alt") != std::wstring::npos) mod |= MOD_ALT;
  if (modStrLower.find(L"win") != std::wstring::npos) mod |= MOD_WIN;
  if (modStrLower.find(L"shift") != std::wstring::npos) mod |= MOD_SHIFT;
  if (mod < 1 || mod > (MOD_CONTROL | MOD_SHIFT | MOD_WIN | MOD_ALT)) {
    mod = MOD_CONTROL | MOD_WIN | MOD_ALT;
  }
  return mod;
}

std::pair<unsigned int, bool> Config::ParseVK(const wchar_t* str, char mode) {
  static const std::unordered_map<std::wstring, unsigned int> vkMap = {
      {L"xbutton1", VK_XBUTTON1},
      {L"xbutton2", VK_XBUTTON2},
      {L"backspace", VK_BACK},
      {L"tab", VK_TAB},
      {L"space", VK_SPACE},
      {L"enter", VK_RETURN},
      {L"delete", VK_DELETE},
      {L"insert", VK_INSERT},
      {L"esc", VK_ESCAPE},
      {L"f1", VK_F1},
      {L"f2", VK_F2},
      {L"f3", VK_F3},
      {L"f4", VK_F4},
      {L"f5", VK_F5},
      {L"f6", VK_F6},
      {L"f7", VK_F7},
      {L"f8", VK_F8},
      {L"f9", VK_F9},
      {L"f10", VK_F10},
      {L"f11", VK_F11},
      {L"f12", VK_F12},
      {L"f13", VK_F13},
      {L"f14", VK_F14},
      {L"f15", VK_F15},
      {L"f16", VK_F16},
      {L"f17", VK_F17},
      {L"f18", VK_F18},
      {L"f19", VK_F19},
      {L"f20", VK_F20},
      {L"f21", VK_F21},
      {L"f22", VK_F22},
      {L"f23", VK_F23},
      {L"f24", VK_F24},
      {L"ctrl", VK_LCONTROL},
      {L"control", VK_LCONTROL},
      {L"lctrl", VK_LCONTROL},
      {L"rctrl", VK_RCONTROL},
      {L"shift", VK_LSHIFT},
      {L"lshift", VK_LSHIFT},
      {L"rshift", VK_RSHIFT},
      {L"win", VK_LWIN},
      {L"lwin", VK_LWIN},
      {L"rwin", VK_RWIN},
      {L"alt", VK_LMENU},
      {L"lalt", VK_LMENU},
      {L"ralt", VK_RMENU},
      {L"numpad0", VK_NUMPAD0},
      {L"numpad1", VK_NUMPAD1},
      {L"numpad2", VK_NUMPAD2},
      {L"numpad3", VK_NUMPAD3},
      {L"numpad4", VK_NUMPAD4},
      {L"numpad5", VK_NUMPAD5},
      {L"numpad6", VK_NUMPAD6},
      {L"numpad7", VK_NUMPAD7},
      {L"numpad8", VK_NUMPAD8},
      {L"numpad9", VK_NUMPAD9},
      {L"0", VK_0},
      {L"1", VK_1},
      {L"2", VK_2},
      {L"3", VK_3},
      {L"4", VK_4},
      {L"5", VK_5},
      {L"6", VK_6},
      {L"7", VK_7},
      {L"8", VK_8},
      {L"9", VK_9},
      {L"a", VK_A},
      {L"b", VK_B},
      {L"c", VK_C},
      {L"d", VK_D},
      {L"e", VK_E},
      {L"f", VK_F},
      {L"g", VK_G},
      {L"h", VK_H},
      {L"i", VK_I},
      {L"j", VK_J},
      {L"k", VK_K},
      {L"l", VK_L},
      {L"m", VK_M},
      {L"n", VK_N},
      {L"o", VK_O},
      {L"p", VK_P},
      {L"q", VK_Q},
      {L"r", VK_R},
      {L"s", VK_S},
      {L"t", VK_T},
      {L"u", VK_U},
      {L"v", VK_V},
      {L"w", VK_W},
      {L"x", VK_X},
      {L"y", VK_Y},
      {L"z", VK_Z},
      {L"+", VK_OEM_PLUS},
      {L"=", VK_OEM_PLUS},
      {L"plus", VK_OEM_PLUS},
      {L",", VK_OEM_COMMA},
      {L"<", VK_OEM_COMMA},
      {L"comma", VK_OEM_COMMA},
      {L"_", VK_OEM_MINUS},
      {L"-", VK_OEM_MINUS},
      {L"minus", VK_OEM_MINUS},
      {L".", VK_OEM_PERIOD},
      {L">", VK_OEM_PERIOD},
      {L"period", VK_OEM_PERIOD},
      {L"/", VK_OEM_2},
      {L"?", VK_OEM_2},
      {L"slash", VK_OEM_2},
      {L";", VK_OEM_1},
      {L":", VK_OEM_1},
      {L"semicolon", VK_OEM_1},
      {L"'", VK_OEM_7},
      {L"\"", VK_OEM_7},
      {L"apostrophe", VK_OEM_7},
      {L"[", VK_OEM_4},
      {L"{", VK_OEM_4},
      {L"bracketleft", VK_OEM_4},
      {L"|", VK_OEM_5},
      {L"\\", VK_OEM_5},
      {L"backslash", VK_OEM_5},
      {L"]", VK_OEM_6},
      {L"}", VK_OEM_6},
      {L"bracketright", VK_OEM_6},
      {L"`", VK_OEM_3},
      {L"~", VK_OEM_3},
      {L"grave", VK_OEM_3}};
  std::wstring key = (str != nullptr) ? str : L"";
  std::transform(key.begin(), key.end(), key.begin(), towlower);
  if (key.length() > 3 && key.substr(0, 3) == L"vk_") key = key.substr(3);
  if (const auto item = vkMap.find(key); item != vkMap.end()) {
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
