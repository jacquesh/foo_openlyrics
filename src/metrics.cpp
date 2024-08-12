#include "stdafx.h"

#include <chrono>

#pragma warning(push, 0)
#include <bcrypt.h>
#include <foobar2000/helpers/DarkMode.h>
#pragma warning(pop)

#include "cJSON.h"

// Don't recompile every time in dev/debug builds
#ifdef NDEBUG
#include "openlyrics_version.h" // Defines OPENLYRICS_VERSION
#else
#define OPENLYRICS_VERSION "0.0-dev"
#endif

#include "hash_utils.h"
#include "logging.h"
#include "metrics.h"
#include "sources/lyric_source.h"
#include "ui_hooks.h"

static const GUID GUID_METRICS_FIRST_VERSION_INSTALL_DATE_DAYS_SINCE_UNIX_EPOCH = { 0x6ce22b14, 0x3237, 0x4afb, { 0x9d, 0x66, 0xe8, 0xe9, 0x9b, 0xa4, 0xee, 0xe6 } };
static const GUID GUID_METRICS_CURRENT_VERSION_INSTALL_DATE_DAYS_SINCE_UNIX_EPOCH = { 0xe827d9b6, 0x681a, 0x4d99, { 0xa5, 0xaa, 0x41, 0xcf, 0xef, 0xf2, 0xf, 0x18 } };
static const GUID GUID_METRICS_CURRENT_VERSION = { 0x922ab2fa, 0x5b, 0x4971, { 0x9b, 0x78, 0xf5, 0xc5, 0x57, 0x30, 0xc1, 0x32 } };
static const GUID GUID_METRICS_GENERATION = { 0xf2d97e96, 0x38ab, 0x4e5e, { 0x83, 0x7c, 0xe3, 0x3b, 0x5a, 0x82, 0x43, 0xca } };

static cfg_int_t<uint64_t> cfg_metrics_first_version_install_date_days_since_unix_epoch(GUID_METRICS_FIRST_VERSION_INSTALL_DATE_DAYS_SINCE_UNIX_EPOCH, 0);
static cfg_int_t<uint64_t> cfg_metrics_current_version_install_date_days_since_unix_epoch(GUID_METRICS_CURRENT_VERSION_INSTALL_DATE_DAYS_SINCE_UNIX_EPOCH, 0);
static cfg_string cfg_metrics_current_version(GUID_METRICS_CURRENT_VERSION, "");
static cfg_int_t<uint64_t> cfg_metrics_generation(GUID_METRICS_GENERATION, 0);

// The metrics "generation", which tells us whether or not we need to send a new batch of metrics.
// Manually bump this when a new round of metrics collection is desired.
constexpr uint64_t current_metrics_generation = 2;
constexpr std::chrono::year_month_day last_metrics_collection_day = {std::chrono::year(2023), std::chrono::month(07), std::chrono::day(10)};

class FeatureTracker;
std::vector<FeatureTracker*> g_all_feature_trackers;
class FeatureTracker : public cfg_var_reader, public cfg_var_writer
{
private:
    const char* m_name;
    uint64_t m_last_used; // Days since the unix epoch
    uint64_t m_first_used; // Days since the unix epoch
    uint64_t m_usage_count; // Number of tracked usages since first used

public:
    explicit FeatureTracker(const char* name, const GUID& p_guid)
        : cfg_var_reader(p_guid)
        , cfg_var_writer(p_guid)
        , m_name(name)
        , m_last_used(0)
        , m_first_used(0)
        , m_usage_count(0)
    {
        g_all_feature_trackers.push_back(this);
    }

    const char* name() const
    {
        return m_name;
    }

    uint64_t last_used() const
    {
        return m_last_used;
    }

    uint64_t first_used() const
    {
        return m_first_used;
    }

    uint64_t usage_count() const
    {
        return m_usage_count;
    }

