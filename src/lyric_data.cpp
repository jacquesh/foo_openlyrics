#include "stdafx.h"

#include "lyric_data.h"
#include "mvtf/mvtf.h"

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

static bool is_credit_or_placeholder_line(std::wstring_view line)
{
    size_t start = 0;
    while(start < line.length() && is_char_whitespace(line[start]))
    {
        start++;
    }
    size_t end = line.length();
    while(end > start && is_char_whitespace(line[end - 1]))
    {
        end--;
    }
    if(start >= end)
    {
        return true; // Empty line
    }
    std::wstring_view trimmed = line.substr(start, end - start);

    // Section header check, e.g. [Verse 1], [Chorus], (Chorus)
    if((trimmed.front() == L'[' && trimmed.back() == L']') ||
       (trimmed.front() == L'(' && trimmed.back() == L')'))
    {
        std::wstring inner;
        inner.reserve(trimmed.length());
        for(size_t i = 1; i + 1 < trimmed.length(); i++)
        {
            inner.push_back(towlower(trimmed[i]));
        }
        if(inner.find(L"verse") != std::wstring::npos ||
           inner.find(L"chorus") != std::wstring::npos ||
           inner.find(L"bridge") != std::wstring::npos ||
           inner.find(L"intro") != std::wstring::npos ||
           inner.find(L"outro") != std::wstring::npos ||
           inner.find(L"hook") != std::wstring::npos ||
           inner.find(L"refrain") != std::wstring::npos ||
           inner.find(L"interlude") != std::wstring::npos ||
           inner.find(L"solo") != std::wstring::npos ||
           inner.find(L"instrumental") != std::wstring::npos)
        {
            return true;
        }
    }

    std::wstring lower;
    lower.reserve(trimmed.length());
    for(wchar_t c : trimmed)
    {
        lower.push_back(towlower(c));
    }

    // List of credit / metadata prefixes
    static const wchar_t* const credit_prefixes[] = {
        L"作词", L"作詞", L"作曲", L"编曲", L"編曲",
        L"制作人", L"製作人", L"制作:", L"制作：", L"製作:", L"製作：",
        L"监制", L"監制", L"混音", L"母带", L"母帶",
        L"录音", L"錄音", L"和声", L"和聲", L"伴唱",
        L"吉他", L"贝斯", L"貝斯", L"鼓", L"键盘", L"鍵盤",
        L"弦乐", L"管乐", L"打击乐", L"合唱",
        L"发行", L"出品", L"企划", L"企劃", L"统筹", L"統籌", L"文案", L"封面",
        L"op:", L"op：", L"sp:", L"sp：",
        L"lyrics by", L"lyricist", L"written by", L"composed by", L"composer",
        L"produced by", L"producer", L"arranged by", L"arranger",
        L"mixed by", L"mixing", L"mastered by", L"mastering",
        L"recorded by", L"recording", L"music by", L"words by",
        L"engineered by", L"vocals by", L"backing vocals"
    };

    for(const wchar_t* prefix : credit_prefixes)
    {
        size_t prefix_len = wcslen(prefix);
        if(lower.length() >= prefix_len && lower.compare(0, prefix_len, prefix) == 0)
        {
            return true;
        }
    }

    // List of instrumental or placeholder full lines / phrases
    static const wchar_t* const placeholder_patterns[] = {
        L"纯音乐", L"純音樂", L"没有填词", L"沒有填詞", L"暂无歌词", L"暫無歌詞",
        L"无歌词", L"無歌詞", L"instrumental", L"no lyrics",
        L"we do not have the lyrics", L"qq音乐", L"网易云音乐", L"網易雲音樂"
    };

    for(const wchar_t* pattern : placeholder_patterns)
    {
        if(lower.find(pattern) != std::wstring::npos)
        {
            return true;
        }
    }

    return false;
}

bool has_substantive_lyrics(const LyricData& lyrics)
{
    if(lyrics.IsEmpty())
    {
        return false;
    }

    size_t substantive_line_count = 0;
    for(const LyricDataLine& line : lyrics.lines)
    {
        if(!is_credit_or_placeholder_line(line.text))
        {
            substantive_line_count++;
        }
    }

    return substantive_line_count > 0;
}

