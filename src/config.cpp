#include "./config.h"

#include <cwctype>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
std::wstring TrimAndLower(std::wstring value) {
  const auto whitespace = [](wchar_t character) { return iswspace(character); };
  while (!value.empty() && whitespace(value.front())) value.erase(value.begin());
  while (!value.empty() && whitespace(value.back())) value.pop_back();
  for (wchar_t& character : value) {
    character = static_cast<wchar_t>(towlower(character));
  }
  return value;
}

std::optional<std::wstring> ReadString(const wchar_t* section,
                                       const wchar_t* key,
                                       const wchar_t* filename,
                                       bool required = true) {
  constexpr wchar_t kMissing[] = L"{92D52289-BC0B-4E11-9FC6-55B21A223BA7}";
  std::wstring buffer(256, L'\0');
  const DWORD length = GetPrivateProfileStringW(
      section, key, kMissing, buffer.data(), static_cast<DWORD>(buffer.size()),
      filename);
  if (length >= buffer.size() - 1) return std::nullopt;
  buffer.resize(length);
  if (buffer == kMissing) {
    return required ? std::nullopt : std::optional<std::wstring>{L""};
  }
  return TrimAndLower(std::move(buffer));
}

std::optional<unsigned int> ParseUnsigned(std::wstring_view text,
                                          unsigned int minimum,
                                          unsigned int maximum) {
  if (text.empty()) return std::nullopt;
  unsigned long long value = 0;
  for (const wchar_t character : text) {
    if (character < L'0' || character > L'9') return std::nullopt;
    const unsigned int digit = static_cast<unsigned int>(character - L'0');
    if (value > (std::numeric_limits<unsigned int>::max() - digit) / 10ULL) {
      return std::nullopt;
    }
    value = value * 10ULL + digit;
  }
  if (value < minimum || value > maximum) return std::nullopt;
  return static_cast<unsigned int>(value);
}

std::optional<unsigned int> ReadUnsigned(const wchar_t* section,
                                         const wchar_t* key,
                                         const wchar_t* filename,
                                         unsigned int minimum,
                                         unsigned int maximum) {
  const auto value = ReadString(section, key, filename);
  return value ? ParseUnsigned(*value, minimum, maximum) : std::nullopt;
}
}  // namespace

