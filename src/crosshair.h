#pragma once
#include <windows.h>
#include <d2d1.h>
#include "config.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

class CrosshairWindow {
 public:
  CrosshairWindow(HINSTANCE hInst, const Config& cfg);
  ~CrosshairWindow();

  bool Create();
  void ToggleVisible();
  bool IsVisible() const;
  void ApplyConfig(const Config& cfg);

  // 鼠标钩子接口
  void OnMouseMove() const;

 private:
  static HHOOK g_mouseHook;
  static CrosshairWindow* g_instance;
  static bool g_hookInstalled;
  static INIT_ONCE g_criticalSectionInitOnce;
  static CRITICAL_SECTION g_criticalSection;
  static BOOL CALLBACK InitCriticalSectionOnce(PINIT_ONCE, PVOID, PVOID*);
  static void EnsureSyncInitialized();

  static bool InstallMouseHook();
  static void UninstallMouseHook();

  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

  HINSTANCE hInstance;
  Config config;
  bool visible;

  // 我们使用四个基础窗口来独立表示留出中间空隙的横线和竖线
  HWND hwndL;  // 左侧水平线
  HWND hwndR;  // 右侧水平线
  HWND hwndT;  // 上侧垂直线
  HWND hwndB;  // 下侧垂直线

  // Direct2D resources - each render target has its own brushes
  ID2D1Factory* d2dFactory;
  ID2D1HwndRenderTarget* renderTargetL;
  ID2D1HwndRenderTarget* renderTargetR;
  ID2D1HwndRenderTarget* renderTargetT;
  ID2D1HwndRenderTarget* renderTargetB;
  ID2D1SolidColorBrush* brushL;  // Left horizontal line brush
  ID2D1SolidColorBrush* brushR;  // Right horizontal line brush
  ID2D1SolidColorBrush* brushT;  // Top vertical line brush
  ID2D1SolidColorBrush* brushB;  // Bottom vertical line brush

  // 节流相关
  mutable ULONGLONG lastUpdateTime;
  static const ULONGLONG THROTTLE_MS = 16;  // 约60fps

  bool InitializeDirect2D();
  void CleanupDirect2D();
  HRESULT RenderWindow(ID2D1HwndRenderTarget* target,
                       ID2D1SolidColorBrush* brush, bool isHorizontal) const;

  static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
};
