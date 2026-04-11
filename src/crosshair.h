#pragma once
#include <windows.h>
#include "config.h"

class CrosshairWindow {
 public:
  CrosshairWindow(HINSTANCE hInst, const Config& cfg);
  ~CrosshairWindow();

  bool Create();
  void ToggleVisible();

  // 鼠标钩子接口
  void OnMouseMove() const;

 private:
  static HHOOK g_mouseHook;
  static CrosshairWindow* g_instance;
  static unsigned int g_windowCount;
  static bool g_hookInstalled;

  static void InstallMouseHook();
  static void UninstallMouseHook();

  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

  HINSTANCE hInstance;
  Config config;
  bool visible;

  // 我们使用两个基础窗口来独立表示横线和竖线
  HWND hwndH;
  HWND hwndV;

  static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
};