static bool is_cjk(wchar_t c)
{
    return (c >= 0x4E00 && c <= 0x9FFF) ||
           (c >= 0x3400 && c <= 0x4DBF) ||
           (c >= 0x20000 && c <= 0x2A6DF) ||
           (c >= 0x3040 && c <= 0x309F) ||
           (c >= 0x30A0 && c <= 0x30FF) ||
           (c >= 0xAC00 && c <= 0xD7AF) ||
           (c >= 0x1100 && c <= 0x11FF) ||
           (c >= 0x3100 && c <= 0x312F);
}

static std::vector<std::wstring> get_substantive_tokens(const LyricData& lyrics)
{
    std::vector<std::wstring> tokens;
    for(const LyricDataLine& line : lyrics.lines)
    {
        if(is_credit_or_placeholder_line(line.text))
        {
            continue;
        }

        std::wstring current_word;
        for(wchar_t c : line.text)
        {
            if(is_cjk(c))
            {
                if(!current_word.empty())
                {
                    tokens.push_back(std::move(current_word));
                    current_word.clear();
                }
                tokens.emplace_back(1, c);
            }
            else if(iswalnum(c))
            {
                current_word.push_back(towlower(c));
            }
            else
            {
                if(!current_word.empty())
                {
                    tokens.push_back(std::move(current_word));
                    current_word.clear();
                }
            }
        }
        if(!current_word.empty())
        {
            tokens.push_back(std::move(current_word));
        }
    }
    return tokens;
}

static std::vector<std::wstring> get_normalized_substantive_lines(const LyricData& lyrics)
{
    std::vector<std::wstring> result;
    for(const LyricDataLine& line : lyrics.lines)
    {
        if(is_credit_or_placeholder_line(line.text))
        {
            continue;
        }

        std::wstring norm;
        for(wchar_t c : line.text)
        {
            if(iswalnum(c) || is_cjk(c))
            {
                norm.push_back(towlower(c));
            }
        }
        if(norm.length() >= 2)
        {
            result.push_back(std::move(norm));
        }
    }
    return result;
}

bool are_lyrics_for_same_song(const LyricData& candidate, const LyricData& existing)
{
    if(!has_substantive_lyrics(candidate) || !has_substantive_lyrics(existing))
    {
        return false;
    }

    const std::vector<std::wstring> cand_norm_lines = get_normalized_substantive_lines(candidate);
    const std::vector<std::wstring> exist_norm_lines = get_normalized_substantive_lines(existing);

    // If existing song has a substantive set of lines, but candidate only has a tiny fragment, reject.
    if(exist_norm_lines.size() >= 8 && cand_norm_lines.size() <= 2)
    {
        return false;
    }

    std::unordered_map<std::wstring, int> cand_tokens;
    int total_cand_tokens = 0;
    for(const std::wstring& t : get_substantive_tokens(candidate))
    {
        cand_tokens[t]++;
        total_cand_tokens++;
    }

    std::unordered_map<std::wstring, int> exist_tokens;
    int total_exist_tokens = 0;
    for(const std::wstring& t : get_substantive_tokens(existing))
    {
        exist_tokens[t]++;
        total_exist_tokens++;
    }

    if(total_cand_tokens == 0 || total_exist_tokens == 0)
    {
        return false;
    }

    int matched_tokens = 0;
    for(const auto& [token, count] : cand_tokens)
    {
        auto iter = exist_tokens.find(token);
        if(iter != exist_tokens.end())
        {
            matched_tokens += std::min(count, iter->second);
        }
    }

    const int min_required_matches = (total_exist_tokens >= 30) ? 10 : 4;
    if(matched_tokens < min_required_matches)
    {
        return false;
    }

    int matched_lines = 0;
    for(const std::wstring& c_line : cand_norm_lines)
    {
        for(const std::wstring& e_line : exist_norm_lines)
        {
            if(c_line == e_line)
            {
                matched_lines++;
                break;
            }
            if(c_line.length() >= 6 && e_line.length() >= 6)
            {
                if((c_line.find(e_line) != std::wstring::npos) || (e_line.find(c_line) != std::wstring::npos))
                {
                    matched_lines++;
                    break;
                }
            }
        }
    }

    const double cand_token_ratio = double(matched_tokens) / double(total_cand_tokens);
    const double exist_token_ratio = double(matched_tokens) / double(total_exist_tokens);
    const double line_ratio = (!cand_norm_lines.empty()) ? double(matched_lines) / double(cand_norm_lines.size()) : 0.0;

    const bool token_match = (cand_token_ratio >= 0.40 && exist_token_ratio >= 0.25) || (cand_token_ratio >= 0.60);
    const bool line_match = (line_ratio >= 0.35);

    return token_match || line_match;
}

