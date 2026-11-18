#pragma once

#include <windows.h>

#include "config.h"

#include "frame_timer.h"

class CrosshairWindow {
 public:
  CrosshairWindow(HINSTANCE instance, const Config& config) noexcept;
  ~CrosshairWindow();

  CrosshairWindow(const CrosshairWindow&) = delete;
  CrosshairWindow& operator=(const CrosshairWindow&) = delete;

  [[nodiscard]] bool Create();
  [[nodiscard]] int NextMessage(MSG& message);
  void ToggleVisible();
  [[nodiscard]] bool IsVisible() const noexcept { return visible_; }
  [[nodiscard]] bool ApplyConfig(const Config& config);

 private:
  static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM w_param,
                                  LPARAM l_param) noexcept;

  void DestroyWindows() noexcept;
  [[nodiscard]] bool UpdatePosition();
  void HideWindows() noexcept;

  HINSTANCE instance_ = nullptr;
  Config config_;
  FrameTimer frame_timer_;
  bool visible_ = true;
  bool class_registered_ = false;
  POINT last_cursor_pos_ = {LONG_MIN, LONG_MIN};

  HWND left_window_ = nullptr;
  HWND right_window_ = nullptr;
  HWND top_window_ = nullptr;
  HWND bottom_window_ = nullptr;
};
