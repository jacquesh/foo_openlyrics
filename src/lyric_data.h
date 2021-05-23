#pragma once

#include "stdafx.h"

#include "preferences.h"
#include "win32_util.h"

// Raw (unparsed) lyric data
struct LyricDataRaw
{
    GUID source_id;
    std::string persistent_storage_path;
    std::string text;
};

// Parsed lyric data
struct LyricDataLine
{
    std::tstring text;
    double timestamp;
};

struct LyricData : LyricDataRaw
{
    std::vector<std::string> tags;
    std::vector<LyricDataLine> lines;
    double timestamp_offset;

    LyricData() = default;
    LyricData(const LyricData& other) = default;
    LyricData(LyricData&& other);

    bool IsTimestamped() const;
    bool IsEmpty() const;
    double LineTimestamp(int line_index) const;
    double LineTimestamp(size_t line_index) const;

    void operator =(const LyricData& other) = delete;
    void operator =(LyricData&& other);
};
