#include "crosshair.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"
#define TIMER_ID 0x1001
#define TIMER_INTERVAL 10
#define TRANSPARENT_COLOR RGB(0,0,0)

CrosshairWindow::CrosshairWindow(const HINSTANCE hInst, const Config &cfg)
    : hInstance(hInst), hwnd(nullptr), config(cfg), visible(true) {
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

    // 设置窗口背景色为透明色（黑色），并用颜色键实现背景透明
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
            break;
        case WM_ERASEBKGND:
            // 禁止系统自动擦除背景，减少闪烁
            return 1;
        case WM_TIMER:
            if (wParam == TIMER_ID && self && self->visible) {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        case WM_DESTROY:
            KillTimer(hWnd, TIMER_ID);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void CrosshairWindow::DrawCrosshair(const HDC hdc) const {
    RECT rc;
    GetClientRect(hwnd, &rc);
    const int width = rc.right - rc.left;
    const int height = rc.bottom - rc.top;

    // 创建内存DC和兼容位图
    const HDC memDC = CreateCompatibleDC(hdc);
    const HBITMAP memBmp = CreateCompatibleBitmap(hdc, width, height);
    const HGDIOBJ oldBmp = SelectObject(memDC, memBmp);

    // 用透明色刷背景
    const HBRUSH hBrush = CreateSolidBrush(TRANSPARENT_COLOR);
    FillRect(memDC, &rc, hBrush);
    DeleteObject(hBrush);

    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // 横线
    {
        const auto &c = config.horizontal;
        const HPEN pen = CreatePen(PS_SOLID, c.width, RGB(c.r, c.g, c.b));
        const HGDIOBJ oldPen = SelectObject(memDC, pen);
        MoveToEx(memDC, pt.x - c.length, pt.y, nullptr);
        LineTo(memDC, pt.x + c.length, pt.y);
        SelectObject(memDC, oldPen);
        DeleteObject(pen);
    }
    // 竖线
    {
        const auto &c = config.vertical;
        HPEN pen = CreatePen(PS_SOLID, c.width, RGB(c.r, c.g, c.b));
        HGDIOBJ oldPen = SelectObject(memDC, pen);
        MoveToEx(memDC, pt.x, pt.y - c.length, nullptr);
        LineTo(memDC, pt.x, pt.y + c.length);
        SelectObject(memDC, oldPen);
        DeleteObject(pen);
    }

    // 将内存DC内容拷贝到窗口DC
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // 清理
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}