    void log_usage()
    {
        const auto since_unix_epoch = std::chrono::system_clock::now().time_since_epoch();
        const uint64_t days_since_unix_epoch  = std::chrono::floor<std::chrono::days>(since_unix_epoch).count();

        if(m_last_used < days_since_unix_epoch)
        {
            m_last_used = days_since_unix_epoch;
        }
        if(m_first_used == 0)
        {
            m_first_used = days_since_unix_epoch;
        }
        m_usage_count++;
    }

protected:
    void get_data_raw(stream_writer* p_stream, abort_callback& p_abort) override
    {
        p_stream->write_lendian_t(m_last_used, p_abort);
        p_stream->write_lendian_t(m_first_used, p_abort);
        p_stream->write_lendian_t(m_usage_count, p_abort);
    }

    void set_data_raw(stream_reader* p_stream, size_t bytes_available, abort_callback& p_abort) override
    {
        uint64_t temp_last_used;
        p_stream->read_lendian_t(temp_last_used, p_abort); //alter member data only on success, this will throw an exception when something isn't right
        m_last_used = temp_last_used;

        if(bytes_available >= 24)
        {
            uint64_t temp_first_used;
            uint64_t temp_usage_count;
            p_stream->read_lendian_t(temp_first_used, p_abort);
            p_stream->read_lendian_t(temp_usage_count, p_abort);
            m_first_used = temp_first_used;
            m_usage_count = temp_usage_count;
        }
    }
};

#define DECLARE_FEATURETRACKER(FTNAME, ...) static FeatureTracker featuretrack_##FTNAME(#FTNAME, __VA_ARGS__)
DECLARE_FEATURETRACKER(bulksearch, { 0x10f5f5c1, 0x472d, 0x4bc6, { 0xa4, 0xb4, 0x70, 0x54, 0x85, 0x0f, 0xc5, 0xe2 } });
DECLARE_FEATURETRACKER(manualsearch, { 0x452cd5bf, 0x4c0c, 0x4814, { 0x8f, 0x4e, 0x1b, 0xda, 0x2f, 0x53, 0x4c, 0x6a } });
DECLARE_FEATURETRACKER(lyriceditor, { 0x3304c89e, 0xec50, 0x4af1, { 0xa1, 0xa0, 0x21, 0xe3, 0xfd, 0xea, 0x12, 0x32 } });
DECLARE_FEATURETRACKER(autoedit, { 0xb1df2480, 0xde3a, 0x49ec, { 0xbf, 0xd, 0x7d, 0x39, 0x31, 0xc8, 0x41, 0x24 } });
DECLARE_FEATURETRACKER(markinstrumental, { 0x12253c20, 0x9324, 0x4095, { 0x89, 0xb8, 0x3e, 0xc8, 0x3f, 0x13, 0xf8, 0x23 } });
DECLARE_FEATURETRACKER(showlyrics, { 0xa6665198, 0xd2d1, 0x44ac, { 0xa8, 0xde, 0x1c, 0x6c, 0xb5, 0xbe, 0x0d, 0x81 } });
DECLARE_FEATURETRACKER(externalwindow, { 0xf426ac64, 0x4aa7, 0x403a, { 0x97, 0x64, 0xff, 0x62, 0x51, 0xcb, 0xe6, 0x73 } });
DECLARE_FEATURETRACKER(manualscroll, { 0x3b751894, 0x9163, 0x4902, { 0x8d, 0x65, 0x3, 0x10, 0x35, 0x21, 0xb5, 0x4d } });
DECLARE_FEATURETRACKER(lyricupload, { 0xf4975820, 0x23c, 0x44e6, { 0x8d, 0x30, 0xf1, 0xb8, 0x63, 0x2a, 0x8c, 0x46 } });
DECLARE_FEATURETRACKER(remotetrack, { 0xa0cbfda0, 0x99c3, 0x4be7, { 0x86, 0xf0, 0x8e, 0xb2, 0xd1, 0x48, 0x89, 0x9 } });

