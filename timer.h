#pragma once
#include "util.h"
class timer
{
	int64_t m_begin_t;
public:
	timer();
	timer(const timer&) = delete;
	timer& operator= (const timer&) = delete;

	static int64_t now();
	static int64_t since_start();
	static timer s_timer;
};

