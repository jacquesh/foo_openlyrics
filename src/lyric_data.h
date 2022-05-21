#pragma once

#include "stdafx.h"

#include "preferences.h"
#include "win32_util.h"

// Raw (unparsed) lyric data
struct LyricDataRaw
{
    GUID source_id;          // The source from which the lyrics were retrieved
    std::string source_path; // The path (on the originating source) at which the lyrics were found

    std::string artist;      // The track artist, as reported by the source
    std::string album;       // The track album, as reported by the source
    std::string title;       // The track title, as reported by the source
    std::string lookup_id;   // An ID used by the source to get the lyrics text after a search. Used only temporarily during searching.
    std::string text;        // The raw lyrics text
};

// Parsed lyric data
struct LyricDataLine
{
    std::tstring text;
    double timestamp;
};

struct LyricData
{
    GUID source_id;                  // The source from which the lyrics were retrieved
    std::string source_path;         // The path (on the originating source) at which the lyrics were found

    std::optional<GUID> save_source; // The source to which the lyrics were last saved (if any)
    std::string save_path;           // The path (on the save source) at which the lyrics can be found (if they've been saved)

    std::string artist;              // The track artist, as reported by the source
    std::string album;               // The track album, as reported by the source
    std::string title;               // The track title, as reported by the source

    std::vector<std::string> tags;
    std::vector<LyricDataLine> lines;
    double timestamp_offset;

    LyricData() = default;
    LyricData(const LyricData& other) = default;
    LyricData(LyricData&& other) = default;

    bool IsTimestamped() const;
    bool IsEmpty() const;
    double LineTimestamp(int line_index) const;
    double LineTimestamp(size_t line_index) const;

    void operator =(const LyricData& other) = delete;
    LyricData& operator =(LyricData&& other) = default;
};
