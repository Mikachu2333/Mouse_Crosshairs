#include <windows.h>
#include <string>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#include "src/crosshair.h"
#include "src/config.h"
#include "src/hotkey.h"
#include "src/config_file_util.h"

using namespace Gdiplus;

// GDI+ 初始化令牌
ULONG_PTR gdiToken;

int WINAPI WinMain(const HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 创建互斥体防止程序重复运行
    const HANDLE hMutex = CreateMutexA(nullptr, FALSE, "F5B6239126A64833BE094D6DC8DC1951");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxA(nullptr, "Already Exist.", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 设置 DPI 感知，确保在高分辨率屏幕上正确显示
    SetProcessDPIAware();

    // 初始化 GDI+
    const GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiToken, &gdiplusStartupInput, nullptr);

    // 获取配置文件路径并确保文件存在
    const std::string configPath = get_config_path();
    ensure_config_exists(configPath);

    // 加载配置文件
    Config config;
    if (!config.Load(configPath.c_str())) {
        MessageBoxA(nullptr, "Error load config, use default values.", "Warning", MB_OK | MB_ICONINFORMATION);
    }
    config.AutoSetLength();

    // 创建十字准星窗口
    CrosshairWindow crosshair(hInstance, config);
    if (!crosshair.Create()) {
        MessageBoxA(nullptr, "Error creating Crosshair window.", "Error", MB_OK | MB_ICONERROR);
        GdiplusShutdown(gdiToken);
        return 1;
    }

    // 注册全局热键
    HotkeyManager::RegisterToggleHotkey(config.hotkey_h_s, config.hotkey_exit);

    // 主消息循环
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
                case HOTKEY_ID:
                    crosshair.ToggleVisible();
                    continue;
                case HOTKEY_ID2:
                    exit(0);
            }
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理资源
    HotkeyManager::UnregisterAll();
    GdiplusShutdown(gdiToken);
    if (hMutex) CloseHandle(hMutex);
    return 0;
}