#if MVTF_TESTS_ENABLED
MVTF_TEST(has_substantive_lyrics_false_for_empty)
{
    LyricData lyrics;
    ASSERT(!has_substantive_lyrics(lyrics));
}

MVTF_TEST(has_substantive_lyrics_false_for_netease_composers_only)
{
    LyricData lyrics;
    lyrics.lines.push_back({ _T("作词 : Bob Dylan"), 0.0 });
    lyrics.lines.push_back({ _T("作曲 : Bob Dylan"), 1.0 });
    lyrics.lines.push_back({ _T("编曲 : Bob Dylan"), 2.0 });
    ASSERT(!has_substantive_lyrics(lyrics));
}

MVTF_TEST(has_substantive_lyrics_false_for_instrumental_placeholder)
{
    LyricData lyrics;
    lyrics.lines.push_back({ _T("纯音乐，请欣赏"), 0.0 });
    ASSERT(!has_substantive_lyrics(lyrics));
}

MVTF_TEST(has_substantive_lyrics_true_for_real_lyrics)
{
    LyricData lyrics;
    lyrics.lines.push_back({ _T("Come gather 'round people wherever you roam"), 5.0 });
    lyrics.lines.push_back({ _T("And admit that the waters around you have grown"), 10.0 });
    ASSERT(has_substantive_lyrics(lyrics));
}

MVTF_TEST(has_substantive_lyrics_true_for_lyrics_with_credits_at_top)
{
    LyricData lyrics;
    lyrics.lines.push_back({ _T("作词 : Bob Dylan"), 0.0 });
    lyrics.lines.push_back({ _T("作曲 : Bob Dylan"), 1.0 });
    lyrics.lines.push_back({ _T("Come gather 'round people wherever you roam"), 5.0 });
    lyrics.lines.push_back({ _T("And admit that the waters around you have grown"), 10.0 });
    ASSERT(has_substantive_lyrics(lyrics));
}

MVTF_TEST(are_lyrics_for_same_song_confirmed_for_matching_english_lyrics)
{
    LyricData unsynced;
    unsynced.lines.push_back({ _T("Yesterday, all my troubles seemed so far away."), DBL_MAX });
    unsynced.lines.push_back({ _T("Now it looks as though they're here to stay."), DBL_MAX });
    unsynced.lines.push_back({ _T("Oh, I believe in yesterday."), DBL_MAX });
    unsynced.lines.push_back({ _T("Suddenly, I'm not half the man I used to be."), DBL_MAX });
    unsynced.lines.push_back({ _T("There's a shadow hanging over me."), DBL_MAX });
    unsynced.lines.push_back({ _T("Oh, yesterday came suddenly."), DBL_MAX });

    LyricData synced;
    synced.lines.push_back({ _T("作词 : Paul McCartney"), 0.0 });
    synced.lines.push_back({ _T("作曲 : John Lennon"), 1.0 });
    synced.lines.push_back({ _T("Yesterday all my troubles seemed so far away"), 5.0 });
    synced.lines.push_back({ _T("Now it looks as though they're here to stay"), 9.0 });
    synced.lines.push_back({ _T("Oh I believe in yesterday"), 14.0 });
    synced.lines.push_back({ _T("Suddenly I'm not half the man I used to be"), 19.0 });
    synced.lines.push_back({ _T("There's a shadow hanging over me"), 24.0 });
    synced.lines.push_back({ _T("Oh yesterday came suddenly"), 29.0 });

    ASSERT(are_lyrics_for_same_song(synced, unsynced));
}

