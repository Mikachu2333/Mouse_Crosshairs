#include "./crosshair.h"
#include <d2d1.h>

#define CLASS_NAME L"MouseCrosshairWindow"

// Static initialization
HHOOK CrosshairWindow::g_mouseHook = nullptr;
CrosshairWindow* CrosshairWindow::g_instance = nullptr;
bool CrosshairWindow::g_hookInstalled = false;
INIT_ONCE CrosshairWindow::g_criticalSectionInitOnce = INIT_ONCE_STATIC_INIT;
CRITICAL_SECTION CrosshairWindow::g_criticalSection = {};

BOOL CALLBACK CrosshairWindow::InitCriticalSectionOnce(PINIT_ONCE, PVOID,
                                                       PVOID*) {
  InitializeCriticalSection(&g_criticalSection);
  return TRUE;
}

void CrosshairWindow::EnsureSyncInitialized() {
  InitOnceExecuteOnce(&g_criticalSectionInitOnce,
                      CrosshairWindow::InitCriticalSectionOnce, nullptr,
                      nullptr);
}

CrosshairWindow::CrosshairWindow(HINSTANCE hInst, const Config& cfg)
    : hInstance(hInst),
      config(cfg),
      visible(true),
      hwndL(nullptr),
      hwndR(nullptr),
      hwndT(nullptr),
      hwndB(nullptr),
      d2dFactory(nullptr),
      renderTargetL(nullptr),
      renderTargetR(nullptr),
      renderTargetT(nullptr),
      renderTargetB(nullptr),
      brushL(nullptr),
      brushR(nullptr),
      brushT(nullptr),
      brushB(nullptr),
      lastUpdateTime(0) {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);
  g_instance = this;
  LeaveCriticalSection(&g_criticalSection);
}

CrosshairWindow::~CrosshairWindow() {
  UninstallMouseHook();
  EnsureSyncInitialized();
  bool hookStillInstalled = false;
  EnterCriticalSection(&g_criticalSection);
  hookStillInstalled = g_hookInstalled;
  if (g_instance == this) {
    g_instance = nullptr;
  }
  LeaveCriticalSection(&g_criticalSection);
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
  CleanupDirect2D();
  UnregisterClassW(CLASS_NAME, hInstance);
  if (!hookStillInstalled) {
    DeleteCriticalSection(&g_criticalSection);
  }
}

bool CrosshairWindow::InitializeDirect2D() {
  if (!hwndL || !hwndR || !hwndT || !hwndB) {
    return false;
  }

  if (FAILED(
          D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory))) {
    return false;
  }

  RECT rc;
  D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties(
      D2D1_RENDER_TARGET_TYPE_DEFAULT,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                        D2D1_ALPHA_MODE_PREMULTIPLIED));

  // Create render target for left window
  GetClientRect(hwndL, &rc);
  D2D1_HWND_RENDER_TARGET_PROPERTIES props =
      D2D1::HwndRenderTargetProperties(hwndL, D2D1::SizeU(rc.right, rc.bottom));
  if (FAILED(
          d2dFactory->CreateHwndRenderTarget(rtProps, props, &renderTargetL))) {
    CleanupDirect2D();
    return false;
  }

  // Create render target for right window
  GetClientRect(hwndR, &rc);
  props =
      D2D1::HwndRenderTargetProperties(hwndR, D2D1::SizeU(rc.right, rc.bottom));
  if (FAILED(
          d2dFactory->CreateHwndRenderTarget(rtProps, props, &renderTargetR))) {
    CleanupDirect2D();
    return false;
  }

  // Create render target for top window
  GetClientRect(hwndT, &rc);
  props =
      D2D1::HwndRenderTargetProperties(hwndT, D2D1::SizeU(rc.right, rc.bottom));
  if (FAILED(
          d2dFactory->CreateHwndRenderTarget(rtProps, props, &renderTargetT))) {
    CleanupDirect2D();
    return false;
  }

  // Create render target for bottom window
  GetClientRect(hwndB, &rc);
  props =
      D2D1::HwndRenderTargetProperties(hwndB, D2D1::SizeU(rc.right, rc.bottom));
  if (FAILED(
          d2dFactory->CreateHwndRenderTarget(rtProps, props, &renderTargetB))) {
    CleanupDirect2D();
    return false;
  }

  // Create brushes with explicit type conversions
  D2D1_COLOR_F colorH = D2D1::ColorF(
      static_cast<float>(static_cast<int>(config.horizontal.r)) / 255.0f,
      static_cast<float>(static_cast<int>(config.horizontal.g)) / 255.0f,
      static_cast<float>(static_cast<int>(config.horizontal.b)) / 255.0f, 1.0f);
  D2D1_COLOR_F colorV = D2D1::ColorF(
      static_cast<float>(static_cast<int>(config.vertical.r)) / 255.0f,
      static_cast<float>(static_cast<int>(config.vertical.g)) / 255.0f,
      static_cast<float>(static_cast<int>(config.vertical.b)) / 255.0f, 1.0f);

  // Create brushes for each render target - left horizontal
  if (FAILED(renderTargetL->CreateSolidColorBrush(colorH, &brushL))) {
    CleanupDirect2D();
    return false;
  }

  // Right horizontal
  if (FAILED(renderTargetR->CreateSolidColorBrush(colorH, &brushR))) {
    CleanupDirect2D();
    return false;
  }

  // Top vertical
  if (FAILED(renderTargetT->CreateSolidColorBrush(colorV, &brushT))) {
    CleanupDirect2D();
    return false;
  }

  // Bottom vertical
  if (FAILED(renderTargetB->CreateSolidColorBrush(colorV, &brushB))) {
    CleanupDirect2D();
    return false;
  }

  return true;
}

