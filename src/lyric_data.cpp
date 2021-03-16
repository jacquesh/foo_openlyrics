#include "stdafx.h"

#include "lyric_data.h"

LyricData::LyricData() :
    LyricDataRaw(),
    tags(),
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
    tags = std::move(other.tags);
    line_count = other.line_count;
    lines = other.lines;
    line_lengths = other.line_lengths;
    timestamps = other.timestamps;

    other.format = LyricFormat::Unknown;
    other.source_id = {};
    other.text.clear();
    other.line_count = 0;
    other.lines = nullptr;
    other.line_lengths = nullptr;
    other.timestamps = nullptr;
}

LyricData::~LyricData()
{
    if(lines != nullptr)
    {
        for(int i=0; i<line_count; i++)
        {
            delete[] lines[i];
        }
        delete[] lines;
    }

    delete[] line_lengths;
    delete[] timestamps;
}
