#pragma once

#include "stdafx.h"

#include "lyric_data.h"

namespace sources
{
    GUID GetSaveSource();
    void SaveLyrics(metadb_handle_ptr track, const LyricData& lyrics, abort_callback& abort);
}

// TODO: Add sources for:
// - https://www.syair.info

class LyricSourceBase : public initquit
{
public:
    static LyricSourceBase* get(GUID guid);
    static std::vector<GUID> get_all_ids();

    virtual void on_init();

    virtual const GUID& id() const = 0;
    virtual const TCHAR* friendly_name() const = 0;
    virtual bool can_save() const = 0;
    virtual LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) = 0;
    virtual void save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, abort_callback& abort) = 0;

protected:
    const char* get_artist(metadb_handle_ptr track) const;
    const char* get_album(metadb_handle_ptr track) const;
    const char* get_title(metadb_handle_ptr track) const;

    pfc::string8 trim_surrounding_whitespace(const char* str) const;
};

class LyricSourceRemote : public LyricSourceBase
{
    bool can_save() const final;
    void save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, abort_callback& abort) final;
};

template<typename T>
class LyricSourceFactory : public initquit_factory_t<T> {};