void CrosshairWindow::CleanupDirect2D() {
  if (renderTargetL) {
    renderTargetL->Release();
    renderTargetL = nullptr;
  }
  if (renderTargetR) {
    renderTargetR->Release();
    renderTargetR = nullptr;
  }
  if (renderTargetT) {
    renderTargetT->Release();
    renderTargetT = nullptr;
  }
  if (renderTargetB) {
    renderTargetB->Release();
    renderTargetB = nullptr;
  }
  if (brushL) {
    brushL->Release();
    brushL = nullptr;
  }
  if (brushR) {
    brushR->Release();
    brushR = nullptr;
  }
  if (brushT) {
    brushT->Release();
    brushT = nullptr;
  }
  if (brushB) {
    brushB->Release();
    brushB = nullptr;
  }
  if (d2dFactory) {
    d2dFactory->Release();
    d2dFactory = nullptr;
  }
}

HRESULT CrosshairWindow::RenderWindow(ID2D1HwndRenderTarget* target,
                                      ID2D1SolidColorBrush* brush,
                                      bool isHorizontal) const {
  if (!target || !brush) return E_INVALIDARG;

  target->BeginDraw();
  target->Clear(D2D1::ColorF(0, 0, 0, 0));

  RECT rc;
  GetClientRect(target->GetHwnd(), &rc);

  if (isHorizontal) {
    // Draw horizontal line with integer coordinates to avoid pixel alignment
    int top = (rc.bottom - static_cast<int>(config.horizontal.width)) / 2;
    int bottom = top + static_cast<int>(config.horizontal.width);
    D2D1_RECT_F rect =
        D2D1::RectF(0.0f, static_cast<float>(top), static_cast<float>(rc.right),
                    static_cast<float>(bottom));
    target->FillRectangle(rect, brush);
  } else {
    // Draw vertical line with integer coordinates to avoid pixel alignment
    int left = (rc.right - static_cast<int>(config.vertical.width)) / 2;
    int right = left + static_cast<int>(config.vertical.width);
    D2D1_RECT_F rect =
        D2D1::RectF(static_cast<float>(left), 0.0f, static_cast<float>(right),
                    static_cast<float>(rc.bottom));
    target->FillRectangle(rect, brush);
  }

  return target->EndDraw();
}

bool CrosshairWindow::InstallMouseHook() {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);
  if (g_hookInstalled) {
    LeaveCriticalSection(&g_criticalSection);
    return true;
  }
  HINSTANCE hookModule = GetModuleHandleW(nullptr);
  if (!hookModule) {
    LeaveCriticalSection(&g_criticalSection);
    return false;
  }
  HHOOK hook = SetWindowsHookExW(WH_MOUSE_LL, MouseProc, hookModule, 0);
  if (hook) {
    g_mouseHook = hook;
    g_hookInstalled = true;
  }
  const bool installed = g_hookInstalled;
  LeaveCriticalSection(&g_criticalSection);
  return installed;
}

