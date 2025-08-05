#pragma once
#include <windows.h>
#include "config.h"

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

    void DrawCrosshair(HDC hdc) const;

    void OnResize();

    HINSTANCE hInstance;
    HWND hwnd;
    Config config;
    bool visible;

    // GDI资源缓存
    HBITMAP hBmp;
    HDC memDC;
    int bmpWidth, bmpHeight;

    // 鼠标钩子
    static HHOOK g_mouseHook;
    static CrosshairWindow *g_instance;

    static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
};