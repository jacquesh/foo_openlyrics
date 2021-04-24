#pragma once

#include "stdafx.h"

#include "lyric_data.h"
#include "winstr_util.h"

// TODO: Add sources for:
// - https://www.syair.info

class LyricSourceBase : public initquit
{
public:
    static LyricSourceBase* get(GUID guid);
    static std::vector<GUID> get_all_ids();

    virtual void on_init();

    virtual const GUID& id() const = 0;
    virtual std::tstring_view friendly_name() const = 0;
    virtual bool can_save() const = 0;
    virtual LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) = 0;
    virtual std::string save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) = 0;

protected:
    const char* get_artist(metadb_handle_ptr track) const;
    const char* get_album(metadb_handle_ptr track) const;
    const char* get_title(metadb_handle_ptr track) const;

    std::string trim_surrounding_whitespace(const char* str) const;
};

class LyricSourceRemote : public LyricSourceBase
{
    bool can_save() const final;
    std::string save(metadb_handle_ptr track, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) final;
};

template<typename T>
class LyricSourceFactory : public initquit_factory_t<T> {};