void CrosshairWindow::UninstallMouseHook() {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);
  if (!g_hookInstalled || !g_mouseHook) {
    LeaveCriticalSection(&g_criticalSection);
    return;
  }
  UnhookWindowsHookEx(g_mouseHook);
  g_mouseHook = nullptr;
  g_hookInstalled = false;
  LeaveCriticalSection(&g_criticalSection);
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

  // Create four windows with proper error checking
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

  if (!hwndL || !hwndR || !hwndT || !hwndB) {
    if (hwndL) DestroyWindow(hwndL);
    if (hwndR) DestroyWindow(hwndR);
    if (hwndT) DestroyWindow(hwndT);
    if (hwndB) DestroyWindow(hwndB);
    hwndL = hwndR = hwndT = hwndB = nullptr;
    UnregisterClassW(CLASS_NAME, hInstance);
    return false;
  }

  // Set layered window attributes
  const BYTE alphaH =
      static_cast<BYTE>(static_cast<int>(config.horizontal.alpha));
  const BYTE alphaV =
      static_cast<BYTE>(static_cast<int>(config.vertical.alpha));
  const BOOL layeredOk =
      SetLayeredWindowAttributes(hwndL, RGB(0, 0, 0), alphaH, LWA_ALPHA) &&
      SetLayeredWindowAttributes(hwndR, RGB(0, 0, 0), alphaH, LWA_ALPHA) &&
      SetLayeredWindowAttributes(hwndT, RGB(0, 0, 0), alphaV, LWA_ALPHA) &&
      SetLayeredWindowAttributes(hwndB, RGB(0, 0, 0), alphaV, LWA_ALPHA);
  if (!layeredOk) {
    if (hwndL) DestroyWindow(hwndL);
    if (hwndR) DestroyWindow(hwndR);
    if (hwndT) DestroyWindow(hwndT);
    if (hwndB) DestroyWindow(hwndB);
    hwndL = hwndR = hwndT = hwndB = nullptr;
    UnregisterClassW(CLASS_NAME, hInstance);
    return false;
  }

  // Initialize Direct2D - cleanup on failure
  if (!InitializeDirect2D()) {
    if (hwndL) DestroyWindow(hwndL);
    if (hwndR) DestroyWindow(hwndR);
    if (hwndT) DestroyWindow(hwndT);
    if (hwndB) DestroyWindow(hwndB);
    hwndL = hwndR = hwndT = hwndB = nullptr;

    UnregisterClassW(CLASS_NAME, hInstance);
    return false;
  }

  ShowWindow(hwndL, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  ShowWindow(hwndR, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  ShowWindow(hwndT, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  ShowWindow(hwndB, visible ? SW_SHOWNOACTIVATE : SW_HIDE);

  if (!InstallMouseHook()) {
    CleanupDirect2D();
    if (hwndL) DestroyWindow(hwndL);
    if (hwndR) DestroyWindow(hwndR);
    if (hwndT) DestroyWindow(hwndT);
    if (hwndB) DestroyWindow(hwndB);
    hwndL = hwndR = hwndT = hwndB = nullptr;
    UnregisterClassW(CLASS_NAME, hInstance);
    return false;
  }

  return true;
}

void CrosshairWindow::ToggleVisible() {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);

  visible = !visible;

  if (hwndL) ShowWindow(hwndL, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndR) ShowWindow(hwndR, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndT) ShowWindow(hwndT, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
  if (hwndB) ShowWindow(hwndB, visible ? SW_SHOWNOACTIVATE : SW_HIDE);

  if (visible) {
    LeaveCriticalSection(&g_criticalSection);
    if (!InstallMouseHook()) {
      EnterCriticalSection(&g_criticalSection);
      visible = false;
      if (hwndL) ShowWindow(hwndL, SW_HIDE);
      if (hwndR) ShowWindow(hwndR, SW_HIDE);
      if (hwndT) ShowWindow(hwndT, SW_HIDE);
      if (hwndB) ShowWindow(hwndB, SW_HIDE);
      LeaveCriticalSection(&g_criticalSection);
      return;
    }
    OnMouseMove();
  } else {
    LeaveCriticalSection(&g_criticalSection);
    UninstallMouseHook();
  }
}

bool CrosshairWindow::IsVisible() const {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);
  const bool currentVisible = visible;
  LeaveCriticalSection(&g_criticalSection);
  return currentVisible;
}

void CrosshairWindow::ApplyConfig(const Config& cfg) {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);
  config = cfg;

  const BYTE alphaH =
      static_cast<BYTE>(static_cast<int>(config.horizontal.alpha));
  const BYTE alphaV =
      static_cast<BYTE>(static_cast<int>(config.vertical.alpha));
  const BOOL layeredUpdateOk =
      (!hwndL ||
       SetLayeredWindowAttributes(hwndL, RGB(0, 0, 0), alphaH, LWA_ALPHA)) &&
      (!hwndR ||
       SetLayeredWindowAttributes(hwndR, RGB(0, 0, 0), alphaH, LWA_ALPHA)) &&
      (!hwndT ||
       SetLayeredWindowAttributes(hwndT, RGB(0, 0, 0), alphaV, LWA_ALPHA)) &&
      (!hwndB ||
       SetLayeredWindowAttributes(hwndB, RGB(0, 0, 0), alphaV, LWA_ALPHA));

  const D2D1_COLOR_F colorH = D2D1::ColorF(
      static_cast<float>(static_cast<int>(config.horizontal.r)) / 255.0f,
      static_cast<float>(static_cast<int>(config.horizontal.g)) / 255.0f,
      static_cast<float>(static_cast<int>(config.horizontal.b)) / 255.0f, 1.0f);
  const D2D1_COLOR_F colorV = D2D1::ColorF(
      static_cast<float>(static_cast<int>(config.vertical.r)) / 255.0f,
      static_cast<float>(static_cast<int>(config.vertical.g)) / 255.0f,
      static_cast<float>(static_cast<int>(config.vertical.b)) / 255.0f, 1.0f);

  if (brushL) brushL->SetColor(colorH);
  if (brushR) brushR->SetColor(colorH);
  if (brushT) brushT->SetColor(colorV);
  if (brushB) brushB->SetColor(colorV);

  const bool isVisibleNow = visible;
  HWND__* const localHwndL = hwndL;
  HWND__* const localHwndR = hwndR;
  HWND__* const localHwndT = hwndT;
  HWND__* const localHwndB = hwndB;
  LeaveCriticalSection(&g_criticalSection);
  if (!layeredUpdateOk) {
    return;
  }

  if (localHwndL) InvalidateRect(localHwndL, nullptr, FALSE);
  if (localHwndR) InvalidateRect(localHwndR, nullptr, FALSE);
  if (localHwndT) InvalidateRect(localHwndT, nullptr, FALSE);
  if (localHwndB) InvalidateRect(localHwndB, nullptr, FALSE);

  if (isVisibleNow) {
    OnMouseMove();
  }
}

