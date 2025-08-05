#include <windows.h>
#include <string>
#include "crosshair.h"
#include "config.h"
#include "hotkey.h"
#include "config_file_util.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
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
    return 0;
}
