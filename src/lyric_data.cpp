#include "stdafx.h"

#include "lyric_data.h"

LyricDataRaw::LyricDataRaw(LyricDataCommon common)
    : LyricDataCommon(common)
{
}

LyricData::LyricData(LyricDataCommon common)
    : LyricDataCommon(common)
{
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

void LyricData::RemoveTimestamps()
{
    for(LyricDataLine& line : lines)
    {
        line.timestamp = DBL_MAX;
    }
}