void CrosshairWindow::OnMouseMove() const {
  EnsureSyncInitialized();
  EnterCriticalSection(&g_criticalSection);
  const bool isVisible = visible;
  HWND__* const localHwndL = hwndL;
  HWND__* const localHwndR = hwndR;
  HWND__* const localHwndT = hwndT;
  HWND__* const localHwndB = hwndB;
  const int gap = static_cast<int>(config.gap);
  const int horizontalWidth = static_cast<int>(config.horizontal.width);
  const int verticalWidth = static_cast<int>(config.vertical.width);
  LeaveCriticalSection(&g_criticalSection);
  if (!isVisible) return;
  if (!localHwndL || !localHwndR || !localHwndT || !localHwndB) return;

  POINT pt;
  if (!GetCursorPos(&pt)) return;

  HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
  if (!hMon) {
    if (localHwndL)
      SetWindowPos(localHwndL, nullptr, 0, 0, 0, 0,
                   SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    if (localHwndR)
      SetWindowPos(localHwndR, nullptr, 0, 0, 0, 0,
                   SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    if (localHwndT)
      SetWindowPos(localHwndT, nullptr, 0, 0, 0, 0,
                   SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    if (localHwndB)
      SetWindowPos(localHwndB, nullptr, 0, 0, 0, 0,
                   SWP_NOACTIVATE | SWP_NOZORDER | SWP_HIDEWINDOW);
    return;
  }

  MONITORINFO mi = {sizeof(mi)};
  if (!GetMonitorInfo(hMon, &mi)) {
    return;
  }

  // Calculate window dimensions
  int leftW = pt.x - gap - mi.rcMonitor.left;
  if (leftW < 0) leftW = 0;

  int rightX = pt.x + gap;
  int rightW = mi.rcMonitor.right - rightX;
  if (rightW < 0) rightW = 0;

  int topH = pt.y - gap - mi.rcMonitor.top;
  if (topH < 0) topH = 0;

  int bottomY = pt.y + gap;
  int bottomH = mi.rcMonitor.bottom - bottomY;
  if (bottomH < 0) bottomH = 0;

  // Batch update windows with error checking
  HDWP hdwp = BeginDeferWindowPos(4);
  if (!hdwp) return;

  hdwp = DeferWindowPos(hdwp, localHwndL, HWND_TOPMOST, mi.rcMonitor.left,
                        pt.y - horizontalWidth / 2, leftW, horizontalWidth,
                        SWP_NOACTIVATE | SWP_SHOWWINDOW);
  if (!hdwp) return;

  hdwp = DeferWindowPos(hdwp, localHwndR, HWND_TOPMOST, rightX,
                        pt.y - horizontalWidth / 2, rightW, horizontalWidth,
                        SWP_NOACTIVATE | SWP_SHOWWINDOW);
  if (!hdwp) return;

  hdwp = DeferWindowPos(hdwp, localHwndT, HWND_TOPMOST,
                        pt.x - verticalWidth / 2, mi.rcMonitor.top,
                        verticalWidth, topH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  if (!hdwp) return;

  hdwp = DeferWindowPos(hdwp, localHwndB, HWND_TOPMOST,
                        pt.x - verticalWidth / 2, bottomY, verticalWidth,
                        bottomH, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  if (!hdwp) return;

  EndDeferWindowPos(hdwp);

  // 强制刷新，避免尺寸持续变化时出现未重绘的黑色残影
  if (localHwndL) InvalidateRect(localHwndL, nullptr, FALSE);
  if (localHwndR) InvalidateRect(localHwndR, nullptr, FALSE);
  if (localHwndT) InvalidateRect(localHwndT, nullptr, FALSE);
  if (localHwndB) InvalidateRect(localHwndB, nullptr, FALSE);
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
      if (!self) break;
      PAINTSTRUCT ps;
      BeginPaint(hWnd, &ps);

      bool isHorizontal = (hWnd == self->hwndL || hWnd == self->hwndR);
      ID2D1HwndRenderTarget* target = nullptr;
      ID2D1SolidColorBrush* brush = nullptr;

      if (hWnd == self->hwndL) {
        target = self->renderTargetL;
        brush = self->brushL;
      } else if (hWnd == self->hwndR) {
        target = self->renderTargetR;
        brush = self->brushR;
      } else if (hWnd == self->hwndT) {
        target = self->renderTargetT;
        brush = self->brushT;
      } else if (hWnd == self->hwndB) {
        target = self->renderTargetB;
        brush = self->brushB;
      }

      if (target && brush) {
        RECT rc;
        GetClientRect(hWnd, &rc);
        const UINT width = (rc.right > 0) ? static_cast<UINT>(rc.right) : 1U;
        const UINT height = (rc.bottom > 0) ? static_cast<UINT>(rc.bottom) : 1U;
        HRESULT hr = target->Resize(D2D1::SizeU(width, height));
        if (SUCCEEDED(hr)) {
          hr = self->RenderWindow(target, brush, isHorizontal);
        }
        if (hr == D2DERR_RECREATE_TARGET) {
          self->CleanupDirect2D();
          if (!self->InitializeDirect2D()) {
            InvalidateRect(hWnd, nullptr, FALSE);
          }
        }
      }

      EndPaint(hWnd, &ps);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    default:
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 全局鼠标钩子回调（带节流）
LRESULT CALLBACK CrosshairWindow::MouseProc(const int nCode,
                                            const WPARAM wParam,
                                            const LPARAM lParam) {
  EnsureSyncInitialized();
  CrosshairWindow* instance = nullptr;
  bool shouldUpdate = false;
  EnterCriticalSection(&g_criticalSection);
  if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE && g_instance &&
      g_instance->visible) {
    ULONGLONG now = GetTickCount64();
    if (now - g_instance->lastUpdateTime >= THROTTLE_MS) {
      g_instance->lastUpdateTime = now;
      instance = g_instance;
      shouldUpdate = true;
    }
  }
  LeaveCriticalSection(&g_criticalSection);
  if (shouldUpdate && instance) {
    instance->OnMouseMove();
  }
  return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}
