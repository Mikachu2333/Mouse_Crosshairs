#pragma once

#include <d2d1.h>
#include <windows.h>

#include "config.h"

#pragma comment(lib, "d2d1.lib")

class CrosshairWindow {
 public:
  CrosshairWindow(HINSTANCE instance, const Config& config) noexcept;
  ~CrosshairWindow();

  CrosshairWindow(const CrosshairWindow&) = delete;
  CrosshairWindow& operator=(const CrosshairWindow&) = delete;

  [[nodiscard]] bool Create();
  void ToggleVisible();
  [[nodiscard]] bool IsVisible() const noexcept { return visible_; }
  [[nodiscard]] bool ApplyConfig(const Config& config);

 private:
  static constexpr UINT_PTR kUpdateTimerId = 1;
  // Win32 整数毫秒定时器无法精确表示 120 Hz；8 ms 对应最高约 125 次/秒。
  static constexpr UINT kUpdateIntervalMs = 8;

  static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM w_param,
                                  LPARAM l_param) noexcept;

  [[nodiscard]] bool InitializeDirect2D();
  void CleanupDirect2D() noexcept;
  void DestroyWindows() noexcept;
  void UpdatePosition();
  void SetWindowsVisible(bool visible) noexcept;
  [[nodiscard]] HRESULT RenderWindow(
      ID2D1HwndRenderTarget* target,
      ID2D1SolidColorBrush* brush) const noexcept;

  HINSTANCE instance_ = nullptr;
  Config config_;
  bool visible_ = true;
  bool class_registered_ = false;
  POINT last_cursor_pos_ = {LONG_MIN, LONG_MIN};

  HWND left_window_ = nullptr;
  HWND right_window_ = nullptr;
  HWND top_window_ = nullptr;
  HWND bottom_window_ = nullptr;

  ID2D1Factory* d2d_factory_ = nullptr;
  ID2D1HwndRenderTarget* left_target_ = nullptr;
  ID2D1HwndRenderTarget* right_target_ = nullptr;
  ID2D1HwndRenderTarget* top_target_ = nullptr;
  ID2D1HwndRenderTarget* bottom_target_ = nullptr;
  ID2D1SolidColorBrush* left_brush_ = nullptr;
  ID2D1SolidColorBrush* right_brush_ = nullptr;
  ID2D1SolidColorBrush* top_brush_ = nullptr;
  ID2D1SolidColorBrush* bottom_brush_ = nullptr;
};
