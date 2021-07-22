#pragma once

#include "stdafx.h"

#include "lyric_data.h"
#include "win32_util.h"

// TODO: Add sources for:
// - https://www.syair.info

class LyricSourceBase : public initquit
{
public:
    static LyricSourceBase* get(GUID guid);
    static std::vector<GUID> get_all_ids();

    void on_init() final;

    virtual const GUID& id() const = 0;
    virtual std::tstring_view friendly_name() const = 0;
    virtual bool is_local() const = 0;

    virtual std::vector<LyricDataRaw> search(metadb_handle_ptr track, abort_callback& abort) = 0;
    virtual LyricDataRaw lookup(std::string_view lookup_id, abort_callback& abort) = 0;

    virtual std::string save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) = 0;

protected:
    static std::string track_metadata(metadb_handle_ptr track, std::string_view key);
    static std::string urlencode(std::string_view input);
    static std::string_view trim_surrounding_whitespace(std::string_view str);
    static std::string_view trim_trailing_text_in_brackets(std::string_view str);
    static bool tag_values_match(std::string_view tagA, std::string_view tagB);
};

class LyricSourceRemote : public LyricSourceBase
{
public:
    bool is_local() const final;
    std::vector<LyricDataRaw> search(metadb_handle_ptr track, abort_callback& abort) final;
    std::string save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) final;

    virtual std::vector<LyricDataRaw> search(std::string_view artist, std::string_view album, std::string_view title, abort_callback& abort) = 0;
};

template<typename T>
class LyricSourceFactory : public initquit_factory_t<T> {};
