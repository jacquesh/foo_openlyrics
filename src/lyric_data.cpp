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

double LyricData::LineTimestamp(int line_index) const
{
    if(line_index < 0) return 0.0;
    return LineTimestamp(static_cast<size_t>(line_index));
}

double LyricData::LineTimestamp(size_t line_index) const
{
    if(line_index >= lines.size()) return DBL_MAX;
    return lines[line_index].timestamp - timestamp_offset;
}

void LyricData::operator =(LyricData&& other)
{
    source_id = other.source_id;
    persistent_storage_path = std::move(other.persistent_storage_path);
    artist = std::move(other.artist);
    album = std::move(other.album);
    title = std::move(other.title);
    text = std::move(other.text);
    tags = std::move(other.tags);
    lines = std::move(other.lines);
    timestamp_offset = other.timestamp_offset;

    other.source_id = {};
    other.text.clear();
}
