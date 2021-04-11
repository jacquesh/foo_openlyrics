#include "stdafx.h"

#include "lyric_data.h"

LyricData::LyricData(LyricData&& other)
{
    *this = std::move(other);
}

bool LyricData::IsTimestamped() const
{
    return !lines.empty() && (lines[0].timestamp != DBL_MAX);
}

bool LyricData::IsEmpty() const
{
    return lines.empty();
}

void LyricData::operator =(LyricData&& other)
{
    source_id = other.source_id;
    persistent_storage_path = std::move(other.persistent_storage_path);
    text = std::move(other.text);
    tags = std::move(other.tags);
    lines = std::move(other.lines);

    other.source_id = {};
    other.text.clear();
}

LyricData::~LyricData()
{
    for(const LyricDataLine& line : lines)
    {
        delete[] line.text;
    }
}
