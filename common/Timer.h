#pragma once
#include <cstdint>

namespace Common {

class Timer
{
public:
  using Value = std::uint64_t;

  Timer();

  static Value GetCurrentValue();
  static double ConvertValueToSeconds(Value value);
  static double ConvertValueToMilliseconds(Value value);
  static double ConvertValueToNanoseconds(Value value);
  static Value ConvertSecondsToValue(double s);
  static Value ConvertMillisecondsToValue(double s);
  static Value ConvertNanosecondsToValue(double ns);
  static void BusyWait(std::uint64_t ns);
  static void HybridSleep(std::uint64_t ns, std::uint64_t min_sleep_time = UINT64_C(2000000));
  static void NanoSleep(std::uint64_t ns);
  static void SleepUntil(Value value, bool exact);

  void Reset();
  void ResetTo(Value value) { m_tvStartValue = value; }

  Value GetStartValue() const { return m_tvStartValue; }

  double GetTimeSeconds() const;
  double GetTimeMilliseconds() const;
  double GetTimeNanoseconds() const;

  double GetTimeSecondsAndReset();
  double GetTimeMillisecondsAndReset();
  double GetTimeNanosecondsAndReset();

private:
  Value m_tvStartValue;
};

class ThreadCPUTimer
{
public:
	using Value = std::uint64_t;

  ThreadCPUTimer();
  ThreadCPUTimer(ThreadCPUTimer&& move);
  ThreadCPUTimer(const ThreadCPUTimer&) = delete;
  ~ThreadCPUTimer();

  void Reset();
  void ResetTo(Value value) { m_start_value = value; }

  Value GetStartValue() const { return m_start_value; }
  Value GetCurrentValue() const;

  double GetTimeSeconds() const;
  double GetTimeMilliseconds() const;
  double GetTimeNanoseconds() const;

  void GetUsageInSecondsAndReset(Value time_diff, double* usage_time, double* usage_percent);
  void GetUsageInMillisecondsAndReset(Value time_diff, double* usage_time, double* usage_percent);
  void GetUsageInNanosecondsAndReset(Value time_diff, double* usage_time, double* usage_percent);

  static double GetUtilizationPercentage(Timer::Value time_diff, Value cpu_time_diff);

  static double ConvertValueToSeconds(Value value);
  static double ConvertValueToMilliseconds(Value value);
  static double ConvertValueToNanoseconds(Value value);

  static ThreadCPUTimer GetForCallingThread();

  ThreadCPUTimer& operator=(const ThreadCPUTimer&) = delete;
  ThreadCPUTimer& operator=(ThreadCPUTimer&& move);

private:
  void* m_thread_handle = nullptr;
  Value m_start_value = 0;
};

} // namespace Common