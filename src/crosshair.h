#pragma once
#include <windows.h>
#include "config.h"

class CrosshairWindow {
public:
    CrosshairWindow(HINSTANCE hInst, const Config &cfg);

    ~CrosshairWindow();

    bool Create();

    void ToggleVisible();

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

    void DrawCrosshair(HDC hdc) const;

    HINSTANCE hInstance;
    HWND hwnd;
    Config config;
    bool visible;
};
