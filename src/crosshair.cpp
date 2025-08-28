#include "./crosshair.h"

#include <gdiplus.h>

#include <algorithm>
#include <thread>
#pragma comment(lib, "gdiplus.lib")

#define CLASS_NAME L"MouseCrosshairWindow"
constexpr int INTERVAL_mSEC = 40;  // 鼠标移动事件间隔，单位毫秒

HHOOK CrosshairWindow::g_mouseHook = nullptr;
CrosshairWindow *CrosshairWindow::g_instance = nullptr;
unsigned int CrosshairWindow::g_windowCount = 0;

CrosshairWindow::CrosshairWindow(HINSTANCE hInst, const Config &cfg)
    : hInstance(hInst),
      config(cfg),
      visible(true),
      monitorsChanged(false),
      lastMousePos{0, 0},
      lastMousePosValid(false) {
  g_instance = this;
}

CrosshairWindow::~CrosshairWindow() {
  DestroyMonitorWindows();
  UninstallMouseHook();
  g_instance = nullptr;
}

void CrosshairWindow::InstallMouseHook() {
  if (g_mouseHook) return;  // 新增：幂等
  g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, nullptr, 0);
}

void CrosshairWindow::UninstallMouseHook() {
  if (!g_mouseHook) return;  // 新增：幂等
  UnhookWindowsHookEx(g_mouseHook);
  g_mouseHook = nullptr;
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

  // 枚举所有屏幕
  UpdateMonitors();

  // 为每个屏幕创建窗口
  for (auto &monitor : monitors) {
    CreateWindowForMonitor(monitor);
  }

  InstallMouseHook();

  return !monitors.empty();
}

void CrosshairWindow::UpdateMonitors() {
  // 清理旧的监视器信息
  DestroyMonitorWindows();
  monitors.clear();

  // 枚举所有监视器
  EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc,
                      reinterpret_cast<LPARAM>(this));
}

BOOL CALLBACK CrosshairWindow::MonitorEnumProc(HMONITOR hMonitor,
                                               HDC hdcMonitor,
                                               LPRECT lprcMonitor,
                                               LPARAM dwData) {
  auto *self = reinterpret_cast<CrosshairWindow *>(dwData);

  MonitorInfo info = {};
  info.hMonitor = hMonitor;
  info.rect = *lprcMonitor;
  info.hwnd = nullptr;
  info.memDC = nullptr;
  info.hBmp = nullptr;
  info.needsUpdate = true;
  info.hasMouseCursor = false;

  self->monitors.push_back(info);
  return TRUE;
}

void CrosshairWindow::CreateWindowForMonitor(MonitorInfo &monitor) {
  const int width = monitor.rect.right - monitor.rect.left;
  const int height = monitor.rect.bottom - monitor.rect.top;

  monitor.hwnd = CreateWindowExW(
      WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
          WS_EX_NOACTIVATE,
      CLASS_NAME, L"", WS_POPUP, monitor.rect.left, monitor.rect.top, width,
      height, nullptr, nullptr, hInstance, this);

  if (monitor.hwnd) {
    g_windowCount++;
    ShowWindow(monitor.hwnd, visible ? SW_SHOW : SW_HIDE);
    OnResize(monitor);
  }
}

void CrosshairWindow::DestroyMonitorWindows() {
  for (auto &monitor : monitors) {
    if (monitor.hwnd) {
      DestroyWindow(monitor.hwnd);
      monitor.hwnd = nullptr;
    }
    if (monitor.memDC) {
      DeleteDC(monitor.memDC);
      monitor.memDC = nullptr;
    }
    if (monitor.hBmp) {
      DeleteObject(monitor.hBmp);
      monitor.hBmp = nullptr;
    }
  }
}

void CrosshairWindow::ToggleVisible() {
  visible = !visible;

  for (const auto &monitor : monitors) {
    if (monitor.hwnd) {
      ShowWindow(monitor.hwnd, visible ? SW_SHOW : SW_HIDE);
    }
  }

  if (visible) {
    // 重新安装钩子并立即更新一次
    InstallMouseHook();
    OnMouseMove();
  } else {
    // 隐藏时卸载钩子，停止跟踪鼠标，降低 CPU
    UninstallMouseHook();
    // 可选：清空图层，避免残影（即便窗口已隐藏）
    for (auto &m : monitors) {
      ClearMonitor(m);
    }
  }
}

