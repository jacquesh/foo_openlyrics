#pragma once

#include "stdafx.h"

#include "preferences.h"

enum class LyricFormat
{
    Unknown,
    Plaintext,
    Timestamped
};

// Raw (unparsed) lyric data
struct LyricDataRaw
{
    LyricSource source;
    LyricFormat format;
    pfc::string8 text;
};

// Parsed lyric data
struct LyricData : LyricDataRaw
{
    size_t line_count;
    TCHAR** lines;
    size_t* line_lengths;
    double* timestamps;

    LyricData();
    LyricData(const LyricData& other) = delete;
    LyricData(LyricData&& other);

    void operator =(const LyricData& other) = delete;
    void operator =(LyricData&& other);

    ~LyricData();
};
