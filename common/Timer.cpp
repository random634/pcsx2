#include "PrecompiledHeader.h"

#include "Timer.h"
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include "RedtapeWindows.h"
#else
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#endif

namespace Common {

#ifdef _WIN32

static double s_counter_frequency;
static bool s_counter_initialized = false;

// This gets leaked... oh well.
static thread_local HANDLE s_sleep_timer;
static thread_local bool s_sleep_timer_created = false;

static HANDLE GetSleepTimer()
{
  if (s_sleep_timer_created)
    return s_sleep_timer;

  s_sleep_timer_created = true;
  s_sleep_timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
  if (!s_sleep_timer)
    std::fprintf(stderr, "CreateWaitableTimer() failed, falling back to Sleep()\n");

  return s_sleep_timer;
}

Timer::Value Timer::GetCurrentValue()
{
  // even if this races, it should still result in the same value..
  if (!s_counter_initialized)
  {
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    s_counter_frequency = static_cast<double>(Freq.QuadPart) / 1000000000.0;
    s_counter_initialized = true;
  }

  Timer::Value ReturnValue;
  QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&ReturnValue));
  return ReturnValue;
}

double Timer::ConvertValueToNanoseconds(Timer::Value value)
{
  return (static_cast<double>(value) / s_counter_frequency);
}

double Timer::ConvertValueToMilliseconds(Timer::Value value)
{
  return ((static_cast<double>(value) / s_counter_frequency) / 1000000.0);
}

double Timer::ConvertValueToSeconds(Timer::Value value)
{
  return ((static_cast<double>(value) / s_counter_frequency) / 1000000000.0);
}

Timer::Value Timer::ConvertSecondsToValue(double s)
{
  return static_cast<Value>((s * 1000000000.0) * s_counter_frequency);
}

Timer::Value Timer::ConvertMillisecondsToValue(double ms)
{
  return static_cast<Value>((ms * 1000000.0) * s_counter_frequency);
}

Timer::Value Timer::ConvertNanosecondsToValue(double ns)
{
  return static_cast<Value>(ns * s_counter_frequency);
}

void Timer::SleepUntil(Value value, bool exact)
{
  if (exact)
  {
    while (GetCurrentValue() < value)
      SleepUntil(value, false);
  }
  else
  {
    const std::int64_t diff = static_cast<std::int64_t>(value - GetCurrentValue());
    if (diff <= 0)
      return;

#ifndef _UWP
    HANDLE timer = GetSleepTimer();
    if (timer)
    {
      FILETIME ft;
      GetSystemTimeAsFileTime(&ft);

      LARGE_INTEGER fti;
      fti.LowPart = ft.dwLowDateTime;
      fti.HighPart = ft.dwHighDateTime;
      fti.QuadPart += diff;

      if (SetWaitableTimer(timer, &fti, 0, nullptr, nullptr, FALSE))
      {
        WaitForSingleObject(timer, INFINITE);
        return;
      }
    }
#endif

    // falling back to sleep... bad.
    Sleep(static_cast<DWORD>(static_cast<std::uint64_t>(diff) / 1000000));
  }
}

#else

Timer::Value Timer::GetCurrentValue()
{
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return ((Value)tv.tv_nsec + (Value)tv.tv_sec * 1000000000);
}

double Timer::ConvertValueToNanoseconds(Timer::Value value)
{
  return static_cast<double>(value);
}

double Timer::ConvertValueToMilliseconds(Timer::Value value)
{
  return (static_cast<double>(value) / 1000000.0);
}

double Timer::ConvertValueToSeconds(Timer::Value value)
{
  return (static_cast<double>(value) / 1000000000.0);
}

Timer::Value Timer::ConvertSecondsToValue(double s)
{
  return static_cast<Value>(s * 1000000000.0);
}

Timer::Value Timer::ConvertMillisecondsToValue(double ms)
{
  return static_cast<Value>(ms * 1000000.0);
}

Timer::Value Timer::ConvertNanosecondsToValue(double ns)
{
  return static_cast<Value>(ns);
}

