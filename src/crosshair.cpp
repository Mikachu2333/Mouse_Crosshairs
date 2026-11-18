#include "./crosshair.h"

#include <algorithm>
#include <array>

namespace {
constexpr wchar_t kClassName[] = L"MouseCrosshairWindow";
constexpr DWORD kExtendedWindowStyle = WS_EX_LAYERED | WS_EX_TRANSPARENT |
                                       WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
                                       WS_EX_NOACTIVATE;

}  // namespace

CrosshairWindow::CrosshairWindow(HINSTANCE instance,
                                 const Config& config) noexcept
    : instance_(instance), config_(config) {}

CrosshairWindow::~CrosshairWindow() {
  DestroyWindows();
  if (class_registered_) {
    UnregisterClassW(kClassName, instance_);
  }
}

void CrosshairWindow::DestroyWindows() noexcept {
  const std::array<HWND*, 4> windows = {&left_window_, &right_window_,
                                        &top_window_, &bottom_window_};
  for (HWND* window : windows) {
    if (*window != nullptr) {
      SetWindowLongPtrW(*window, GWLP_USERDATA, 0);
      DestroyWindow(*window);
      *window = nullptr;
    }
  }
}

bool CrosshairWindow::Create() {
  if (!frame_timer_.IsValid()) return false;

  WNDCLASSEXW window_class = {sizeof(window_class)};
  window_class.lpfnWndProc = WndProc;
  window_class.hInstance = instance_;
  window_class.lpszClassName = kClassName;
  window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);

  if (RegisterClassExW(&window_class) == 0) {
    return false;
  }
  class_registered_ = true;

  // 窗口先以最小尺寸隐藏创建，避免首次定位前在屏幕角落闪烁。
  auto create_window = [this]() {
    return CreateWindowExW(kExtendedWindowStyle, kClassName, L"", WS_POPUP, 0,
                           0, 1, 1, nullptr, nullptr, instance_, this);
  };
  left_window_ = create_window();
  right_window_ = create_window();
  top_window_ = create_window();
  bottom_window_ = create_window();
  if (left_window_ == nullptr || right_window_ == nullptr ||
      top_window_ == nullptr || bottom_window_ == nullptr) {
    return false;
  }

  if (!ApplyConfig(config_)) return false;
  frame_timer_.Reset();
  return true;
}

int CrosshairWindow::NextMessage(MSG& message) {
  if (!visible_) return GetMessageW(&message, nullptr, 0, 0);
  for (;;) {
    if (frame_timer_.IsDue()) {
      const bool moved = UpdatePosition();
      frame_timer_.Advance(config_.fps, moved);
    }
    if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
      return message.message == WM_QUIT ? 0 : 1;
    }
    if (!frame_timer_.WaitForFrameOrMessage()) return -1;
  }
}

bool CrosshairWindow::ApplyConfig(const Config& config) {
  config_ = config;
  if (left_window_ == nullptr) {
    return true;
  }

  const BYTE horizontal_alpha = config_.horizontal.alpha.value;
  const BYTE vertical_alpha = config_.vertical.alpha.value;
  if (!SetLayeredWindowAttributes(left_window_, 0, horizontal_alpha,
                                  LWA_ALPHA) ||
      !SetLayeredWindowAttributes(right_window_, 0, horizontal_alpha,
                                  LWA_ALPHA) ||
      !SetLayeredWindowAttributes(top_window_, 0, vertical_alpha, LWA_ALPHA) ||
      !SetLayeredWindowAttributes(bottom_window_, 0, vertical_alpha,
                                  LWA_ALPHA)) {
    return false;
  }

  last_cursor_pos_ = {LONG_MIN, LONG_MIN};
  for (HWND window :
       {left_window_, right_window_, top_window_, bottom_window_}) {
    InvalidateRect(window, nullptr, FALSE);
  }
  return true;
}

void CrosshairWindow::ToggleVisible() {
  visible_ = !visible_;
  last_cursor_pos_ = {LONG_MIN, LONG_MIN};
  frame_timer_.Reset();
  if (!visible_) HideWindows();
}

void CrosshairWindow::HideWindows() noexcept {
  for (HWND window :
       {left_window_, right_window_, top_window_, bottom_window_}) {
    ShowWindow(window, SW_HIDE);
  }
}

