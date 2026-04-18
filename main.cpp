#include <windows.h>
#include <string>

#include "src/crosshair.h"
#include "src/config.h"
#include "src/hotkey.h"
#include "src/config_file_util.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  // 创建互斥体防止程序重复运行
  HANDLE hMutex =
      CreateMutexA(nullptr, FALSE, "F5B6239126A64833BE094D6DC8DC1951");
  if (!hMutex) {
    MessageBoxA(nullptr, "Unable to create process mutex.", "Error",
                MB_OK | MB_ICONERROR);
    return 1;
  }

  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(hMutex);
    MessageBoxA(nullptr, "Already Exist.", "Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  // 设置 DPI 感知，确保在高分辨率屏幕上正确显示
  SetProcessDPIAware();

  // 获取配置文件路径并确保文件存在
  const std::string configPath = get_config_path();
  if (!ensure_config_exists(configPath)) {
    CloseHandle(hMutex);
    return 1;
  }

  // 加载配置文件
  Config config;
  if (!config.Load(configPath.c_str())) {
    MessageBoxA(nullptr, "Error load config, use default values.", "Warning",
                MB_OK | MB_ICONINFORMATION);
  }
  config.AutoSetLength();

  // 创建十字准星窗口
  CrosshairWindow crosshair(hInstance, config);
  if (!crosshair.Create()) {
    MessageBoxA(nullptr, "Error creating Crosshair window.", "Error",
                MB_OK | MB_ICONERROR);
    CloseHandle(hMutex);
    return 1;
  }

  // 注册全局热键
  if (!HotkeyManager::RegisterToggleHotkey(config.hotkey_h_s,
                                           config.hotkey_exit)) {
    MessageBoxA(nullptr, "Failed to register global hotkeys.", "Error",
                MB_OK | MB_ICONERROR);
    CloseHandle(hMutex);
    return 1;
  }

  // 主消息循环
  MSG msg;
  BOOL messageResult = 0;
  while ((messageResult = GetMessage(&msg, nullptr, 0, 0)) > 0) {
    if (msg.message == WM_HOTKEY) {
      switch (msg.wParam) {
        case HOTKEY_ID: {
          crosshair.ToggleVisible();
          Config updatedConfig;
          if (updatedConfig.Load(configPath.c_str())) {
            updatedConfig.AutoSetLength();
            crosshair.ApplyConfig(updatedConfig);
            if (!HotkeyManager::RegisterToggleHotkey(
                    updatedConfig.hotkey_h_s, updatedConfig.hotkey_exit)) {
              MessageBoxA(nullptr, "Failed to reload hotkeys from config.",
                          "Warning", MB_OK | MB_ICONWARNING);
              if (!HotkeyManager::RegisterToggleHotkey(config.hotkey_h_s,
                                                       config.hotkey_exit)) {
                MessageBoxA(nullptr, "Hotkeys are currently unavailable.",
                            "Error", MB_OK | MB_ICONERROR);
              }
            } else {
              config = updatedConfig;
            }
          } else {
            MessageBoxA(nullptr, "Failed to reload config file.", "Warning",
                        MB_OK | MB_ICONWARNING);
          }
          break;
        }
        case HOTKEY_ID2: {
          PostQuitMessage(0);
          break;
        }
      }
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  if (messageResult == -1) {
    MessageBoxA(nullptr, "Message loop error.", "Error", MB_OK | MB_ICONERROR);
  }

  // 清理资源
  HotkeyManager::UnregisterAll();
  CloseHandle(hMutex);
  return (messageResult == -1) ? 1 : 0;
}
