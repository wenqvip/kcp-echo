#pragma once
class timer
{
	long m_begin_t;
public:
	timer();
	timer(const timer&) = delete;
	timer& operator= (const timer&) = delete;

	static long now();
	static long since_start();
	static timer s_timer;
};

