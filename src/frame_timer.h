#pragma once

#include <windows.h>

#include <chrono>

class FrameTimer {
 public:
  FrameTimer() noexcept;
  ~FrameTimer();
  FrameTimer(const FrameTimer&) = delete;
  FrameTimer& operator=(const FrameTimer&) = delete;

  [[nodiscard]] bool IsValid() const noexcept { return timer_ != nullptr; }
  void Reset() noexcept;
  [[nodiscard]] bool IsDue() const noexcept { return Clock::now() >= next_; }
  void Advance(unsigned int fps, bool moved) noexcept;
  [[nodiscard]] bool WaitForFrameOrMessage() const noexcept;

 private:
  using Clock = std::chrono::steady_clock;
  HANDLE timer_;
  Clock::time_point next_ = Clock::now();
  Clock::time_point last_motion_ = next_;
};
