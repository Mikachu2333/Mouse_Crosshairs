#include "./config_file_util.h"

#include <windows.h>

#include <fstream>

std::wstring get_config_path() {
  wchar_t path[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    return L"crosshair.ini";
  }
  std::wstring exePath = path;
  const size_t pos = exePath.find_last_of(L"\\/");
  if (pos != std::wstring::npos) {
    exePath = exePath.substr(0, pos + 1);
  }
  return exePath + L"crosshair.ini";
}

bool ensure_config_exists(const std::wstring& path) {
  std::ifstream file(path);
  if (file.good()) {
    return true;
  }

  std::ofstream outFile(path, std::ios::out | std::ios::trunc);
  if (!outFile.is_open()) {
    MessageBoxW(nullptr, L"Unable to create config file.", L"Error",
                MB_OK | MB_ICONERROR);
    return false;
  }

  outFile << DEFAULT_INI;
  if (!outFile.good()) {
    outFile.close();
    MessageBoxW(nullptr, L"Failed to write default config file.", L"Error",
                MB_OK | MB_ICONERROR);
    return false;
  }
  outFile.close();

  MessageBoxW(nullptr, L"Config file created with default settings.",
              L"Success", MB_OK | MB_ICONINFORMATION);
  return true;
}