bool CrosshairWindow::UpdatePosition() {
  POINT cursor = {};
  if (!GetCursorPos(&cursor) ||
      (cursor.x == last_cursor_pos_.x && cursor.y == last_cursor_pos_.y)) {
    return false;
  }

  const HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONULL);
  MONITORINFO monitor_info = {sizeof(monitor_info)};
  if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
    HideWindows();
    last_cursor_pos_ = {LONG_MIN, LONG_MIN};
    return false;
  }

  const RECT bounds = monitor_info.rcMonitor;
  const int gap = static_cast<int>(config_.gap);
  const int horizontal_width = static_cast<int>(config_.horizontal.width);
  const int vertical_width = static_cast<int>(config_.vertical.width);
  const int horizontal_y =
      std::clamp(cursor.y - horizontal_width / 2, bounds.top, bounds.bottom);
  const int vertical_x =
      std::clamp(cursor.x - vertical_width / 2, bounds.left, bounds.right);
  const int horizontal_height =
      (std::min)(horizontal_width,
                 static_cast<int>(bounds.bottom - horizontal_y));
  const int vertical_size =
      (std::min)(vertical_width, static_cast<int>(bounds.right - vertical_x));
  const int left_end = std::clamp(cursor.x - gap, bounds.left, bounds.right);
  const int right_start = std::clamp(cursor.x + gap, bounds.left, bounds.right);
  const int top_end = std::clamp(cursor.y - gap, bounds.top, bounds.bottom);
  const int bottom_start =
      std::clamp(cursor.y + gap, bounds.top, bounds.bottom);

  HDWP positions = BeginDeferWindowPos(4);
  if (positions == nullptr) {
    return false;
  }
  const UINT flags = SWP_NOACTIVATE | SWP_SHOWWINDOW;
  positions = DeferWindowPos(positions, left_window_, HWND_TOPMOST, bounds.left,
                             horizontal_y, left_end - bounds.left,
                             horizontal_height, flags);
  if (positions != nullptr) {
    positions = DeferWindowPos(
        positions, right_window_, HWND_TOPMOST, right_start, horizontal_y,
        bounds.right - right_start, horizontal_height, flags);
  }
  if (positions != nullptr) {
    positions =
        DeferWindowPos(positions, top_window_, HWND_TOPMOST, vertical_x,
                       bounds.top, vertical_size, top_end - bounds.top, flags);
  }
  if (positions != nullptr) {
    positions = DeferWindowPos(positions, bottom_window_, HWND_TOPMOST,
                               vertical_x, bottom_start, vertical_size,
                               bounds.bottom - bottom_start, flags);
  }
  if (positions == nullptr || !EndDeferWindowPos(positions)) {
    HideWindows();
    last_cursor_pos_ = {LONG_MIN, LONG_MIN};
    return false;
  }
  last_cursor_pos_ = cursor;
  return true;
}

LRESULT CALLBACK CrosshairWindow::WndProc(HWND window, UINT message,
                                          WPARAM w_param,
                                          LPARAM l_param) noexcept {
  CrosshairWindow* self = reinterpret_cast<CrosshairWindow*>(
      GetWindowLongPtrW(window, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCTW*>(l_param);
    self = static_cast<CrosshairWindow*>(create->lpCreateParams);
    SetLastError(ERROR_SUCCESS);
    if (SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(self)) == 0 &&
        GetLastError() != ERROR_SUCCESS) {
      return FALSE;
    }
  }

  switch (message) {
    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
      if (self != nullptr) {
        self->last_cursor_pos_ = {LONG_MIN, LONG_MIN};
        return 0;
      }
      break;
    case WM_PAINT: {
      PAINTSTRUCT paint = {};
      const HDC device = BeginPaint(window, &paint);
      if (device != nullptr && self != nullptr) {
        const LineConfig& line =
            (window == self->left_window_ || window == self->right_window_)
                ? self->config_.horizontal
                : self->config_.vertical;
        SetDCBrushColor(device, RGB(line.r.value, line.g.value, line.b.value));
        FillRect(device, &paint.rcPaint,
                 static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
      }
      EndPaint(window, &paint);
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_NCDESTROY:
      SetWindowLongPtrW(window, GWLP_USERDATA, 0);
      break;
    default:
      break;
  }
  return DefWindowProcW(window, message, w_param, l_param);
}
