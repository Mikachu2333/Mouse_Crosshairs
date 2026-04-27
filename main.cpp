#include <windows.h>
#include <string>

#include "src/crosshair.h"
#include "src/config.h"
#include "src/hotkey.h"
#include "src/config_file_util.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  HANDLE hMutex =
      CreateMutexW(nullptr, FALSE, L"F5B6239126A64833BE094D6DC8DC1951");
  if (!hMutex) {
    MessageBoxW(nullptr, L"Unable to create process mutex.", L"Error",
                MB_OK | MB_ICONERROR);
    return 1;
  }

  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(hMutex);
    MessageBoxW(nullptr, L"Already Exist.", L"Error", MB_OK | MB_ICONERROR);
    return 1;
  }

  SetProcessDPIAware();

  const std::wstring configPath = get_config_path();
  if (!ensure_config_exists(configPath)) {
    CloseHandle(hMutex);
    return 1;
  }

  Config config;
  if (!config.Load(configPath.c_str())) {
    MessageBoxW(nullptr, L"Error load config, use default values.", L"Warning",
                MB_OK | MB_ICONINFORMATION);
  }
  config.AutoSetLength();

  BOOL messageResult = 0;
  {
    CrosshairWindow crosshair(hInstance, config);
    if (!crosshair.Create()) {
      MessageBoxW(nullptr, L"Error creating Crosshair window.", L"Error",
                  MB_OK | MB_ICONERROR);
      CloseHandle(hMutex);
      return 1;
    }

    if (!HotkeyManager::RegisterToggleHotkey(config.hotkey_h_s,
                                             config.hotkey_exit)) {
      MessageBoxW(nullptr, L"Failed to register global hotkeys.", L"Error",
                  MB_OK | MB_ICONERROR);
      CloseHandle(hMutex);
      return 1;
    }

    MSG msg;
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
                MessageBoxW(nullptr, L"Failed to reload hotkeys from config.",
                            L"Warning", MB_OK | MB_ICONWARNING);
                if (!HotkeyManager::RegisterToggleHotkey(config.hotkey_h_s,
                                                         config.hotkey_exit)) {
                  MessageBoxW(nullptr, L"Hotkeys are currently unavailable.",
                              L"Error", MB_OK | MB_ICONERROR);
                }
              } else {
                config = updatedConfig;
              }
            } else {
              MessageBoxW(nullptr, L"Failed to reload config file.", L"Warning",
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
      MessageBoxW(nullptr, L"Message loop error.", L"Error",
                  MB_OK | MB_ICONERROR);
    }

    HotkeyManager::UnregisterAll();
  }

  CloseHandle(hMutex);
  return (messageResult == -1) ? 1 : 0;
}
