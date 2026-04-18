#include "./config_file_util.h"

#include <windows.h>

#include <fstream>

// 获取配置文件绝对路径（与exe同目录）
std::string get_config_path() {
  char path[MAX_PATH] = {};
  const DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    return "crosshair.ini";
  }
  std::string exePath = path;
  const size_t pos = exePath.find_last_of("\\/");
  if (pos != std::string::npos) {
    exePath = exePath.substr(0, pos + 1);
  }
  return exePath + "crosshair.ini";
}

// 检查配置文件是否存在，不存在则创建默认配置
bool ensure_config_exists(const std::string& path) {
  std::ifstream file(path);
  if (file.good()) {
    return true;
  }

  std::ofstream outFile(path, std::ios::out | std::ios::trunc);
  if (!outFile.is_open()) {
    MessageBoxA(nullptr, "Unable to create config file.", "Error",
                MB_OK | MB_ICONERROR);
    return false;
  }

  outFile << DEFAULT_INI;
  if (!outFile.good()) {
    outFile.close();
    MessageBoxA(nullptr, "Failed to write default config file.", "Error",
                MB_OK | MB_ICONERROR);
    return false;
  }
  outFile.close();

  MessageBoxA(nullptr, "Config file created with default settings.", "Success",
              MB_OK | MB_ICONINFORMATION);
  return true;
}