void metrics::log_used_bulk_search()
{
    featuretrack_bulksearch.log_usage();
}

void metrics::log_used_manual_search()
{
    featuretrack_manualsearch.log_usage();
}

void metrics::log_used_lyric_editor()
{
    featuretrack_lyriceditor.log_usage();
}

void metrics::log_used_auto_edit()
{
    featuretrack_autoedit.log_usage();
}

void metrics::log_used_mark_instrumental()
{
    featuretrack_markinstrumental.log_usage();
}

void metrics::log_used_show_lyrics()
{
    featuretrack_showlyrics.log_usage();
}

void metrics::log_used_external_window()
{
    featuretrack_externalwindow.log_usage();
}

void metrics::log_used_manual_scroll()
{
    featuretrack_manualscroll.log_usage();
}

void metrics::log_used_lyric_upload()
{
    featuretrack_lyricupload.log_usage();
}

void metrics::log_searched_for_lyrics_for_a_remote_track()
{
    featuretrack_remotetrack.log_usage();
}

static const char* get_windows_version_string()
{
    // NOTE: There is no `IsWindows11OrGreater()` function
    if(IsWindows10OrGreater()) return "10+";
    if(IsWindows8Point1OrGreater()) return "8.1"; 
    if(IsWindows8OrGreater()) return "8.0";
    if(IsWindows7SP1OrGreater()) return "7.1";
    if(IsWindows7OrGreater()) return "7.0";
    return "old";
}

static std::string get_openlyrics_dll_hash(abort_callback& abort)
{
    Sha256Context sha;
    try
    {
        file_ptr file;
        const char* dll_path = core_api::get_my_full_path();
        filesystem::g_open_read(file, dll_path, abort);
        if(file == nullptr)
        {
            return "<fileopen-error>";
        }

        uint8_t tmp_buffer[4096];
        while(true)
        {
            size_t bytes_read = file->read(tmp_buffer, sizeof(tmp_buffer), abort);
            if(bytes_read == 0)
            {
                break;
            }
            sha.add_data(tmp_buffer, bytes_read);
        }
    }
    catch(const std::exception&)
    {
        return "<fileio-error>";
    }

    uint8_t hash_out[32] = {};
    sha.finalise(hash_out);

    char out[1024] = {};
    for(size_t i=0; i<sizeof(hash_out); i++)
    {
        snprintf(&out[2*i], sizeof(out)-2*i, "%02x", hash_out[i]);
    }
    return out;
}

static std::string get_source_name(GUID guid)
{
    LyricSourceBase* source = LyricSourceBase::get(guid);
    if(source == nullptr)
    {
        return "<unknown>";
    }

    return from_tstring(source->friendly_name());
}

