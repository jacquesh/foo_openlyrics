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
    GUID source_id;
    LyricFormat format;
    pfc::string8 text;
};

// Parsed lyric data
struct LyricData : LyricDataRaw
{
    std::vector<std::string> tags;
    int line_count;
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