bool Config::Load(const wchar_t* filename) {
  if (filename == nullptr || *filename == L'\0') return false;
  const DWORD attributes = GetFileAttributesW(filename);
  if (attributes == INVALID_FILE_ATTRIBUTES ||
      (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
    return false;
  }

  Config loaded;
  const auto gap_value = ReadUnsigned(L"Crosshair", L"Gap", filename, 0, 400);
  const auto horizontal_width =
      ReadUnsigned(L"Horizontal", L"Width", filename, 1, 200);
  const auto horizontal_r =
      ReadUnsigned(L"Horizontal", L"R", filename, 0, 255);
  const auto horizontal_g =
      ReadUnsigned(L"Horizontal", L"G", filename, 0, 255);
  const auto horizontal_b =
      ReadUnsigned(L"Horizontal", L"B", filename, 0, 255);
  const auto horizontal_alpha =
      ReadUnsigned(L"Horizontal", L"Alpha", filename, 0, 255);
  const auto vertical_width =
      ReadUnsigned(L"Vertical", L"Width", filename, 1, 200);
  const auto vertical_r = ReadUnsigned(L"Vertical", L"R", filename, 0, 255);
  const auto vertical_g = ReadUnsigned(L"Vertical", L"G", filename, 0, 255);
  const auto vertical_b = ReadUnsigned(L"Vertical", L"B", filename, 0, 255);
  const auto vertical_alpha =
      ReadUnsigned(L"Vertical", L"Alpha", filename, 0, 255);
  const auto hide_key = ReadString(L"Hide_Show_Hotkey", L"VK", filename);
  const auto hide_mod =
      ReadString(L"Hide_Show_Hotkey", L"Mod", filename, false);
  const auto exit_key = ReadString(L"Exit_Hotkey", L"VK", filename);
  const auto exit_mod = ReadString(L"Exit_Hotkey", L"Mod", filename, false);

  if (!gap_value || !horizontal_width || !horizontal_r || !horizontal_g ||
      !horizontal_b || !horizontal_alpha || !vertical_width || !vertical_r ||
      !vertical_g || !vertical_b || !vertical_alpha || !hide_key || !hide_mod ||
      !exit_key || !exit_mod) {
    return false;
  }

  loaded.gap = *gap_value;
  loaded.horizontal.width = *horizontal_width;
  loaded.horizontal.r = ColorChannel(static_cast<int>(*horizontal_r));
  loaded.horizontal.g = ColorChannel(static_cast<int>(*horizontal_g));
  loaded.horizontal.b = ColorChannel(static_cast<int>(*horizontal_b));
  loaded.horizontal.alpha = ColorChannel(static_cast<int>(*horizontal_alpha));
  loaded.vertical.width = *vertical_width;
  loaded.vertical.r = ColorChannel(static_cast<int>(*vertical_r));
  loaded.vertical.g = ColorChannel(static_cast<int>(*vertical_g));
  loaded.vertical.b = ColorChannel(static_cast<int>(*vertical_b));
  loaded.vertical.alpha = ColorChannel(static_cast<int>(*vertical_alpha));

  const auto parsed_hide_key = ParseVK(hide_key->c_str(), 'h');
  const auto parsed_exit_key = ParseVK(exit_key->c_str(), 'e');
  loaded.hotkey_h_s.vk = parsed_hide_key.first;
  loaded.hotkey_h_s.mod = ParseMod(hide_mod->c_str(), parsed_hide_key.second);
  loaded.hotkey_exit.vk = parsed_exit_key.first;
  loaded.hotkey_exit.mod = ParseMod(exit_mod->c_str(), parsed_exit_key.second);
  loaded.ClampAll();

  if (loaded.hotkey_h_s.mod == loaded.hotkey_exit.mod &&
      loaded.hotkey_h_s.vk == loaded.hotkey_exit.vk) {
    return false;
  }

  *this = loaded;
  return true;
}

void Config::ClampAll() noexcept {
  gap = std::min(gap, 400U);
  horizontal.Clamp();
  vertical.Clamp();
  hotkey_h_s.Clamp('h');
  hotkey_exit.Clamp('e');
}

void HotkeyConfig::Clamp(char mode) noexcept {
  constexpr unsigned int kAllowedModifiers =
      MOD_ALT | MOD_CONTROL | MOD_SHIFT | MOD_WIN;
  if ((mod & ~kAllowedModifiers) != 0) mod = 0;

  const bool function_key = vk >= VK_F1 && vk <= VK_F24;
  const bool valid_key = function_key || (vk >= L'0' && vk <= L'9') ||
                         (vk >= L'A' && vk <= L'Z') ||
                         (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) ||
                         vk == VK_XBUTTON1 || vk == VK_XBUTTON2 ||
                         vk == VK_BACK || vk == VK_TAB || vk == VK_SPACE ||
                         vk == VK_RETURN || vk == VK_DELETE || vk == VK_INSERT ||
                         vk == VK_ESCAPE || (vk >= VK_OEM_1 && vk <= VK_OEM_3) ||
                         (vk >= VK_OEM_4 && vk <= VK_OEM_8) ||
                         vk == VK_OEM_PLUS || vk == VK_OEM_COMMA ||
                         vk == VK_OEM_MINUS || vk == VK_OEM_PERIOD;
  if (!valid_key) vk = (mode == 'e') ? L'E' : L'H';
  if (!function_key && mod == 0) mod = MOD_CONTROL | MOD_ALT | MOD_WIN;
}

unsigned int Config::ParseMod(const wchar_t* text, bool function_key) {
  constexpr unsigned int kDefault = MOD_CONTROL | MOD_ALT | MOD_WIN;
  std::wstring input = TrimAndLower(text != nullptr ? text : L"");
  if (input.empty()) return function_key ? 0U : kDefault;

  unsigned int modifiers = 0;
  size_t start = 0;
  while (start <= input.size()) {
    const size_t separator = input.find(L',', start);
    const size_t end = separator == std::wstring::npos ? input.size() : separator;
    const std::wstring token = TrimAndLower(input.substr(start, end - start));
    if (token == L"ctrl" || token == L"control") {
      modifiers |= MOD_CONTROL;
    } else if (token == L"alt") {
      modifiers |= MOD_ALT;
    } else if (token == L"win" || token == L"windows") {
      modifiers |= MOD_WIN;
    } else if (token == L"shift") {
      modifiers |= MOD_SHIFT;
    } else {
      return kDefault;
    }
    if (separator == std::wstring::npos) break;
    start = separator + 1;
  }
  return modifiers == 0 ? (function_key ? 0U : kDefault) : modifiers;
}

std::pair<unsigned int, bool> Config::ParseVK(const wchar_t* text, char mode) {
  static const std::unordered_map<std::wstring, unsigned int> key_map = {
      {L"xbutton1", VK_XBUTTON1}, {L"xbutton2", VK_XBUTTON2},
      {L"backspace", VK_BACK},    {L"tab", VK_TAB},
      {L"space", VK_SPACE},       {L"enter", VK_RETURN},
      {L"delete", VK_DELETE},     {L"insert", VK_INSERT},
      {L"esc", VK_ESCAPE},        {L"escape", VK_ESCAPE},
      {L"plus", VK_OEM_PLUS},     {L"+", VK_OEM_PLUS},
      {L"=", VK_OEM_PLUS},        {L"comma", VK_OEM_COMMA},
      {L",", VK_OEM_COMMA},       {L"minus", VK_OEM_MINUS},
      {L"-", VK_OEM_MINUS},       {L"_", VK_OEM_MINUS},
      {L"period", VK_OEM_PERIOD}, {L".", VK_OEM_PERIOD},
      {L"slash", VK_OEM_2},       {L"/", VK_OEM_2},
      {L"semicolon", VK_OEM_1},   {L";", VK_OEM_1},
      {L"apostrophe", VK_OEM_7},  {L"'", VK_OEM_7},
      {L"bracketleft", VK_OEM_4}, {L"[", VK_OEM_4},
      {L"backslash", VK_OEM_5},   {L"\\", VK_OEM_5},
      {L"bracketright", VK_OEM_6},{L"]", VK_OEM_6},
      {L"grave", VK_OEM_3},       {L"`", VK_OEM_3}};

  std::wstring key = TrimAndLower(text != nullptr ? text : L"");
  if (key.starts_with(L"vk_")) key.erase(0, 3);
  if (key.size() == 1) {
    const wchar_t character = key.front();
    if ((character >= L'a' && character <= L'z') ||
        (character >= L'0' && character <= L'9')) {
      return {static_cast<unsigned int>(towupper(character)), false};
    }
  }
  if (key.size() >= 2 && key.front() == L'f') {
    if (const auto number = ParseUnsigned(std::wstring_view(key).substr(1), 1, 24)) {
      return {VK_F1 + *number - 1, true};
    }
  }
  if (key.starts_with(L"numpad")) {
    if (const auto number = ParseUnsigned(std::wstring_view(key).substr(6), 0, 9)) {
      return {VK_NUMPAD0 + *number, false};
    }
  }
  if (const auto item = key_map.find(key); item != key_map.end()) {
    return {item->second, false};
  }
  return {static_cast<unsigned int>(mode == 'e' ? L'E' : L'H'), false};
}
