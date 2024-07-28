#include "stdafx.h"

#include "cJSON.h"

#include "hash_utils.h"
#include "logging.h"
#include "lyric_data.h"
#include "lyric_source.h"
#include "mvtf/mvtf.h"
#include "openlyrics_version.h"
#include "parsers.h"

static const GUID src_guid = { 0x9b4be445, 0x9a38, 0x4342, { 0xab, 0x72, 0x6f, 0x55, 0x8c, 0x4, 0x4d, 0xc0 } };

class LrclibLyricsSource : public LyricSourceRemote
{
    const GUID& id() const final { return src_guid; }
    std::tstring_view friendly_name() const final { return _T("LRCLIB"); }

    std::vector<LyricDataRaw> search(const LyricSearchParams& params, abort_callback& abort) final;
    bool lookup(LyricDataRaw& data, abort_callback& abort) final;

    bool supports_upload() const final { return true; }
    void upload(const LyricData& lyrics, abort_callback& abort) final;
};
static const LyricSourceFactory<LrclibLyricsSource> src_factory;

static const char* g_api_url = "https://lrclib.net/api/";

std::vector<LyricDataRaw> LrclibLyricsSource::search(const LyricSearchParams& params, abort_callback& abort)
{
    std::string url = std::string(g_api_url) + "search";
    url += "?artist_name=" + urlencode(params.artist);
    url += "&album_name=" + urlencode(params.album);
    url += "&track_name=" + urlencode(params.title);
    LOG_INFO("Querying for track ID from %s", url.c_str());

    pfc::string8 content;
    try
    {
        http_request::ptr request = http_client::get()->create_request("GET");
        request->add_header("User-Agent", "foo_openlyrics v" OPENLYRICS_VERSION " (https://github.com/jacquesh/foo_openlyrics)");
        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to make LRCLIB search request to %s: %s", url.c_str(), e.what());
        return {};
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json == nullptr) || (json->type != cJSON_Array))
    {
        LOG_WARN("Received LRCLIB search result but root was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    std::vector<LyricDataRaw> results;
    cJSON* json_result = nullptr;
    cJSON_ArrayForEach(json_result, json)
    {
        if((json_result == nullptr) || (json_result->type != cJSON_Object))
        {
            LOG_WARN("Received LRCLIB search result but track was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_title = cJSON_GetObjectItem(json_result, "trackName");
        if((json_title == nullptr) || (json_title->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but trackName was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_artist = cJSON_GetObjectItem(json_result, "artistName");
        if((json_artist == nullptr) || (json_artist->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but artistName was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_album = cJSON_GetObjectItem(json_result, "albumName");
        if((json_album == nullptr) || (json_album->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but albumName was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_plainlyrics = cJSON_GetObjectItem(json_result, "plainLyrics");
        if((json_plainlyrics == nullptr) || (json_plainlyrics->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but plainLyrics was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_syncedlyrics = cJSON_GetObjectItem(json_result, "syncedLyrics");
        if((json_syncedlyrics == nullptr) || (json_syncedlyrics->type != cJSON_String))
        {
            LOG_WARN("Received LRCLIB search result but syncedLyrics was malformed: %s", content.c_str());
            break;
        }

        cJSON* json_id = cJSON_GetObjectItem(json_result, "id");
        if((json_id == nullptr) || (json_id->type != cJSON_Number))
        {
            LOG_WARN("Received LRCLIB search result but id was malformed: %s", content.c_str());
            break;
        }
        const std::string source_path = std::string(g_api_url) + "get/" + std::to_string(json_id->valueint);

        // Add synced lyrics to the output first so that those will be preferred
        if(strlen(json_syncedlyrics->valuestring) > 0) {
            LOG_INFO("Successfully retrieved synced lyrics from %s", source_path.c_str());
            LyricDataRaw data = {};
            data.source_id = id();
            data.source_path = source_path;
            data.artist = json_artist->valuestring;
            data.album = json_album->valuestring;
            data.title = json_title->valuestring;
            data.type = LyricType::Synced;
            data.text_bytes = string_to_raw_bytes(json_syncedlyrics->valuestring);
            results.push_back(std::move(data));
        }

        if(strlen(json_plainlyrics->valuestring) > 0) {
            LOG_INFO("Successfully retrieved unsynced lyrics from %s", source_path.c_str());
            LyricDataRaw data = {};
            data.source_id = id();
            data.source_path = source_path;
            data.artist = json_artist->valuestring;
            data.album = json_album->valuestring;
            data.title = json_title->valuestring;
            data.type = LyricType::Unsynced;
            data.text_bytes = string_to_raw_bytes(json_plainlyrics->valuestring);
            results.push_back(std::move(data));
        }
    }

    cJSON_Delete(json);
    return results;
}

bool LrclibLyricsSource::lookup(LyricDataRaw& /*data*/, abort_callback& /*abort*/)
{
    LOG_ERROR("We should never need to do a lookup of the %s source", friendly_name().data());
    assert(false);
    return false;
}

struct UploadChallenge
{
    std::string prefix;
    std::string target;
};

static std::optional<UploadChallenge> get_challenge(abort_callback& abort)
{
    LOG_INFO("Requesting a challenge for LRCLIB upload...");
    std::string url = std::string(g_api_url) + "request-challenge";
    pfc::string8 content;
    try
    {
        http_request::ptr request = http_client::get()->create_request("POST");
        request->add_header("User-Agent", "foo_openlyrics v" OPENLYRICS_VERSION " (https://github.com/jacquesh/foo_openlyrics)");
        file_ptr response_file = request->run(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to request LRCLIB upload challenge from %s: %s", url.c_str(), e.what());
        return {};
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_WARN("Received LRCLIB challenge but JSON root was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_prefix = cJSON_GetObjectItem(json, "prefix");
    if((json_prefix == nullptr) || (json_prefix->type != cJSON_String))
    {
        LOG_WARN("Received LRCLIB challenge but prefix was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    cJSON* json_target = cJSON_GetObjectItem(json, "target");
    if((json_target == nullptr) || (json_target->type != cJSON_String))
    {
        LOG_WARN("Received LRCLIB challenge but target was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return {};
    }

    const UploadChallenge result = {
        json_prefix->valuestring,
        json_target->valuestring,
    };
    cJSON_Delete(json);
    return result;
}

// Returns true if `hash` is lexicographically less than or equal to `target.
// Otherwise false.`
static bool is_hash_less_or_equal(const uint8_t (&hash)[32], const uint8_t (&target)[32])
{
    for(size_t i=0; i<sizeof(hash); i++)
    {
        if(hash[i] > target[i]) return false;
        else if(hash[i] < target[i]) break;
    }
    return true;
}

static bool decode_target_hex_string(const std::string& target, uint8_t (&buffer)[32])
{
    if(target.length() != 64)
    {
        LOG_INFO("Attempt to decode a target hex string '%s' that is not the correct length", target.c_str());
        return false;
    }

    static uint8_t hexvals[] = {
        0,1,2,3,4,5,6,7,8,9, //Digits
        0,0,0,0,0,0,0,       // Special characters
        10,11,12,13,14,15,   // A-F
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // G-Z
        0,0,0,0,0,0,         // Special characters
        10,11,12,13,14,15,   // a-f
    };
    static_assert(sizeof(hexvals) == 'f' - '0' + 1); // `hexvals` is the correct length
    for(size_t i=0; i<32; i++)
    {
        const char hi_nibble = target[2*i];
        const char lo_nibble = target[2*i+1];
        if((hi_nibble < '0') || (hi_nibble > 'f') || (lo_nibble < '0') || (lo_nibble > 'f'))
        {
            return false;
        }
        const uint8_t hi_hex = hexvals[hi_nibble - '0'];
        const uint8_t lo_hex = hexvals[lo_nibble - '0'];
        if((hi_nibble != '0') && (hi_hex == 0) ||
            ((lo_nibble != '0') && (lo_hex == 0)))
        {
            return false;
        }
        buffer[i] = (hi_hex << 4) | lo_hex;
    }

    return true;
}

static uint64_t solve_challenge(const UploadChallenge& challenge, abort_callback& abort)
{
    // Compute the sha-256 of challenge `prefix`, with an integer concatenated at the end (as a utf-8 string).
    // Find one such integer for which the resulting hash value is less than `target` (IE 000000N) for N less than FF
    LARGE_INTEGER start_time = {};
    QueryPerformanceCounter(&start_time);

    Sha256Context sha;
    uint8_t target_buffer[32] = {};
    uint8_t hash_buffer[32] = {};
    char combined_input[128] = {};
    snprintf(combined_input, sizeof(combined_input), "%s", challenge.prefix.c_str());

    const bool target_decode_success = decode_target_hex_string(challenge.target, target_buffer);
    if(!target_decode_success)
    {
        LOG_WARN("Failed to decode challenge target hex string: %s", challenge.target.c_str());
        return 0;
    }

    const size_t prefix_len = challenge.prefix.length();
    char* const nonce_start_ptr = combined_input + prefix_len;
    const size_t nonce_capacity = sizeof(combined_input) - prefix_len;

    uint64_t nonce = 0;
    while(true) {
        const size_t nonce_len = (size_t)snprintf(nonce_start_ptr, nonce_capacity, "%llu", nonce);
        const size_t combined_len = prefix_len + nonce_len;

        sha.add_data((uint8_t*)&combined_input[0], combined_len);
        sha.finalise(hash_buffer);

        if(is_hash_less_or_equal(hash_buffer, target_buffer)) {
            break;
        }
        if((nonce % 1024 == 0) && (abort.is_aborting()))
        {
            LOG_INFO("Breaking out of proof-of-work calculation early due to an abort signal");
            break;
        }

        nonce++;
    }

    LARGE_INTEGER end_time = {};
    QueryPerformanceCounter(&end_time);
    LARGE_INTEGER freq = {};
    QueryPerformanceFrequency(&freq);
    const float elapsed_sec = float(end_time.QuadPart - start_time.QuadPart)/float(freq.QuadPart);
    LOG_INFO("Solved challenge SHA256(%s) < %s with nonce %llu in %.2fs",
            challenge.prefix.c_str(),
            challenge.target.c_str(),
            nonce,
            elapsed_sec);

    return nonce;
}

static void upload_lyrics(const LyricData& lyrics, const UploadChallenge& challenge, uint64_t nonce, abort_callback& abort)
{
    char token_buffer[256];
    snprintf(token_buffer, sizeof(token_buffer), "%s:%llu", challenge.prefix.c_str(), nonce);

    std::string plain_lyrics;
    std::string synced_lyrics;
    if(lyrics.IsTimestamped())
    {
        synced_lyrics = from_tstring(parsers::lrc::expand_text(lyrics));
        LyricData new_lyrics = lyrics;
        new_lyrics.RemoveTimestamps();
        plain_lyrics = from_tstring(parsers::lrc::expand_text(new_lyrics));
    }
    else
    {
        plain_lyrics = from_tstring(parsers::lrc::expand_text(lyrics));
    }

    cJSON* content_root = cJSON_CreateObject();
    cJSON_AddStringToObject(content_root, "trackName", lyrics.title.c_str());
    cJSON_AddStringToObject(content_root, "artistName", lyrics.artist.c_str());
    cJSON_AddStringToObject(content_root, "albumName", lyrics.album.c_str());
    cJSON_AddNumberToObject(content_root, "duration", double(lyrics.duration_sec.value_or(0)));
    cJSON_AddStringToObject(content_root, "plainLyrics", plain_lyrics.c_str());
    cJSON_AddStringToObject(content_root, "syncedLyrics", synced_lyrics.c_str());
    char* json_str = cJSON_Print(content_root);
    cJSON_Delete(content_root);

    LOG_INFO("Uploading lyrics to LRCLIB...");
    std::string url = std::string(g_api_url) + "publish";
    pfc::string8 content;
    try
    {
        http_request_post_v2::ptr post;
        http_client::get()->create_request("POST")->cast(post);
        assert(post.is_valid());

        post->add_header("User-Agent", "foo_openlyrics v" OPENLYRICS_VERSION " (https://github.com/jacquesh/foo_openlyrics)");
        post->add_header("X-Publish-Token", token_buffer);
        post->set_post_data(json_str, strlen(json_str), "application/json");
        file_ptr response_file = post->run_ex(url.c_str(), abort);
        response_file->read_string_raw(content, abort);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to upload to LRCLIB via %s: %s", url.c_str(), e.what());
        cJSON_free(json_str);
        return;
    }
    cJSON_free(json_str);

    // This is empty on success, and only populated on error
    if(content.get_length() == 0)
    {
        LOG_INFO("Successfully uploaded lyrics for %s - %s to LRCLIB", lyrics.artist.c_str(), lyrics.title.c_str());
        return;
    }

    cJSON* json = cJSON_ParseWithLength(content.c_str(), content.get_length());
    if((json == nullptr) || (json->type != cJSON_Object))
    {
        LOG_WARN("Received LRCLIB upload error response but JSON root was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return;
    }

    cJSON* json_code = cJSON_GetObjectItem(json, "statusCode");
    if((json_code == nullptr) || (json_code->type != cJSON_Number))
    {
        LOG_WARN("Received LRCLIB upload error response but code was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return;
    }

    cJSON* json_name = cJSON_GetObjectItem(json, "name");
    if((json_name == nullptr) || (json_name->type != cJSON_String))
    {
        LOG_WARN("Received LRCLIB upload error response but name was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return;
    }

    cJSON* json_msg = cJSON_GetObjectItem(json, "message");
    if((json_msg == nullptr) || (json_msg->type != cJSON_String))
    {
        LOG_WARN("Received LRCLIB upload error response but message was malformed: %s", content.c_str());
        cJSON_Delete(json);
        return;
    }

    LOG_WARN("Failed to upload lyrics to LRCLIB with error code %d/%s: %s",
            json_code->valueint,
            json_name->valuestring,
            json_msg->valuestring);
    cJSON_Delete(json);
}

void LrclibLyricsSource::upload(const LyricData& lyrics, abort_callback& abort)
{
    const std::optional<UploadChallenge> maybe_challenge = get_challenge(abort);
    if(!maybe_challenge.has_value())
    {
        return;
    }
    const UploadChallenge& challenge = maybe_challenge.value();
    uint64_t nonce = solve_challenge(challenge, abort);
    upload_lyrics(lyrics, challenge, nonce, abort);
}


// ============
// Tests
// ============
#ifdef MVTF_TESTS_ENABLED
MVTF_TEST(lrclib_equal_hashes_are_less_or_equal)
{
    uint8_t value[32] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    ASSERT(is_hash_less_or_equal(value, value));
}

MVTF_TEST(lrclib_less_hashes_are_less_or_equal)
{
    uint8_t hash[32] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t trgt[32] = {0x01, 0x02, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    ASSERT(is_hash_less_or_equal(hash, trgt));
}

MVTF_TEST(lrclib_greater_hashes_are_NOT_less_or_equal)
{
    uint8_t hash[32] = {0x01, 0x02, 0x03, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t trgt[32] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    ASSERT(!is_hash_less_or_equal(hash, trgt));
}

MVTF_TEST(lrclib_hexdecode_fails_for_strings_with_invalid_characters_inside_the_valid_range)
{
    const std::string target = "000000FF000000000000000000:0000000000000000000000000000000000000";
    uint8_t buffer[32] = {};
    const bool success = decode_target_hex_string(target, buffer);
    ASSERT(!success);
}

MVTF_TEST(lrclib_hexdecode_fails_for_strings_with_invalid_characters_outside_the_valid_range)
{
    const std::string target = "000000FF000000000000000000~0000000000000000000000000000000000000";
    uint8_t buffer[32] = {};
    const bool success = decode_target_hex_string(target, buffer);
    ASSERT(!success);
}

MVTF_TEST(lrclib_hexdecode_succeeds_for_strings_of_the_correct_length)
{
    const std::string target = "000000FF00000000000000000000000000000000000000000000000000000000";
    uint8_t buffer[32] = {};
    const bool success = decode_target_hex_string(target, buffer);
    ASSERT(success);
}

MVTF_TEST(lrclib_hexdecode_fails_for_strings_of_the_incorrect_length)
{
    const std::string target = "000000FF000000000000000000000000000000000000000000000000000000001";
    uint8_t buffer[32] = {};
    const bool success = decode_target_hex_string(target, buffer);
    ASSERT(!success);
}

MVTF_TEST(lrclib_hexdecode_successfully_decodes_strings_with_upper_case_only)
{
    const std::string target = "000000FF00000000000000000000000000000000000000000FEDCBA987654321";
    uint8_t buffer[32] = {};
    const bool success = decode_target_hex_string(target, buffer);
    ASSERT(success);
    ASSERT(buffer[0] == 0x00);
    ASSERT(buffer[1] == 0x00);
    ASSERT(buffer[2] == 0x00);
    ASSERT(buffer[3] == 0xFF);
    ASSERT(buffer[4] == 0x00);

    ASSERT(buffer[24] == 0x0F);
    ASSERT(buffer[25] == 0xED);
    ASSERT(buffer[26] == 0xCB);
    ASSERT(buffer[27] == 0xA9);
    ASSERT(buffer[28] == 0x87);
    ASSERT(buffer[29] == 0x65);
    ASSERT(buffer[30] == 0x43);
    ASSERT(buffer[31] == 0x21);
}

MVTF_TEST(lrclib_hexdecode_successfully_decodes_strings_with_lower_case_only)
{
    const std::string target = "000000ff00000000000000000000000000000000000000000fedcba987654321";
    uint8_t buffer[32] = {};
    const bool success = decode_target_hex_string(target, buffer);
    ASSERT(success);
    ASSERT(buffer[0] == 0x00);
    ASSERT(buffer[1] == 0x00);
    ASSERT(buffer[2] == 0x00);
    ASSERT(buffer[3] == 0xFF);
    ASSERT(buffer[4] == 0x00);

    ASSERT(buffer[24] == 0x0F);
    ASSERT(buffer[25] == 0xED);
    ASSERT(buffer[26] == 0xCB);
    ASSERT(buffer[27] == 0xA9);
    ASSERT(buffer[28] == 0x87);
    ASSERT(buffer[29] == 0x65);
    ASSERT(buffer[30] == 0x43);
    ASSERT(buffer[31] == 0x21);
}
#endif
