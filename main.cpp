#include <windows.h>

#include <exception>
#include <string>

#include "src/config.h"
#include "src/config_file_util.h"
#include "src/crosshair.h"
#include "src/hotkey.h"

namespace {
constexpr wchar_t kMutexName[] =
    L"Local\\F5B6239126A64833BE094D6DC8DC1951";

class UniqueHandle {
 public:
  explicit UniqueHandle(HANDLE handle = nullptr) noexcept : handle_(handle) {}
  ~UniqueHandle() {
    if (handle_ != nullptr) CloseHandle(handle_);
  }
  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;
  [[nodiscard]] HANDLE get() const noexcept { return handle_; }

 private:
  HANDLE handle_;
};

void EnableBestDpiAwareness() noexcept {
  using SetDpiContext = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
  if (const HMODULE user32 = GetModuleHandleW(L"user32.dll"); user32 != nullptr) {
    const auto set_context = reinterpret_cast<SetDpiContext>(
        GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
    if (set_context != nullptr &&
        set_context(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
      return;
    }
  }
  SetProcessDPIAware();  // Windows 7/8 兼容回退。
}

int Run(HINSTANCE instance) {
  const UniqueHandle mutex(CreateMutexW(nullptr, FALSE, kMutexName));
  if (mutex.get() == nullptr) {
    MessageBoxW(nullptr, L"无法创建单实例互斥量。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    MessageBoxW(nullptr, L"程序已经在当前用户会话中运行。", L"提示",
                MB_OK | MB_ICONINFORMATION);
    return 0;
  }

  EnableBestDpiAwareness();
  const std::wstring config_path = GetConfigPath();
  if (!EnsureConfigExists(config_path)) return 1;

  Config config;
  if (!config.Load(config_path.c_str())) {
    MessageBoxW(nullptr, L"配置文件无效，请修正后重新启动程序。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }

  CrosshairWindow crosshair(instance, config);
  if (!crosshair.Create()) {
    MessageBoxW(nullptr, L"创建或初始化准星窗口失败。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }
  if (!HotkeyManager::Register(config.hotkey_h_s, config.hotkey_exit)) {
    MessageBoxW(nullptr, L"注册全局热键失败；热键可能已被其他程序占用。",
                L"错误", MB_OK | MB_ICONERROR);
    return 1;
  }

  int result = 0;
  MSG message = {};
  while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
    if (message.message != WM_HOTKEY) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
      continue;
    }

    if (message.wParam == kExitHotkeyId) {
      PostQuitMessage(0);
      continue;
    }
    if (message.wParam != kToggleHotkeyId) continue;

    if (crosshair.IsVisible()) {
      crosshair.ToggleVisible();
      continue;
    }

    // 仅在重新显示时热重载。先验证并注册热键，再提交窗口配置。
    Config updated;
    if (!updated.Load(config_path.c_str())) {
      MessageBoxW(nullptr, L"配置文件无效，继续使用上一次有效配置。", L"警告",
                  MB_OK | MB_ICONWARNING);
      crosshair.ToggleVisible();
      continue;
    }
    if (!HotkeyManager::Register(updated.hotkey_h_s, updated.hotkey_exit)) {
      if (!HotkeyManager::Register(config.hotkey_h_s, config.hotkey_exit)) {
        MessageBoxW(nullptr, L"恢复原热键失败，程序将退出。", L"错误",
                    MB_OK | MB_ICONERROR);
        return 1;
      }
      MessageBoxW(nullptr, L"新热键不可用，继续使用上一次有效配置。", L"警告",
                  MB_OK | MB_ICONWARNING);
      crosshair.ToggleVisible();
      continue;
    }
    if (!crosshair.ApplyConfig(updated)) {
      const bool hotkeys_restored =
          HotkeyManager::Register(config.hotkey_h_s, config.hotkey_exit);
      const bool config_restored = crosshair.ApplyConfig(config);
      if (!hotkeys_restored || !config_restored) {
        MessageBoxW(nullptr, L"恢复原配置失败，程序将退出。", L"错误",
                    MB_OK | MB_ICONERROR);
        return 1;
      }
      MessageBoxW(nullptr, L"应用新配置失败，已恢复上一次有效配置。", L"警告",
                  MB_OK | MB_ICONWARNING);
      crosshair.ToggleVisible();
      continue;
    }
    config = updated;
    crosshair.ToggleVisible();
  }

  HotkeyManager::UnregisterAll();
  if (result == -1) {
    MessageBoxW(nullptr, L"Windows 消息循环异常终止。", L"错误",
                MB_OK | MB_ICONERROR);
    return 1;
  }
  return static_cast<int>(message.wParam);
}
}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
  try {
    return Run(instance);
  } catch (const std::exception&) {
    MessageBoxW(nullptr, L"程序发生未处理的 C++ 异常。", L"严重错误",
                MB_OK | MB_ICONERROR);
  } catch (...) {
    MessageBoxW(nullptr, L"程序发生未知异常。", L"严重错误",
                MB_OK | MB_ICONERROR);
  }
  return 1;
}
