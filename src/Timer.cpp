#include "Timer.h"

#include <cstdio>

namespace estimer {

void Timer::StartCountdown(Millis duration, Millis now)
{
    mode_ = TimerMode::Countdown;
    duration_ = duration > 0 ? duration : 0;
    accumulated_ = 0;
    startedAt_ = now;
    running_ = duration_ > 0;
    expiryNotified_ = false;
}

void Timer::StartStopwatch(Millis now)
{
    mode_ = TimerMode::Stopwatch;
    duration_ = 0;
    accumulated_ = 0;
    startedAt_ = now;
    running_ = true;
    expiryNotified_ = false;
}

void Timer::Pause(Millis now)
{
    if (!running_)
        return;
    accumulated_ += now - startedAt_;
    running_ = false;
}

void Timer::Resume(Millis now)
{
    if (running_)
        return;
    if (mode_ == TimerMode::Countdown && duration_ <= 0)
        return;
    startedAt_ = now;
    running_ = true;
}

bool Timer::Toggle(Millis now)
{
    if (running_) {
        Pause(now);
        return true;
    }
    Resume(now);
    return running_;
}

void Timer::Reset()
{
    accumulated_ = 0;
    running_ = false;
    expiryNotified_ = false;
    if (mode_ == TimerMode::Stopwatch) {
        mode_ = TimerMode::Countdown;
        duration_ = 0;
    }
}

void Timer::AddTime(Millis delta, Millis now)
{
    if (mode_ == TimerMode::Stopwatch) {
        mode_ = TimerMode::Countdown;
        duration_ = 0;
        accumulated_ = 0;
        running_ = false;
    }
    duration_ += delta;
    if (duration_ < 0)
        duration_ = 0;
    if (duration_ == 0 && running_)
        Pause(now);
    if (Remaining(now) > 0)
        expiryNotified_ = false;
}

Millis Timer::Elapsed(Millis now) const
{
    return running_ ? accumulated_ + (now - startedAt_) : accumulated_;
}

Millis Timer::Remaining(Millis now) const
{
    return duration_ - Elapsed(now);
}

bool Timer::IsExpired(Millis now) const
{
    return mode_ == TimerMode::Countdown && duration_ > 0 && Remaining(now) <= 0;
}

bool Timer::PollExpired(Millis now)
{
    if (expiryNotified_ || !IsExpired(now))
        return false;
    expiryNotified_ = true;
    return true;
}

std::string Timer::Format(Millis now) const
{
    if (mode_ == TimerMode::Stopwatch)
        return FormatSeconds(Elapsed(now) / kSecond);

    const Millis remaining = Remaining(now);
    if (duration_ == 0)
        return FormatSeconds(0);
    if (remaining > 0) // arredonda para cima: 4:59.3 aparece como 05:00
        return FormatSeconds((remaining + kSecond - 1) / kSecond);
    return "+" + FormatSeconds(-remaining / kSecond);
}

std::string FormatSeconds(Millis seconds)
{
    if (seconds < 0)
        seconds = 0;
    const long long h = seconds / 3600;
    const long long m = (seconds / 60) % 60;
    const long long s = seconds % 60;
    char buf[32];
    if (h > 0)
        std::snprintf(buf, sizeof(buf), "%lld:%02lld:%02lld", h, m, s);
    else
        std::snprintf(buf, sizeof(buf), "%02lld:%02lld", m, s);
    return buf;
}

} // namespace estimer
