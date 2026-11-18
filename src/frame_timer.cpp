#include "./frame_timer.h"

#include <algorithm>
#include <cstdint>

FrameTimer::FrameTimer() noexcept
    : timer_(CreateWaitableTimerExW(nullptr, nullptr,
                                    CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                    TIMER_MODIFY_STATE | SYNCHRONIZE)) {
  if (timer_ == nullptr && GetLastError() == ERROR_INVALID_PARAMETER) {
    timer_ = CreateWaitableTimerW(nullptr, FALSE, nullptr);
  }
}

FrameTimer::~FrameTimer() {
  if (timer_ != nullptr) CloseHandle(timer_);
}

void FrameTimer::Reset() noexcept {
  CancelWaitableTimer(timer_);
  next_ = last_motion_ = Clock::now();
}

void FrameTimer::Advance(unsigned int fps, bool moved) noexcept {
  const auto now = Clock::now();
  if (moved) last_motion_ = now;
  if (now - last_motion_ >= std::chrono::milliseconds(150)) {
    fps = (std::min)(fps, 30U);
  }
  const auto period = std::chrono::round<Clock::duration>(
      std::chrono::duration<double>(1.0 / fps));
  next_ += period * ((now - next_) / period + 1);
}

bool FrameTimer::WaitForFrameOrMessage() const noexcept {
  using TimerTick = std::chrono::duration<std::int64_t, std::ratio<1, 10000000>>;
  const auto delay = std::chrono::ceil<TimerTick>(next_ - Clock::now());
  if (delay.count() <= 0) return true;
  LARGE_INTEGER due = {};
  due.QuadPart = -delay.count();
  if (!SetWaitableTimer(timer_, &due, 0, nullptr, nullptr, FALSE)) return false;
  const DWORD result = MsgWaitForMultipleObjectsEx(
      1, &timer_, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
  return result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0 + 1;
}