void CrosshairWindow::OnResize(MonitorInfo &monitor) {
  const int width = monitor.rect.right - monitor.rect.left;
  const int height = monitor.rect.bottom - monitor.rect.top;

  if (monitor.memDC) {
    DeleteDC(monitor.memDC);
    monitor.memDC = nullptr;
  }
  if (monitor.hBmp) {
    DeleteObject(monitor.hBmp);
    monitor.hBmp = nullptr;
  }

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void *bits = nullptr;
  const HDC screenDC = GetDC(nullptr);  // NOLINT(*-misplaced-const)
  monitor.hBmp =
      CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
  monitor.memDC = CreateCompatibleDC(screenDC);
  SelectObject(monitor.memDC, monitor.hBmp);
  ReleaseDC(nullptr, screenDC);
}

MonitorInfo *CrosshairWindow::FindMonitorContainingPoint(POINT pt) {
  for (auto &monitor : monitors) {
    if (pt.x >= monitor.rect.left && pt.x < monitor.rect.right &&
        pt.y >= monitor.rect.top && pt.y < monitor.rect.bottom) {
      return &monitor;
    }
  }
  return nullptr;
}

void CrosshairWindow::OnMouseMove() const {
  if (!visible) return;

  POINT pt;
  GetCursorPos(&pt);

  // 检查屏幕配置是否发生变化
  static DWORD lastMonitorCheck = 0;
  const DWORD currentTime = GetTickCount();
  if (currentTime - lastMonitorCheck > 2000) {
    lastMonitorCheck = currentTime;
    const DWORD currentMonitorCount = GetSystemMetrics(SM_CMONITORS);
    if (currentMonitorCount != monitors.size()) {
      const_cast<CrosshairWindow *>(this)->UpdateMonitors();
      for (auto &monitor : const_cast<CrosshairWindow *>(this)->monitors) {
        const_cast<CrosshairWindow *>(this)->CreateWindowForMonitor(monitor);
      }
    }
  }

  // 找到当前鼠标所在的屏幕
  MonitorInfo *currentMonitor = nullptr;
  for (auto &monitor : const_cast<CrosshairWindow *>(this)->monitors) {
    if (pt.x >= monitor.rect.left && pt.x < monitor.rect.right &&
        pt.y >= monitor.rect.top && pt.y < monitor.rect.bottom) {
      currentMonitor = &monitor;
      break;
    }
  }

  if (lastMousePosValid) {
    for (auto &monitor : const_cast<CrosshairWindow *>(this)->monitors) {
      if (monitor.hasMouseCursor && &monitor != currentMonitor) {
        monitor.hasMouseCursor = false;
        ClearMonitor(monitor);
      }
    }
  }

  // 更新当前屏幕
  if (currentMonitor && currentMonitor->hwnd) {
    currentMonitor->hasMouseCursor = true;
    InvalidateRect(currentMonitor->hwnd, nullptr, FALSE);
  }

  // 记录当前鼠标位置
  lastMousePos = pt;
  lastMousePosValid = true;
}

void CrosshairWindow::ClearMonitor(const MonitorInfo &monitor) {
  if (!monitor.hwnd || !monitor.memDC) return;

  const int width = monitor.rect.right - monitor.rect.left;
  const int height = monitor.rect.bottom - monitor.rect.top;

  // 清除内存DC内容
  Gdiplus::Graphics graphics(monitor.memDC);
  graphics.Clear(Gdiplus::Color(0, 0, 0, 0));

  // 更新分层窗口（完全透明）
  const HDC screenDC = GetDC(nullptr);
  POINT ptSrc = {0, 0};
  SIZE sizeWnd = {width, height};
  BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

  UpdateLayeredWindow(monitor.hwnd, screenDC, nullptr, &sizeWnd, monitor.memDC,
                      &ptSrc, 0, &blend, ULW_ALPHA);
  ReleaseDC(nullptr, screenDC);
}

LRESULT CALLBACK CrosshairWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam,
                                          LPARAM lParam) {
  CrosshairWindow *self = nullptr;
  if (msg == WM_NCCREATE) {
    const CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
    self = static_cast<CrosshairWindow *>(cs->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  } else {
    self = reinterpret_cast<CrosshairWindow *>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA));
  }

  switch (msg) {
    case WM_PAINT:
      if (self) {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);

        // 找到对应的监视器
        for (auto &monitor : self->monitors) {
          if (monitor.hwnd == hWnd && monitor.memDC) {
            self->DrawCrosshair(monitor.memDC, monitor.rect);
            break;
          }
        }

        EndPaint(hWnd, &ps);
      }
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_SIZE:
      if (self) {
        // 找到对应的监视器并调整大小
        for (auto &monitor : self->monitors) {
          if (monitor.hwnd == hWnd) {
            self->OnResize(monitor);
            break;
          }
        }
      }
      return 0;
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

