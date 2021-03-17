#include "stdafx.h"

#include "lyric_data.h"

LyricData::LyricData(LyricData&& other)
{
    *this = std::move(other);
}

void LyricData::operator =(LyricData&& other)
{
    format = other.format;
    source_id = other.source_id;
    text = other.text;
    tags = std::move(other.tags);
    lines = std::move(other.lines);

    other.format = LyricFormat::Unknown;
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
