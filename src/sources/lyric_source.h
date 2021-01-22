#pragma once

#include "stdafx.h"

#include "lyric_data.h"

// TODO: Add sources:
// - id3 tags
// - genius.com

class LyricSourceBase : public initquit
{
public:
    static LyricSourceBase* get(GUID guid);
    static std::vector<GUID> get_all_ids();

    virtual void on_init();

    virtual const GUID& id() const = 0;
    virtual const TCHAR* friendly_name() const = 0;
    virtual LyricDataRaw query(metadb_handle_ptr track, abort_callback& abort) = 0;

protected:
    const char* get_artist(metadb_handle_ptr track);
    const char* get_album(metadb_handle_ptr track);
    const char* get_title(metadb_handle_ptr track);
};

template<typename T>
class LyricSourceFactory : public initquit_factory_t<T> {};