void CrosshairWindow::DrawCrosshair(HDC hdc, const RECT &monitorRect) const {
  const int width = monitorRect.right - monitorRect.left;
  const int height = monitorRect.bottom - monitorRect.top;

  // 使用 GDI+ 进行抗锯齿绘制
  Gdiplus::Graphics graphics(hdc);
  graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
  graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

  POINT pt;
  GetCursorPos(&pt);

  // 将屏幕坐标转换为当前监视器的相对坐标
  pt.x -= monitorRect.left;
  pt.y -= monitorRect.top;

  // 确保鼠标在当前监视器范围内
  if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height) {
    // 如果鼠标不在当前屏幕，直接返回（保持透明）
    const HDC screenDC = GetDC(nullptr);
    POINT ptSrc = {0, 0};
    SIZE sizeWnd = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    HWND targetHwnd = nullptr;
    for (const auto &monitor : monitors) {
      if (monitor.memDC == hdc) {
        targetHwnd = monitor.hwnd;
        break;
      }
    }

    if (targetHwnd) {
      UpdateLayeredWindow(targetHwnd, screenDC, nullptr, &sizeWnd, hdc, &ptSrc,
                          0, &blend, ULW_ALPHA);
    }
    ReleaseDC(nullptr, screenDC);
    return;
  }

  // 水平线
  {
    const auto &c = config.horizontal;
    Gdiplus::Pen const pen(Gdiplus::Color(c.alpha, c.r, c.g, c.b),
                           static_cast<Gdiplus::REAL>(c.width));
    const int leftLength = std::min<int>(pt.x, width);
    const int rightLength = std::min<int>(width - pt.x, width);
    graphics.DrawLine(&pen, static_cast<Gdiplus::REAL>(pt.x - leftLength),
                      static_cast<Gdiplus::REAL>(pt.y),
                      static_cast<Gdiplus::REAL>(pt.x + rightLength),
                      static_cast<Gdiplus::REAL>(pt.y));
  }
  // 垂直线
  {
    const auto &c = config.vertical;
    Gdiplus::Pen const pen(Gdiplus::Color(c.alpha, c.r, c.g, c.b),
                           static_cast<Gdiplus::REAL>(c.width));
    const int topLength = std::min<int>(pt.y, height);
    const int bottomLength = std::min<int>(height - pt.y, height);
    graphics.DrawLine(&pen, static_cast<Gdiplus::REAL>(pt.x),
                      static_cast<Gdiplus::REAL>(pt.y - topLength),
                      static_cast<Gdiplus::REAL>(pt.x),
                      static_cast<Gdiplus::REAL>(pt.y + bottomLength));
  }

  // 更新分层窗口
  const HDC screenDC = GetDC(nullptr);
  POINT ptSrc = {0, 0};
  SIZE sizeWnd = {width, height};
  BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

  // 找到对应的窗口句柄
  HWND targetHwnd = nullptr;
  for (const auto &monitor : monitors) {
    if (monitor.memDC == hdc) {
      targetHwnd = monitor.hwnd;
      break;
    }
  }

  if (targetHwnd) {
    UpdateLayeredWindow(targetHwnd, screenDC, nullptr, &sizeWnd, hdc, &ptSrc, 0,
                        &blend, ULW_ALPHA);
  }
  ReleaseDC(nullptr, screenDC);
}

// 全局鼠标钩子回调
LRESULT CALLBACK CrosshairWindow::MouseProc(const int nCode,
                                            const WPARAM wParam,
                                            const LPARAM lParam) {
  if (!g_instance || !g_instance->visible) {  // 新增：隐藏时直接放行
    return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
  }
  static auto last = std::chrono::steady_clock::now();
  if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE && g_instance) {
    const auto now = std::chrono::steady_clock::now();
    const auto diff =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last)
            .count();
    if (diff >= INTERVAL_mSEC) {
      last = now;
      g_instance->OnMouseMove();
    }
  }
  return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}
