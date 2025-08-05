#include "./crosshair.h"
#include <gdiplus.h>
#include <algorithm>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"
#define TIMER_ID 0x1001      // 定时器 ID
#define TIMER_INTERVAL 15    // 刷新间隔（约60FPS）

CrosshairWindow::CrosshairWindow(const HINSTANCE hInst, const Config &cfg)
    : hInstance(hInst), hwnd(nullptr), config(cfg), visible(true) {
}

CrosshairWindow::~CrosshairWindow() {
    if (hwnd) {
        KillTimer(hwnd, TIMER_ID);
        DestroyWindow(hwnd);
    }
}

// 创建透明的分层窗口
bool CrosshairWindow::Create() {
    // 注册窗口类
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    // 创建全屏透明窗口
    hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, hInstance, this);

    if (!hwnd) return false;

    // 启动刷新定时器
    SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, nullptr);
    ShowWindow(hwnd, SW_SHOW);
    return true;
}

// 切换十字准星显示状态
void CrosshairWindow::ToggleVisible() {
    visible = !visible;
    ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
}

// 窗口过程函数
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
            return 1; // 阻止背景擦除保持透明
        case WM_TIMER:
            if (wParam == TIMER_ID && self && self->visible) {
                InvalidateRect(hWnd, nullptr, FALSE); // 触发重绘，跟随鼠标
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

// 绘制跟随鼠标的十字准星
void CrosshairWindow::DrawCrosshair(const HDC hdc) const {
    RECT rc;
    GetClientRect(hwnd, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    // 创建32位带alpha通道的DIB位图
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

    // 使用 GDI+ 进行抗锯齿绘制
    Gdiplus::Graphics graphics(memDC);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    // 获取鼠标当前位置
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // 绘制水平线
    {
        const auto &c = config.horizontal;
        Gdiplus::Pen pen(
            Gdiplus::Color(c.alpha, c.r, c.g, c.b),
            static_cast<Gdiplus::REAL>(c.width)
        );

        // 计算线条长度，不超出屏幕边界
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

    // 绘制垂直线
    {
        const auto &c = config.vertical;
        Gdiplus::Pen pen(
            Gdiplus::Color(c.alpha, c.r, c.g, c.b),
            static_cast<Gdiplus::REAL>(c.width)
        );

        // 计算线条长度，不超出屏幕边界
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

    // 更新分层窗口，实现透明效果
    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd, screenDC, nullptr, &sizeWnd, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

    // 清理资源
    SelectObject(memDC, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}
