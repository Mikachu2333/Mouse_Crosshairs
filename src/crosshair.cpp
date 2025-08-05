#include "./crosshair.h"
#include <gdiplus.h>
#include <algorithm>
#include <thread>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"

HHOOK CrosshairWindow::g_mouseHook = nullptr;
CrosshairWindow *CrosshairWindow::g_instance = nullptr;

CrosshairWindow::CrosshairWindow(const HINSTANCE hInst, const Config &cfg)
    : hInstance(hInst), hwnd(nullptr), config(cfg), visible(true),
      hBmp(nullptr), memDC(nullptr), bmpWidth(0), bmpHeight(0) {
    g_instance = this;
}

CrosshairWindow::~CrosshairWindow() {
    if (hwnd) {
        DestroyWindow(hwnd);
    }
    if (memDC) DeleteDC(memDC);
    if (hBmp) DeleteObject(hBmp);
    if (g_mouseHook) UnhookWindowsHookEx(g_mouseHook);
    g_instance = nullptr;
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
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        CLASS_NAME, L"", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, this);

    if (!hwnd) return false;

    ShowWindow(hwnd, SW_SHOW);

    // 安装全局鼠标钩子
    g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, nullptr, 0);

    // 初始化GDI资源
    OnResize();

    return true;
}

void CrosshairWindow::ToggleVisible() {
    visible = !visible;
    ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
}

void CrosshairWindow::OnResize() {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width == bmpWidth && height == bmpHeight) return;

    if (memDC) {
        DeleteDC(memDC);
        memDC = nullptr;
    }
    if (hBmp) {
        DeleteObject(hBmp);
        hBmp = nullptr;
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screenDC = GetDC(nullptr);
    hBmp = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    memDC = CreateCompatibleDC(screenDC);
    SelectObject(memDC, hBmp);
    ReleaseDC(nullptr, screenDC);

    bmpWidth = width;
    bmpHeight = height;
}

void CrosshairWindow::OnMouseMove() {
    if (visible && hwnd) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
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
                BeginPaint(hWnd, &ps);
                self->DrawCrosshair(self->memDC);
                EndPaint(hWnd, &ps);
            }
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            if (self) self->OnResize();
            return 0;
        case WM_DESTROY:
            if (g_mouseHook) {
                UnhookWindowsHookEx(g_mouseHook);
                g_mouseHook = nullptr;
            }
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void CrosshairWindow::DrawCrosshair(const HDC hdc) const {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // 使用 GDI+ 进行抗锯齿绘制
    Gdiplus::Graphics graphics(hdc);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // 水平线
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
    // 垂直线
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

    // 更新分层窗口
    HDC screenDC = GetDC(nullptr);
    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd, screenDC, nullptr, &sizeWnd, hdc, &ptSrc, 0, &blend, ULW_ALPHA);
    ReleaseDC(nullptr, screenDC);
}

// 全局鼠标钩子回调
LRESULT CALLBACK CrosshairWindow::MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE && g_instance) {
        g_instance->OnMouseMove();
    }
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}