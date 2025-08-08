#pragma once
#include <windows.h>
#include <vector>
#include "config.h"

struct MonitorInfo {
    HMONITOR hMonitor;
    RECT rect;
    HWND hwnd;
    HDC memDC;
    HBITMAP hBmp;
    bool needsUpdate;
};

class CrosshairWindow {
public:
    CrosshairWindow(HINSTANCE hInst, const Config &cfg);

    ~CrosshairWindow();

    bool Create();

    void ToggleVisible();

    // 鼠标钩子接口
    void OnMouseMove() const;

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

    void DrawCrosshair(HDC hdc, const RECT &monitorRect) const;

    static void OnResize(MonitorInfo &monitor);

    void UpdateMonitors();

    void CreateWindowForMonitor(MonitorInfo &monitor);

    void DestroyMonitorWindows();

    MonitorInfo *FindMonitorContainingPoint(POINT pt);

    HINSTANCE hInstance;
    Config config;
    bool visible;

    // 多屏幕支持
    std::vector<MonitorInfo> monitors;
    bool monitorsChanged;

    // 鼠标钩子
    static HHOOK g_mouseHook;
    static CrosshairWindow *g_instance;

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
};