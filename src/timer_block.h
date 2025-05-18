#pragma once

#include "stdafx.h"

#define TIME_FUNCTION() TimerBlock _openlyrics_function_timer(__FUNCTION__)
struct TimerBlock
{
    int64_t m_start;
    const char* m_label;

    TimerBlock(const char* label)
        : m_label(label)
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        m_start = now.QuadPart;
    }

    ~TimerBlock()
    {
        LARGE_INTEGER now;
        LARGE_INTEGER freq;
        QueryPerformanceCounter(&now);
        QueryPerformanceFrequency(&freq);

        int64_t tickdiff = now.QuadPart - m_start;
        int64_t ticks_per_us = freq.QuadPart / 1'000'000;
        int64_t us_diff = tickdiff / ticks_per_us;
        LOG_INFO("%s took %dus", m_label, int(us_diff));
    }
};
