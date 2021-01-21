#include "stdafx.h"

#include "lyric_data.h"

LyricData::LyricData() :
    LyricDataRaw(),
    line_count(0),
    lines(nullptr),
    line_lengths(nullptr),
    timestamps(nullptr)
{
}

LyricData::LyricData(LyricData&& other)
{
    *this = std::move(other);
}

void LyricData::operator =(LyricData&& other)
{
    format = other.format;
    source_id = other.source_id;
    text = other.text;
    line_count = other.line_count;
    lines = other.lines;
    line_lengths = other.line_lengths;
    timestamps = other.timestamps;

    other.format = LyricFormat::Unknown;
    other.source_id = {};
    other.text.reset();
    other.line_count = 0;
    other.lines = nullptr;
    other.line_lengths = nullptr;
    other.timestamps = nullptr;
}

LyricData::~LyricData()
{
    if(lines != nullptr)
    {
        for(size_t i=0; i<line_count; i++)
        {
            delete[] lines[i];
        }
        delete[] lines;
    }

    delete[] line_lengths;
    delete[] timestamps;
}
