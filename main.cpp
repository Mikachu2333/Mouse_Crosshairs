#include <windows.h>
#include <string>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "src/crosshair.h"
#include "src/config.h"
#include "src/hotkey.h"
#include "src/config_file_util.h"

using namespace Gdiplus;

ULONG_PTR gdiToken;

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    const HANDLE hMutex = CreateMutexA(nullptr, FALSE, "F5B6239126A64833BE094D6DC8DC1951");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxA(nullptr, "Already Exist.", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    SetProcessDPIAware();

    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiToken, &gdiplusStartupInput, nullptr);

    const std::string configPath = get_config_path();
    ensure_config_exists(configPath);

    Config config;
    if (!config.Load(configPath.c_str())) {
        MessageBoxA(nullptr, "Error load config, use default values.", "Warning", MB_OK | MB_ICONINFORMATION);
    }
    config.AutoSetLength();

    CrosshairWindow crosshair(hInstance, config);
    if (!crosshair.Create()) {
        MessageBoxA(nullptr, "Error creating Crosshair window.", "Error", MB_OK | MB_ICONERROR);
        GdiplusShutdown(gdiToken);
        return 1;
    }

    HotkeyManager hotkey;
    hotkey.RegisterToggleHotkey(config.hotkey);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (hotkey.IsToggleHotkey(msg)) {
            crosshair.ToggleVisible();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hotkey.UnregisterAll();
    GdiplusShutdown(gdiToken);
    if (hMutex) CloseHandle(hMutex);
    return 0;
}
