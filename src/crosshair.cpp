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
      hwndL(nullptr),
      hwndR(nullptr),
      hwndT(nullptr),
      hwndB(nullptr) {
  g_instance = this;
}

CrosshairWindow::~CrosshairWindow() {
  UninstallMouseHook();
  if (hwndL) {
    DestroyWindow(hwndL);
    hwndL = nullptr;
  }
  if (hwndR) {
    DestroyWindow(hwndR);
    hwndR = nullptr;
  }
  if (hwndT) {
    DestroyWindow(hwndT);
    hwndT = nullptr;
  }
  if (hwndB) {
    DestroyWindow(hwndB);
    hwndB = nullptr;
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

  // 创建四个窗口以留空隙
  hwndL = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                              WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                          CLASS_NAME, L"", WS_POPUP, vX, vY, vW,
                          static_cast<int>(config.horizontal.width), nullptr,
                          nullptr, hInstance, this);

  hwndR = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                              WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                          CLASS_NAME, L"", WS_POPUP, vX, vY, vW,
                          static_cast<int>(config.horizontal.width), nullptr,
                          nullptr, hInstance, this);

  hwndT = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                              WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                          CLASS_NAME, L"", WS_POPUP, vX, vY,
                          static_cast<int>(config.vertical.width), vH, nullptr,
                          nullptr, hInstance, this);

  hwndB = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                              WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                          CLASS_NAME, L"", WS_POPUP, vX, vY,
                          static_cast<int>(config.vertical.width), vH, nullptr,
                          nullptr, hInstance, this);

  if (hwndL && hwndR && hwndT && hwndB) {
    g_windowCount += 4;

    // 设置半透明和背景色
    SetLayeredWindowAttributes(hwndL, RGB(0, 0, 0),
                               static_cast<BYTE>(config.horizontal.alpha),
                               LWA_ALPHA);
    SetLayeredWindowAttributes(hwndR, RGB(0, 0, 0),
                               static_cast<BYTE>(config.horizontal.alpha),
                               LWA_ALPHA);
    SetLayeredWindowAttributes(hwndT, RGB(0, 0, 0),
                               static_cast<BYTE>(config.vertical.alpha),
                               LWA_ALPHA);
    SetLayeredWindowAttributes(hwndB, RGB(0, 0, 0),
                               static_cast<BYTE>(config.vertical.alpha),
                               LWA_ALPHA);

    ShowWindow(hwndL, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(hwndR, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(hwndT, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
    ShowWindow(hwndB, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  } else {
    if (hwndL) DestroyWindow(hwndL);
    if (hwndR) DestroyWindow(hwndR);
    if (hwndT) DestroyWindow(hwndT);
    if (hwndB) DestroyWindow(hwndB);
    hwndL = nullptr;
    hwndR = nullptr;
    hwndT = nullptr;
    hwndB = nullptr;
    UnregisterClassW(CLASS_NAME, hInstance);
    return false;
  }

  InstallMouseHook();

  return (hwndL != nullptr && hwndR != nullptr && hwndT != nullptr &&
          hwndB != nullptr);
}

void CrosshairWindow::ToggleVisible() {
  visible = !visible;

  if (hwndL) ShowWindow(hwndL, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndR) ShowWindow(hwndR, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndT) ShowWindow(hwndT, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndB) ShowWindow(hwndB, visible ? SW_SHOWNOACTIVATE : SW_HIDE);

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
    SetWindowPos(hwndL, nullptr, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    SetWindowPos(hwndR, nullptr, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    SetWindowPos(hwndT, nullptr, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    SetWindowPos(hwndB, nullptr, 0, 0, 0, 0,
                 SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    return;
  }

  MONITORINFO mi = {sizeof(mi)};
  if (GetMonitorInfo(hMon, &mi)) {
    // int monW = mi.rcMonitor.right - mi.rcMonitor.left;
    int monH = mi.rcMonitor.bottom - mi.rcMonitor.top;

    int gap = static_cast<int>(config.gap);

    // 计算左侧和右侧窗口的尺寸
    int leftW = pt.x - gap - mi.rcMonitor.left;
    if (leftW < 0) leftW = 0;

    int rightX = pt.x + gap;
    int rightW = mi.rcMonitor.right - rightX;
    if (rightW < 0) rightW = 0;

    // 计算上侧和下侧窗口的尺寸
    int topH = pt.y - gap - mi.rcMonitor.top;
    if (topH < 0) topH = 0;

    int bottomY = pt.y + gap;
    int bottomH = mi.rcMonitor.bottom - bottomY;
    if (bottomH < 0) bottomH = 0;

    // 更新各窗口位置
    SetWindowPos(hwndL, HWND_TOPMOST, mi.rcMonitor.left,
                 pt.y - static_cast<int>(config.horizontal.width) / 2, leftW,
                 static_cast<int>(config.horizontal.width),
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    SetWindowPos(hwndR, HWND_TOPMOST, rightX,
                 pt.y - static_cast<int>(config.horizontal.width) / 2, rightW,
                 static_cast<int>(config.horizontal.width),
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    SetWindowPos(hwndT, HWND_TOPMOST,
                 pt.x - static_cast<int>(config.vertical.width) / 2,
                 mi.rcMonitor.top, static_cast<int>(config.vertical.width),
                 topH, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    SetWindowPos(hwndB, HWND_TOPMOST,
                 pt.x - static_cast<int>(config.vertical.width) / 2, bottomY,
                 static_cast<int>(config.vertical.width), bottomH,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
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
        if (hWnd == self->hwndL || hWnd == self->hwndR) {
          color = RGB(static_cast<BYTE>(self->config.horizontal.r),
                      static_cast<BYTE>(self->config.horizontal.g),
                      static_cast<BYTE>(self->config.horizontal.b));
        } else if (hWnd == self->hwndT || hWnd == self->hwndB) {
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
