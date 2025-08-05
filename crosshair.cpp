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

    // 获取DPI缩放因子
    HDC screenDC = GetDC(hwnd);
    int dpiX = GetDeviceCaps(screenDC, LOGPIXELSX);
    int dpiY = GetDeviceCaps(screenDC, LOGPIXELSY);
    ReleaseDC(hwnd, screenDC);
    float scaleX = dpiX / 96.0f;
    float scaleY = dpiY / 96.0f;

    // 计算缩放后的最大长度
    int maxHorzLength = static_cast<int>((rc.right) / scaleX);
    int maxVertLength = static_cast<int>((rc.bottom) / scaleY);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // 横线
    {
        const auto &c = config.horizontal;
        HPEN pen = CreatePen(PS_SOLID, c.width, RGB(c.r, c.g, c.b));
        HGDIOBJ oldPen = SelectObject(memDC, pen);

        int leftLength = std::min<int>(pt.x, rc.right);
        int rightLength = std::min<int>(rc.right - pt.x, rc.right);

        MoveToEx(memDC, pt.x - leftLength, pt.y, nullptr);
        LineTo(memDC, pt.x + rightLength, pt.y);

        SelectObject(memDC, oldPen);
        DeleteObject(pen);
    }
    // 竖线
    {
        const auto &c = config.vertical;
        HPEN pen = CreatePen(PS_SOLID, c.width, RGB(c.r, c.g, c.b));
        HGDIOBJ oldPen = SelectObject(memDC, pen);

        int topLength = std::min<int>(pt.y, rc.bottom);
        int bottomLength = std::min<int>(rc.bottom - pt.y, rc.bottom);

        MoveToEx(memDC, pt.x, pt.y - topLength, nullptr);
        LineTo(memDC, pt.x, pt.y + bottomLength);

        SelectObject(memDC, oldPen);
        DeleteObject(pen);
    }

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}