std::string collect_metrics(abort_callback& abort, bool is_dark_mode)
{
    LOG_INFO("Metrics collection start");
    cJSON* json = cJSON_CreateObject();

    {
        cJSON* json_fb2k = cJSON_AddObjectToObject(json, "fb2k");
        cJSON_AddStringToObject(json_fb2k, "version", core_version_info::g_get_version_string());
        cJSON_AddBoolToObject(json_fb2k, "is_portable", core_api::is_portable_mode_enabled());
        cJSON_AddBoolToObject(json_fb2k, "is_dark_mode", is_dark_mode);
    }

    {
        cJSON* json_os = cJSON_AddObjectToObject(json, "os");

        // NOTE: We use GetDeviceCaps instead of GetDpiForMonitor (or similar functions) because
        //       fb2k supports Windows 7 and so we should too.
        HDC screen_dc = GetDC(nullptr);
        const int dpiX = GetDeviceCaps(screen_dc, LOGPIXELSX);
        const int dpiY = GetDeviceCaps(screen_dc, LOGPIXELSY);
        ReleaseDC(nullptr, screen_dc);

        cJSON_AddStringToObject(json_os, "version", get_windows_version_string());
        cJSON_AddNumberToObject(json_os, "dpi_x", dpiX);
        cJSON_AddNumberToObject(json_os, "dpi_y", dpiY);
    }

    {
        cJSON* json_ol = cJSON_AddObjectToObject(json, "openlyrics");

        const std::string hash_str = get_openlyrics_dll_hash(abort);
        const std::chrono::year_month_day install_ymd{std::chrono::sys_days{std::chrono::days(cfg_metrics_first_version_install_date_days_since_unix_epoch.get_value())}};
        char install_ymd_str[64] = {};
        snprintf(install_ymd_str, sizeof(install_ymd_str), "%02d-%02u-%02u", int(install_ymd.year()), unsigned int(install_ymd.month()), unsigned int(install_ymd.day()));

        cJSON_AddStringToObject(json_ol, "version", OPENLYRICS_VERSION);
        cJSON_AddStringToObject(json_ol, "library_hash", hash_str.c_str());
        cJSON_AddStringToObject(json_ol, "first_installed",install_ymd_str);
        cJSON_AddNumberToObject(json_ol, "num_panels", double(num_lyric_panels()));
    }

    cJSON* json_features = cJSON_AddObjectToObject(json, "features");
    for(const FeatureTracker* feature : g_all_feature_trackers)
    {
        cJSON* f = cJSON_AddObjectToObject(json_features, feature->name());
        cJSON_AddNumberToObject(f, "first_used", double(feature->first_used()));
        cJSON_AddNumberToObject(f, "last_used", double(feature->last_used()));
        cJSON_AddNumberToObject(f, "usage_count", double(feature->usage_count()));
    }

    cJSON* json_sources = cJSON_AddArrayToObject(json, "sources");
    for(GUID guid : preferences::searching::raw::active_sources_configured())
    {
        const std::string name = get_source_name(guid);
        cJSON_AddItemToArray(json_sources, cJSON_CreateString(name.c_str()));
    }

    {
        cJSON* json_cfg = cJSON_AddObjectToObject(json, "cfg");

        const auto get_auto_edits = []() -> std::string
        {
            std::string result;
            for(AutoEditType edit : preferences::editing::automated_auto_edits())
            {
                result += std::to_string(int(edit));
                result += ',';
            }
            return result;
        };

        // Search settings
        const std::string auto_edit_str = get_auto_edits();
        const std::string save_src_name = get_source_name(preferences::saving::save_source());
        cJSON_AddBoolToObject(json_cfg, "search_exclude_brackets", preferences::searching::exclude_trailing_brackets());
        cJSON_AddBoolToObject(json_cfg, "is_skip_filter_default", preferences::searching::raw::is_skip_filter_default());
        cJSON_AddStringToObject(json_cfg, "search_auto_edits", auto_edit_str.c_str());
        cJSON_AddNumberToObject(json_cfg, "preferred_lyric_type", int(preferences::searching::preferred_lyric_type()));

        // Save settings
        cJSON_AddNumberToObject(json_cfg, "autosave_strategy", int(preferences::saving::autosave_strategy()));
        cJSON_AddStringToObject(json_cfg, "save_source", save_src_name.c_str());
        cJSON_AddNumberToObject(json_cfg, "save_directory_type", int(preferences::saving::raw::directory_class()));
        cJSON_AddBoolToObject(json_cfg, "merge_lrc_lines_on_save", preferences::saving::merge_equivalent_lrc_lines());

        // Display settings
        cJSON_AddBoolToObject(json_cfg, "display_font_is_custom", preferences::display::raw::font_is_custom());
        cJSON_AddNumberToObject(json_cfg, "line_scroll_type", int(preferences::display::scroll_type()));
        cJSON_AddNumberToObject(json_cfg, "scroll_seconds", preferences::display::scroll_time_seconds());
        cJSON_AddNumberToObject(json_cfg, "text_alignment", int(preferences::display::text_alignment()));
        cJSON_AddBoolToObject(json_cfg, "debug_logs_enabled", preferences::display::debug_logs_enabled());

        // Background settings
        cJSON_AddNumberToObject(json_cfg, "background_type", int(preferences::background::fill_type()));
        cJSON_AddNumberToObject(json_cfg, "background_image_type", int(preferences::background::image_type()));

        // Upload settings
        cJSON_AddNumberToObject(json_cfg, "lrclib_upload_strategy", int(preferences::upload::lrclib_upload_strategy()));
    }

    char* json_str = cJSON_Print(json);
    std::string result(json_str);
    free(json_str);

    LOG_INFO("Metrics collection complete");
    return result;
}

