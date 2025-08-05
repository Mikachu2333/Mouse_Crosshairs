#include "crosshair.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"
#define TIMER_ID 0x1001
#define TIMER_INTERVAL 16  // 减少到60FPS左右
#define TRANSPARENT_COLOR RGB(0,0,0)

CrosshairWindow::CrosshairWindow(const HINSTANCE hInst, const Config &cfg)
    : hInstance(hInst), hwnd(nullptr), config(cfg), visible(true) {
}

bool CrosshairWindow::Create() {
    // 在创建窗口前设置DPI感知
    SetProcessDPIAware();

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, this);

    if (!hwnd) return false;

    SetLayeredWindowAttributes(hwnd, TRANSPARENT_COLOR, 0, LWA_COLORKEY);
    SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    return true;
}

void CrosshairWindow::ToggleVisible() {
    visible = !visible;
    ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
}

LRESULT CALLBACK CrosshairWindow::WndProc(const HWND hWnd, const UINT msg, WPARAM wParam, LPARAM lParam) {
    CrosshairWindow *self = nullptr;
    if (msg == WM_NCCREATE) {
        const CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
        self = static_cast<CrosshairWindow *>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->hwnd = hWnd;
    } else {
        self = reinterpret_cast<CrosshairWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    switch (msg) {
        case WM_PAINT:
            if (self) {
                PAINTSTRUCT ps;
                const HDC hdc = BeginPaint(hWnd, &ps);
                self->DrawCrosshair(hdc);
                EndPaint(hWnd, &ps);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;  // 阻止背景擦除
        case WM_TIMER:
            if (wParam == TIMER_ID && self && self->visible) {
                InvalidateRect(hWnd, nullptr, FALSE);  // 使用FALSE避免背景擦除
            }
            return 0;
        case WM_DESTROY:
            KillTimer(hWnd, TIMER_ID);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void CrosshairWindow::DrawCrosshair(const HDC hdc) const {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // 创建内存位图进行双缓冲
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    // 在内存DC上绘制
    Gdiplus::Graphics graphics(memDC);
    graphics.Clear(Gdiplus::Color(TRANSPARENT_COLOR));  // 用透明色清除
    graphics.SetPageUnit(Gdiplus::UnitPixel);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // 横线
    {
        const auto &c = config.horizontal;
        const Gdiplus::Color color(c.alpha, c.r, c.g, c.b);
        const Gdiplus::Pen pen(color, static_cast<Gdiplus::REAL>(c.width));
        graphics.DrawLine(&pen,
                          static_cast<INT>(pt.x - c.length), static_cast<INT>(pt.y),
                          static_cast<INT>(pt.x + c.length), static_cast<INT>(pt.y)
        );
    }
    // 竖线
    {
        const auto &c = config.vertical;
        const Gdiplus::Color color(c.alpha, c.r, c.g, c.b);
        const Gdiplus::Pen pen(color, static_cast<Gdiplus::REAL>(c.width));
        graphics.DrawLine(&pen,
                          static_cast<INT>(pt.x), static_cast<INT>(pt.y - c.length),
                          static_cast<INT>(pt.x), static_cast<INT>(pt.y + c.length)
        );
    }

    // 将内存位图绘制到窗口
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

    // 清理资源
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}