void Timer::SleepUntil(Value value, bool exact)
{
  if (exact)
  {
    while (GetCurrentValue() < value)
      SleepUntil(value, false);
  }
  else
  {
    // Apple doesn't have TIMER_ABSTIME, so fall back to nanosleep in such a case.
#ifdef __APPLE__
    const Value current_time = GetCurrentValue();
    if (value <= current_time)
      return;

    const Value diff = value - current_time;
    struct timespec ts;
    ts.tv_sec = diff / UINT64_C(1000000000);
    ts.tv_nsec = diff % UINT64_C(1000000000);
    nanosleep(&ts, nullptr);
#else
    struct timespec ts;
    ts.tv_sec = value / UINT64_C(1000000000);
    ts.tv_nsec = value % UINT64_C(1000000000);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, nullptr);
#endif
  }
}

#endif

Timer::Timer()
{
  Reset();
}

void Timer::Reset()
{
	m_tvStartValue = GetCurrentValue();
}

double Timer::GetTimeSeconds() const
{
  return ConvertValueToSeconds(GetCurrentValue() - m_tvStartValue);
}

double Timer::GetTimeMilliseconds() const
{
  return ConvertValueToMilliseconds(GetCurrentValue() - m_tvStartValue);
}

double Timer::GetTimeNanoseconds() const
{
  return ConvertValueToNanoseconds(GetCurrentValue() - m_tvStartValue);
}

double Timer::GetTimeSecondsAndReset()
{
	const Value value = GetCurrentValue();
	const double ret = ConvertValueToSeconds(value - m_tvStartValue);
	m_tvStartValue = value;
	return ret;
}

double Timer::GetTimeMillisecondsAndReset()
{
	const Value value = GetCurrentValue();
	const double ret = ConvertValueToMilliseconds(value - m_tvStartValue);
	m_tvStartValue = value;
	return ret;
}

double Timer::GetTimeNanosecondsAndReset()
{
  const Value value = GetCurrentValue();
	const double ret = ConvertValueToNanoseconds(value - m_tvStartValue);
  m_tvStartValue = value;
  return ret;
}

void Timer::BusyWait(std::uint64_t ns)
{
  const Value start = GetCurrentValue();
  const Value end = start + ConvertNanosecondsToValue(static_cast<double>(ns));
  if (end < start)
  {
    // overflow, unlikely
    while (GetCurrentValue() > end)
      ;
  }

  while (GetCurrentValue() < end)
    ;
}

void Timer::HybridSleep(std::uint64_t ns, std::uint64_t min_sleep_time)
{
  const std::uint64_t start = GetCurrentValue();
  const std::uint64_t end = start + ConvertNanosecondsToValue(static_cast<double>(ns));
  if (end < start)
  {
    // overflow, unlikely
    while (GetCurrentValue() > end)
      ;
  }

  std::uint64_t current = GetCurrentValue();
  while (current < end)
  {
    const std::uint64_t remaining = end - current;
    if (remaining >= min_sleep_time)
      NanoSleep(min_sleep_time);

    current = GetCurrentValue();
  }
}

void Timer::NanoSleep(std::uint64_t ns)
{
#if defined(_WIN32)
  HANDLE timer = GetSleepTimer();
  if (timer)
  {
    LARGE_INTEGER due_time;
    due_time.QuadPart = -static_cast<std::int64_t>(static_cast<std::uint64_t>(ns) / 100u);
    if (SetWaitableTimer(timer, &due_time, 0, nullptr, nullptr, FALSE))
      WaitForSingleObject(timer, INFINITE);
    else
      std::fprintf(stderr, "SetWaitableTimer() failed: %08X\n", GetLastError());
  }
  else
  {
    Sleep(static_cast<std::uint32_t>(ns / 1000000));
  }
#elif defined(__ANDROID__)
  // Round down to the next millisecond.
  usleep(static_cast<useconds_t>((ns / 1000000) * 1000));
#else
  const struct timespec ts = {0, static_cast<long>(ns)};
  nanosleep(&ts, nullptr);
#endif
}

ThreadCPUTimer::ThreadCPUTimer() = default;

ThreadCPUTimer::ThreadCPUTimer(ThreadCPUTimer&& move)
  : m_thread_handle(move.m_thread_handle)
{
  move.m_thread_handle = nullptr;
}

ThreadCPUTimer::~ThreadCPUTimer()
{
#ifdef _WIN32
  CloseHandle(reinterpret_cast<HANDLE>(m_thread_handle));
#endif
}

ThreadCPUTimer& ThreadCPUTimer::operator=(ThreadCPUTimer&& move)
{
  std::swap(m_thread_handle, move.m_thread_handle);
  return *this;
}