void submit_metrics(std::string metrics)
{
    // The async_task_manager exposes an abort_callback that gets triggered when fb2k is closed.
    // This function is currently run via fb2k::splitTask, which will prevent the fb2k process
    // from terminating before all tasks are complete. By using the on-shutdown aborter, we
    // ensure that the metrics upload will not prevent fb2k from closing. This would be annoying
    // once the metrics server is offline since then if anybody tries to open fb2k, submits metrics
    // and then tries to restart it by closing and quickly re-opening, it won't work since the
    // previous (shutting down) instance will still be running.
    abort_callback& abort = async_task_manager::get()->get_aborter();

    http_request_post_v2::ptr request;
    http_request::ptr request_generic = http_client::get()->create_request("POST");
    bool cast_success = request_generic->cast(request);
    assert(cast_success);

    const char* metrics_ingest_url = "https://degwclvqbs7p6e24szelk7uxui0jhsxo.lambda-url.eu-west-2.on.aws/";
    LOG_INFO("Submitting metrics to %s...", metrics_ingest_url);

    try
    {
        request->set_post_data(metrics.c_str(), metrics.length(), "application/json");
        request->run(metrics_ingest_url, abort);
        LOG_INFO("Successfully submitted metrics to %s", metrics_ingest_url);
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to submit metrics to %s: %s", metrics_ingest_url, e.what());
    }
}

class AsyncMetricsCollectionAndSubmission : public threaded_process_callback
{
    std::string m_metrics;

    bool is_dark_mode;

public:
    void on_init(ctx_t /*p_wnd*/) override
    {
        LOG_INFO("Initiating pre-collection metrics flow...");
        is_dark_mode = fb2k::isDarkMode(); // In fb2k v2 beta 31 and earlier, isDarkMode() should only be called from the main/UI thread.
    }

    void run(threaded_process_status& /*status*/, abort_callback& abort) override
    {
        try
        {
            m_metrics = collect_metrics(abort, is_dark_mode);
        }
        catch(const std::exception& ex)
        {
            LOG_WARN("Exception encountered during metrics collection: %s", ex.what());
        }
    }

    void on_done(ctx_t /*ctx*/, bool was_aborted) override
    {
        LOG_INFO("Initiating post-collection metrics flow...");
        if(was_aborted)
        {
            LOG_INFO("Metrics collection was aborted, skipping upload prompt");
            return;
        }
        if(m_metrics.empty())
        {
            LOG_WARN("Metrics collection was not aborted, but not metrics data is available. Skipping upload.");
            return;
        }

        popup_message_v3::query_t query = {};
        query.title = "OpenLyrics metrics";
        query.msg = "Would you like to send some basic usage metrics to the foo_openlyrics developer?\n\nTo effectively direct the limited time that I have to work on foo_openlyrics, I'd like to collect some basic, once-off data about usage of foo_openlyrics among the community.\n\nThis usage data will help to inform which features are added or enhanced, as well as potentially which features get removed (e.g if nobody uses them).\n\nNo uniquely-identifying information is collected, all information will be used exclusively to inform foo_openlyrics' development, will never be sold or used for marketting (by anybody), and will be deleted after 6 months.\n\nYou can click 'Retry' to view the exact data that would be submitted.";
        query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo | popup_message_v3::buttonRetry;
        query.defButton = popup_message_v3::buttonNo;
        query.icon = popup_message_v3::iconQuestion;

        uint32_t popup_result = popup_message_v3::buttonRetry;
        do
        {
            popup_result = popup_message_v3::get()->show_query_modal(query);
            if(popup_result == popup_message_v3::buttonRetry)
            {
                popup_message::g_show(m_metrics.c_str(), "OpenLyrics metrics");
            }
            else if(popup_result == popup_message_v3::buttonYes)
            {
                fb2k::splitTask([metrics = this->m_metrics](){
                    submit_metrics(metrics);
                });

                popup_message_v3::query_t thank_query = {};
                thank_query.title = "OpenLyrics metrics";
                thank_query.msg = "Thank you for helping to inform the continue development effort!";
                thank_query.buttons = popup_message_v3::buttonOK;
                thank_query.defButton = popup_message_v3::buttonOK;
                popup_message_v3::get()->show_query(thank_query);
            }
        } while(popup_result == popup_message_v3::buttonRetry);
    }
};

