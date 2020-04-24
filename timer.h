#pragma once
#include "util.h"
class Timer
{
	int64_t begin_time_;
public:
	Timer();
	Timer(const Timer&) = delete;
	Timer& operator= (const Timer&) = delete;

	static int64_t now();
	static int64_t since_start();
	static Timer s_timer;
};