MVTF_TEST(are_lyrics_for_same_song_rejected_for_netease_composers_only)
{
    LyricData unsynced;
    unsynced.lines.push_back({ _T("Yesterday, all my troubles seemed so far away."), DBL_MAX });
    unsynced.lines.push_back({ _T("Now it looks as though they're here to stay."), DBL_MAX });
    unsynced.lines.push_back({ _T("Oh, I believe in yesterday."), DBL_MAX });
    unsynced.lines.push_back({ _T("Suddenly, I'm not half the man I used to be."), DBL_MAX });

    LyricData netease_composer_only;
    netease_composer_only.lines.push_back({ _T("作词 : Paul McCartney"), 0.0 });
    netease_composer_only.lines.push_back({ _T("作曲 : John Lennon"), 1.0 });

    ASSERT(!are_lyrics_for_same_song(netease_composer_only, unsynced));
}

MVTF_TEST(are_lyrics_for_same_song_rejected_for_different_song)
{
    LyricData unsynced;
    unsynced.lines.push_back({ _T("Yesterday, all my troubles seemed so far away."), DBL_MAX });
    unsynced.lines.push_back({ _T("Now it looks as though they're here to stay."), DBL_MAX });
    unsynced.lines.push_back({ _T("Oh, I believe in yesterday."), DBL_MAX });
    unsynced.lines.push_back({ _T("Suddenly, I'm not half the man I used to be."), DBL_MAX });

    LyricData synced_different_song;
    synced_different_song.lines.push_back({ _T("I can't remember anything"), 5.0 });
    synced_different_song.lines.push_back({ _T("Can't tell if this is true or dream"), 10.0 });
    synced_different_song.lines.push_back({ _T("Deep down inside I feel to scream"), 15.0 });
    synced_different_song.lines.push_back({ _T("This terrible silence stops me"), 20.0 });

    ASSERT(!are_lyrics_for_same_song(synced_different_song, unsynced));
}

MVTF_TEST(are_lyrics_for_same_song_confirmed_for_chinese_lyrics)
{
    LyricData unsynced;
    unsynced.lines.push_back({ _T("快使用双截棍 哼哼哈兮"), DBL_MAX });
    unsynced.lines.push_back({ _T("快使用双截棍 哼哼哈兮"), DBL_MAX });
    unsynced.lines.push_back({ _T("习武之人切记 仁者无敌"), DBL_MAX });
    unsynced.lines.push_back({ _T("是谁在练太极 风生水起"), DBL_MAX });

    LyricData synced;
    synced.lines.push_back({ _T("作词 : 方文山"), 0.0 });
    synced.lines.push_back({ _T("作曲 : 周杰伦"), 1.0 });
    synced.lines.push_back({ _T("快使用双截棍 哼哼哈兮"), 10.0 });
    synced.lines.push_back({ _T("快使用双截棍 哼哼哈兮"), 15.0 });
    synced.lines.push_back({ _T("习武之人切记 仁者无敌"), 20.0 });
    synced.lines.push_back({ _T("是谁在练太极 风生水起"), 25.0 });

    ASSERT(are_lyrics_for_same_song(synced, unsynced));
}

MVTF_TEST(are_lyrics_for_same_song_confirmed_with_genius_section_headers)
{
    LyricData unsynced;
    unsynced.lines.push_back({ _T("[Verse 1]"), DBL_MAX });
    unsynced.lines.push_back({ _T("Come gather 'round people wherever you roam"), DBL_MAX });
    unsynced.lines.push_back({ _T("And admit that the waters around you have grown"), DBL_MAX });
    unsynced.lines.push_back({ _T("[Chorus]"), DBL_MAX });
    unsynced.lines.push_back({ _T("For the times they are a-changin'"), DBL_MAX });

    LyricData synced;
    synced.lines.push_back({ _T("Come gather 'round people wherever you roam"), 5.0 });
    synced.lines.push_back({ _T("And admit that the waters around you have grown"), 10.0 });
    synced.lines.push_back({ _T("For the times they are a-changin'"), 15.0 });

    ASSERT(are_lyrics_for_same_song(synced, unsynced));
}

#endif
