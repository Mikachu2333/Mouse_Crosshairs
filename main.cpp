#include <windows.h>
#include <string>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "crosshair.h"
#include "config.h"
#include "hotkey.h"
#include "config_file_util.h"

using namespace Gdiplus;

ULONG_PTR gdiToken;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 在程序开始时设置DPI感知
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
    return 0;
}