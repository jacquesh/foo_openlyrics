#pragma once

#include "stdafx.h"

enum class LyricFormat
{
    Unknown,
    Plaintext,
    Timestamped
};

// Raw (unparsed) lyric data
struct LyricDataRaw
{
    LyricFormat format;
    pfc::string8 file_title;
    pfc::string8 text;
};

// Parsed lyric data
struct LyricData : LyricDataRaw
{
    size_t line_count;
    TCHAR** lines;
    size_t* line_lengths;
    double* timestamps;
};
