#include "timer.h"

#include <chrono>

Timer Timer::s_timer;

Timer::Timer()
{
	std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock().now();
	begin_time_ = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
}

int64_t Timer::now()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock().now();
	int64_t now_t = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
	return now_t;
}

int64_t Timer::since_start()
{
	std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock().now();
	int64_t now_t = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
	return now_t - s_timer.begin_time_;
}
