#include "./crosshair.h"
#include <gdiplus.h>
#include <algorithm>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"
#define TIMER_ID 0x1001
#define TIMER_INTERVAL 16

CrosshairWindow::CrosshairWindow(const HINSTANCE hInst, const Config &cfg)
    : hInstance(hInst), hwnd(nullptr), config(cfg), visible(true) {
}

CrosshairWindow::~CrosshairWindow() {
    if (hwnd) {
        KillTimer(hwnd, TIMER_ID);
        DestroyWindow(hwnd);
    }
}

bool CrosshairWindow::Create() {
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
            return 1; // 阻止背景擦除
        case WM_TIMER:
            if (wParam == TIMER_ID && self && self->visible) {
                InvalidateRect(hWnd, nullptr, FALSE); // 使用FALSE避免背景擦除
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
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    // 创建32位带alpha的DIB
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;
    HDC screenDC = GetDC(nullptr);
    HBITMAP hBmp = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HDC memDC = CreateCompatibleDC(screenDC);
    HGDIOBJ oldBmp = SelectObject(memDC, hBmp);

    Gdiplus::Graphics graphics(memDC);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // 横线
    {
        const auto &c = config.horizontal;
        Gdiplus::Pen pen(
            Gdiplus::Color(c.alpha, c.r, c.g, c.b),
            static_cast<Gdiplus::REAL>(c.width)
        );

        int leftLength = std::min<int>(pt.x, width);
        int rightLength = std::min<int>(width - pt.x, width);

        graphics.DrawLine(
            &pen,
            static_cast<Gdiplus::REAL>(pt.x - leftLength),
            static_cast<Gdiplus::REAL>(pt.y),
            static_cast<Gdiplus::REAL>(pt.x + rightLength),
            static_cast<Gdiplus::REAL>(pt.y)
        );
    }

    // 竖线
    {
        const auto &c = config.vertical;
        Gdiplus::Pen pen(
            Gdiplus::Color(c.alpha, c.r, c.g, c.b),
            static_cast<Gdiplus::REAL>(c.width)
        );

        int topLength = std::min<int>(pt.y, height);
        int bottomLength = std::min<int>(height - pt.y, height);

        graphics.DrawLine(
            &pen,
            static_cast<Gdiplus::REAL>(pt.x),
            static_cast<Gdiplus::REAL>(pt.y - topLength),
            static_cast<Gdiplus::REAL>(pt.x),
            static_cast<Gdiplus::REAL>(pt.y + bottomLength)
        );
    }

    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd, screenDC, nullptr, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}