void ThreadCPUTimer::Reset()
{
	m_start_value = GetCurrentValue();
}

ThreadCPUTimer::Value ThreadCPUTimer::GetCurrentValue() const
{
#ifdef _WIN32
	FILETIME create, exit, user, kernel;
  if (!m_thread_handle || !GetThreadTimes((HANDLE)m_thread_handle, &create, &exit, &user, &kernel))
    return 0;

  Value value = (static_cast<Value>(user.dwHighDateTime) << 32) | (static_cast<Value>(user.dwLowDateTime));
  value += (static_cast<Value>(kernel.dwHighDateTime) << 32) | (static_cast<Value>(kernel.dwLowDateTime));
  return value;
#else
	clockid_t cid;
  if (!m_thread_handle || pthread_getcpuclockid((pthread_t)m_thread_handle, &cid) != 0)
    return 0;

	struct timespec ts;
  if (clock_gettime(cid, &ts) != 0)
    return 0;

  return (static_cast<Value>(ts.tv_nsec) + static_cast<Value>(ts.tv_sec) * 1000000000LL);
#endif
}

double ThreadCPUTimer::GetTimeSeconds() const
{
  return ConvertValueToSeconds(GetCurrentValue() - m_start_value);
}

double ThreadCPUTimer::GetTimeMilliseconds() const
{
  return ConvertValueToMilliseconds(GetCurrentValue() - m_start_value);
}

double ThreadCPUTimer::GetTimeNanoseconds() const
{
  return ConvertValueToNanoseconds(GetCurrentValue() - m_start_value);
}

void ThreadCPUTimer::GetUsageInSecondsAndReset(Value time_diff, double* usage_time, double* usage_percent)
{
	const Value new_value = GetCurrentValue();
	const Value diff = new_value - m_start_value;
	m_start_value = new_value;

	*usage_time = ConvertValueToSeconds(diff);
	*usage_percent = GetUtilizationPercentage(time_diff, diff);
}

void ThreadCPUTimer::GetUsageInMillisecondsAndReset(Value time_diff, double* usage_time, double* usage_percent)
{
	const Value new_value = GetCurrentValue();
	const Value diff = new_value - m_start_value;
	m_start_value = new_value;

	*usage_time = ConvertValueToMilliseconds(diff);
	*usage_percent = GetUtilizationPercentage(time_diff, diff);
}

void ThreadCPUTimer::GetUsageInNanosecondsAndReset(Value time_diff, double* usage_time, double* usage_percent)
{
	const Value new_value = GetCurrentValue();
	const Value diff = new_value - m_start_value;
	m_start_value = new_value;

	*usage_time = ConvertValueToNanoseconds(diff);
	*usage_percent = GetUtilizationPercentage(time_diff, diff);
}

double ThreadCPUTimer::GetUtilizationPercentage(Timer::Value time_diff, Value cpu_time_diff)
{
#ifdef _WIN32
  return ((static_cast<double>(cpu_time_diff) * 10000.0) / (static_cast<double>(time_diff) / s_counter_frequency));
#else
  return (static_cast<double>(cpu_time_diff) * 100.0) / static_cast<double>(time_diff);
#endif
}

double ThreadCPUTimer::ConvertValueToSeconds(Value value)
{
#ifdef _WIN32
  // 100ns units
  return (static_cast<double>(value) / 10000000.0);
#else
	return (static_cast<double>(value) / 1000000000.0);
#endif
}

double ThreadCPUTimer::ConvertValueToMilliseconds(Value value)
{
#ifdef _WIN32
  return (static_cast<double>(value) / 10000.0);
#else
  return (static_cast<double>(value) / 1000000.0);
#endif
}

double ThreadCPUTimer::ConvertValueToNanoseconds(Value value)
{
#ifdef _WIN32
	return (static_cast<double>(value) * 100.0);
#else
  return static_cast<double>(value);
#endif
}

ThreadCPUTimer ThreadCPUTimer::GetForCallingThread()
{
	ThreadCPUTimer ret;
#ifdef _WIN32
  ret.m_thread_handle = (void*)OpenThread(THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
#else
  ret.m_thread_handle = (void*)pthread_self();
#endif
	ret.Reset();
  return ret;
}

} // namespace Common
