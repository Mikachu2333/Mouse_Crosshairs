#include "./crosshair.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"

HHOOK CrosshairWindow::g_mouseHook = nullptr;
CrosshairWindow* CrosshairWindow::g_instance = nullptr;
unsigned int CrosshairWindow::g_windowCount = 0;
bool CrosshairWindow::g_hookInstalled = false;

CrosshairWindow::CrosshairWindow(HINSTANCE hInst, const Config& cfg)
    : hInstance(hInst),
      config(cfg),
      visible(true),
      hwndH(nullptr),
      hwndV(nullptr) {
  g_instance = this;
}

CrosshairWindow::~CrosshairWindow() {
  UninstallMouseHook();
  if (hwndH) {
    DestroyWindow(hwndH);
    hwndH = nullptr;
  }
  if (hwndV) {
    DestroyWindow(hwndV);
    hwndV = nullptr;
  }
  UnregisterClassW(CLASS_NAME, hInstance);
  g_instance = nullptr;
}

void CrosshairWindow::InstallMouseHook() {
  if (g_hookInstalled) return;
  HHOOK hook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, nullptr, 0);
  if (hook) {
    g_mouseHook = hook;
    g_hookInstalled = true;
  }
}

void CrosshairWindow::UninstallMouseHook() {
  if (!g_hookInstalled || !g_mouseHook) return;
  UnhookWindowsHookEx(g_mouseHook);
  g_mouseHook = nullptr;
  g_hookInstalled = false;
}

bool CrosshairWindow::Create() {
  WNDCLASSEXW wc = {sizeof(wc)};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.style = 0;

  ATOM classAtom = RegisterClassExW(&wc);
  if (!classAtom) {
    return false;
  }

  int vX = GetSystemMetrics(SM_XVIRTUALSCREEN);
  int vY = GetSystemMetrics(SM_YVIRTUALSCREEN);
  int vW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int vH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  // 创建横向窗口
  hwndH = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                              WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                          CLASS_NAME, L"", WS_POPUP, vX, vY, vW,
                          static_cast<int>(config.horizontal.width), nullptr,
                          nullptr, hInstance, this);

  // 创建纵向窗口
  hwndV = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                              WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                          CLASS_NAME, L"", WS_POPUP, vX, vY,
                          static_cast<int>(config.vertical.width), vH, nullptr,
                          nullptr, hInstance, this);

  if (hwndH && hwndV) {
    g_windowCount += 2;

    // 设置半透明和背景色
    SetLayeredWindowAttributes(hwndH, RGB(0, 0, 0),
                               static_cast<BYTE>(config.horizontal.alpha),
                               LWA_ALPHA);
    SetLayeredWindowAttributes(hwndV, RGB(0, 0, 0),
                               static_cast<BYTE>(config.vertical.alpha),
                               LWA_ALPHA);

    ShowWindow(hwndH, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(hwndV, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  } else {
    if (hwndH) DestroyWindow(hwndH);
    if (hwndV) DestroyWindow(hwndV);
    hwndH = nullptr;
    hwndV = nullptr;
    UnregisterClassW(CLASS_NAME, hInstance);
    return false;
  }

  InstallMouseHook();

  return (hwndH != nullptr && hwndV != nullptr);
}

void CrosshairWindow::ToggleVisible() {
  visible = !visible;

  if (hwndH) ShowWindow(hwndH, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndV) ShowWindow(hwndV, visible ? SW_SHOWNOACTIVATE : SW_HIDE);

  if (visible) {
    InstallMouseHook();
    OnMouseMove();
  } else {
    UninstallMouseHook();
  }
}

void CrosshairWindow::OnMouseMove() const {
  if (!visible) return;

  POINT pt;
  if (!GetCursorPos(&pt)) return;

  // 获取当前鼠标所在的显示器
  HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
  if (!hMon) {
    SetWindowPos(hwndH, nullptr, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    SetWindowPos(hwndV, nullptr, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    return;
  }

  MONITORINFO mi = {sizeof(mi)};
  if (GetMonitorInfo(hMon, &mi)) {
    int monW = mi.rcMonitor.right - mi.rcMonitor.left;
    int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // 仅在当前显示器范围内移动窗口，宽度/高度等于显示器大小
    SetWindowPos(hwndH, HWND_TOPMOST, mi.rcMonitor.left,
                 pt.y - static_cast<int>(config.horizontal.width) / 2, monW,
                 static_cast<int>(config.horizontal.width),
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    SetWindowPos(hwndV, HWND_TOPMOST,
                 pt.x - static_cast<int>(config.vertical.width) / 2,
                 mi.rcMonitor.top, static_cast<int>(config.vertical.width),
                 monH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  }
}

LRESULT CALLBACK CrosshairWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam) {
  CrosshairWindow* self = nullptr;
  if (msg == WM_NCCREATE) {
    const CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
    self = static_cast<CrosshairWindow*>(cs->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  } else {
    self = reinterpret_cast<CrosshairWindow*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA));
  }

  switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      if (self) {
        RECT rc;
        GetClientRect(hWnd, &rc);
        // 判断是哪个窗口，使用对应的颜色填充
        COLORREF color = RGB(255, 255, 255);
        if (hWnd == self->hwndH) {
          color = RGB(static_cast<BYTE>(self->config.horizontal.r),
                      static_cast<BYTE>(self->config.horizontal.g),
                      static_cast<BYTE>(self->config.horizontal.b));
        } else if (hWnd == self->hwndV) {
          color = RGB(static_cast<BYTE>(self->config.vertical.r),
                      static_cast<BYTE>(self->config.vertical.g),
                      static_cast<BYTE>(self->config.vertical.b));
        }
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
      }
      EndPaint(hWnd, &ps);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_CLOSE:
    case WM_QUIT:
    case WM_NCDESTROY:
    case WM_DESTROY:
      g_windowCount--;
      break;
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 全局鼠标钩子回调
LRESULT CALLBACK CrosshairWindow::MouseProc(const int nCode,
                                            const WPARAM wParam,
                                            const LPARAM lParam) {
  if (!g_instance || !g_instance->visible) {
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
  }
  if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE) {
    g_instance->OnMouseMove();
  }
  return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}
