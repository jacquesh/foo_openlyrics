#pragma once

#include "stdafx.h"

#include "lyric_data.h"
#include "win32_util.h"

// TODO: Add sources for:
// - https://www.syair.info
// - MiniLyrics (https://crintsoft.com/) - See a wireshark trace of LyricShowPanel3 attempting to make HTTP calls

struct LyricSearchParams
{
    std::string artist;
    std::string album;
    std::string title;
    std::optional<int> duration_sec;

    LyricSearchParams(std::string in_artist, std::string in_album, std::string in_title, std::optional<int> in_duration_sec)
        : artist(std::move(in_artist))
        , album(std::move(in_album))
        , title(std::move(in_title))
        , duration_sec(in_duration_sec)
    {}
};

class LyricSourceBase : public initquit
{
public:
    static LyricSourceBase* get(GUID guid);
    static std::vector<GUID> get_all_ids();

    void on_init() final;

    virtual const GUID& id() const = 0;
    virtual std::tstring_view friendly_name() const = 0;
    virtual bool is_local() const = 0;

    virtual std::vector<LyricDataRaw> search(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, abort_callback& abort) = 0;

    // Lookup any outstanding data in the given lyric data, modifying it in-place.
    // Returns true on success, otherwise false.
    virtual bool lookup(LyricDataRaw& data, abort_callback& abort) = 0;

    virtual std::string save(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) = 0;
    virtual bool delete_persisted(metadb_handle_ptr track, const std::string& path) = 0;

    virtual std::tstring get_file_path(metadb_handle_ptr track, const LyricData& lyrics) = 0;

protected:
    static std::string urlencode(std::string_view input);
    static std::vector<uint8_t> string_to_raw_bytes(std::string_view str);
};

class LyricSourceRemote : public LyricSourceBase
{
public:
    bool is_local() const final;
    std::vector<LyricDataRaw> search(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, abort_callback& abort) final;
    std::string save(metadb_handle_ptr track, const metadb_v2_rec_t& track_info, bool is_timestamped, std::string_view lyrics, bool allow_overwrite, abort_callback& abort) final;
    bool delete_persisted(metadb_handle_ptr track, const std::string& path) final;
    std::tstring get_file_path(metadb_handle_ptr track, const LyricData& lyrics) final;

    virtual std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) = 0;
};

template<typename T>
class LyricSourceFactory : public initquit_factory_t<T> {};


// NOTE: We need access to this one function from outside the normal lyric-source interaction flow
std::string musixmatch_get_token(abort_callback& abort);

