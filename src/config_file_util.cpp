#include "./config_file_util.h"

#include <windows.h>
#include <shlobj.h>

#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

namespace {
std::wstring GetExecutablePath() {
  std::vector<wchar_t> buffer(512);
  for (;;) {
    SetLastError(ERROR_SUCCESS);
    const DWORD length = GetModuleFileNameW(
        nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) return {};
    if (length < buffer.size() - 1 ||
        (length < buffer.size() && GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
      return std::wstring(buffer.data(), length);
    }
    if (buffer.size() >= 32768) return {};
    buffer.resize(std::min<size_t>(buffer.size() * 2, 32768));
  }
}

std::wstring GetLocalAppDataConfigPath() {
  PWSTR raw_path = nullptr;
  const HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData,
                                               KF_FLAG_CREATE, nullptr,
                                               &raw_path);
  if (FAILED(result) || raw_path == nullptr) return {};
  const std::filesystem::path path =
      std::filesystem::path(raw_path) / L"MouseCrosshair" / L"crosshair.ini";
  CoTaskMemFree(raw_path);
  return path.wstring();
}

bool IsRegularFile(const std::filesystem::path& path) {
  std::error_code error;
  return std::filesystem::is_regular_file(path, error) && !error;
}

bool CanCreateInDirectory(const std::filesystem::path& directory) {
  std::error_code error;
  if (!std::filesystem::is_directory(directory, error) || error) return false;
  const auto probe = directory / L".mouse_crosshair_write_test.tmp";
  HANDLE file = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr,
                            CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY |
                                            FILE_FLAG_DELETE_ON_CLOSE,
                            nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  CloseHandle(file);
  return true;
}
}  // namespace

std::wstring GetConfigPath() {
  const std::wstring executable = GetExecutablePath();
  if (!executable.empty()) {
    const std::filesystem::path portable_path =
        std::filesystem::path(executable).parent_path() / L"crosshair.ini";
    // 已存在的便携配置始终优先；否则仅在目录可写时创建。
    if (IsRegularFile(portable_path) ||
        CanCreateInDirectory(portable_path.parent_path())) {
      return portable_path.wstring();
    }
  }
  return GetLocalAppDataConfigPath();
}

bool EnsureConfigExists(const std::wstring& path) {
  if (path.empty()) {
    MessageBoxW(nullptr, L"无法确定配置文件路径。", L"错误",
                MB_OK | MB_ICONERROR);
    return false;
  }

  const std::filesystem::path config_path(path);
  if (IsRegularFile(config_path)) return true;

  std::error_code error;
  std::filesystem::create_directories(config_path.parent_path(), error);
  if (error) {
    MessageBoxW(nullptr, L"无法创建配置目录。", L"错误",
                MB_OK | MB_ICONERROR);
    return false;
  }

  // CREATE_NEW 防止检查与创建之间覆盖另一个进程刚创建的文件。
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    return GetLastError() == ERROR_FILE_EXISTS;
  }

  DWORD written = 0;
  const DWORD size = static_cast<DWORD>(sizeof(kDefaultIni) - 1);
  const bool success = WriteFile(file, kDefaultIni, size, &written, nullptr) &&
                       written == size && FlushFileBuffers(file);
  CloseHandle(file);
  if (!success) {
    DeleteFileW(path.c_str());
    MessageBoxW(nullptr, L"写入默认配置文件失败。", L"错误",
                MB_OK | MB_ICONERROR);
    return false;
  }

  MessageBoxW(nullptr, L"已创建默认配置文件。", L"成功",
              MB_OK | MB_ICONINFORMATION);
  return true;
}