static void send_metrics_on_init()
{
    const auto since_unix_epoch = std::chrono::system_clock::now().time_since_epoch();
    const uint64_t days_since_unix_epoch = static_cast<uint64_t>(std::chrono::floor<std::chrono::days>(since_unix_epoch).count());
    if(cfg_metrics_first_version_install_date_days_since_unix_epoch.get_value() == 0)
    {
        cfg_metrics_first_version_install_date_days_since_unix_epoch = days_since_unix_epoch;
    }

    const bool current_version_set = (cfg_metrics_current_version_install_date_days_since_unix_epoch.get_value() != 0);
    const bool current_version_matches_installed_version = (cfg_metrics_current_version == OPENLYRICS_VERSION);
    if(!current_version_set || !current_version_matches_installed_version)
    {
        cfg_metrics_current_version = OPENLYRICS_VERSION;
        cfg_metrics_current_version_install_date_days_since_unix_epoch = days_since_unix_epoch;
    }

    // We use our "installed-date" config to delay initial metrics by at least a week from first
    // install. This lets people have some time to use the plugin before we prompt for metrics.
    // We don't want to bother somebody on their very first launch (they might just be trying it
    // out and not plan to use it long-term, and it probably wouldn't garner any favour from new users).
    // Immediately after install is also the least useful time to collect metrics since new users
    // will not have had the opportunity to change any configuration yet.
    const uint64_t min_days_of_delay_after_install = 7;
    const uint64_t min_days_since_first_install = min_days_of_delay_after_install + cfg_metrics_first_version_install_date_days_since_unix_epoch.get_value();
    const uint64_t min_days_since_last_install = min_days_of_delay_after_install + cfg_metrics_current_version_install_date_days_since_unix_epoch.get_value();
    if((days_since_unix_epoch < min_days_since_first_install) || (days_since_unix_epoch < min_days_since_last_install))
    {
        return;
    }

    // Set an end-date after which we don't prompt for metrics anymore (at least not for this generation).
    // This prevents us from prompting new users for metrics collection forever, long
    // after we've turned off the collection server.
    const std::chrono::year_month_day current_day = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
    if(current_day > last_metrics_collection_day)
    {
        return;
    }

    // Don't send metrics for this "generation" if we've already done so.
    if(cfg_metrics_generation.get_value() >= current_metrics_generation)
    {
        return;
    }
    cfg_metrics_generation = current_metrics_generation;

    bool kickoff_success = threaded_process::g_run_modeless(new service_impl_t<AsyncMetricsCollectionAndSubmission>(),
                                                            threaded_process::flag_show_abort | threaded_process::flag_show_delayed,
                                                            core_api::get_main_window(),
                                                            "Preparing metrics for OpenLyrics...");
    if(kickoff_success)
    {
        LOG_INFO("Successfully initialised metrics collection");
    }
    else
    {
        LOG_WARN("Failed to initiate metrics collection");
    }
}
FB2K_RUN_ON_INIT(send_metrics_on_init)