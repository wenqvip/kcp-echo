#include "timer.h"

#include <chrono>

timer timer::s_timer;

timer::timer()
{
	std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock().now();
	m_begin_t = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
}

int64_t timer::now()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock().now();
	int64_t now_t = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
	return now_t;
}

int64_t timer::since_start()
{
	std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock().now();
	int64_t now_t = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
	return now_t - s_timer.m_begin_t;
}
