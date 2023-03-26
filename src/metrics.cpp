#include "stdafx.h"

#include <time.h>

#pragma warning(push, 0)
#include <bcrypt.h>
#include <foobar2000/helpers/DarkMode.h>
#pragma warning(pop)

#include "cJSON.h"

#include "logging.h"
#include "sources/lyric_source.h"
#include "ui_hooks.h"

static const GUID GUID_METRICS_INSTALL_DATE = { 0x6ce22b14, 0x3237, 0x4afb, { 0x9d, 0x66, 0xe8, 0xe9, 0x9b, 0xa4, 0xee, 0xe6 } };
static const GUID GUID_METRICS_GENERATION = { 0xf2d97e96, 0x38ab, 0x4e5e, { 0x83, 0x7c, 0xe3, 0x3b, 0x5a, 0x82, 0x43, 0xca } };

static cfg_int_t<uint64_t> cfg_metrics_install_date(GUID_METRICS_INSTALL_DATE, 0);
static cfg_int_t<uint64_t> cfg_metrics_generation(GUID_METRICS_GENERATION, 0);

constexpr uint64_t current_metrics_generation = 2;


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

std::string collect_metrics(abort_callback& abort)
{
    cJSON* json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "fb2k.version", core_version_info::g_get_version_string());
    cJSON_AddBoolToObject(json, "fb2k.is_portable", core_api::is_portable_mode_enabled());
    cJSON_AddBoolToObject(json, "fb2k.is_dark_mode", fb2k::isDarkMode());

    // NOTE: We use GetDeviceCaps instead of GetDpiForMonitor (or similar functions) because
    //       fb2k supports Windows 7 and so we should too.
    HDC screen_dc = GetDC(nullptr);
    const int dpiX = GetDeviceCaps(screen_dc, LOGPIXELSX);
    const int dpiY = GetDeviceCaps(screen_dc, LOGPIXELSY);
    ReleaseDC(nullptr, screen_dc);

    cJSON_AddStringToObject(json, "os.version", get_windows_version_string());
    cJSON_AddNumberToObject(json, "os.dpi_x", dpiX);
    cJSON_AddNumberToObject(json, "os.dpi_y", dpiY);

    const auto get_openlyrics_dll_hash = [](abort_callback& abort) -> std::string
    {
        const auto is_success = [](NTSTATUS status)
        {
            return status >= 0; // According to the example code at https://learn.microsoft.com/en-us/windows/win32/seccng/creating-a-hash-with-cng
        };

        // BCryptGetProperty wants to output the number of bytes written to the output buffer.
        // This parameter is meant to be optional but we get failures without it.
        ULONG get_property_bytes_written = 0;

        BCRYPT_ALG_HANDLE hash_alg = {};
        NTSTATUS status = BCryptOpenAlgorithmProvider(&hash_alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        if(!is_success(status))
        {
            return "<hopen-error>";
        }

        DWORD hash_obj_size = 0;
        status = BCryptGetProperty(hash_alg, BCRYPT_OBJECT_LENGTH, (UCHAR*)&hash_obj_size, sizeof(hash_obj_size), &get_property_bytes_written, 0);
        if(!is_success(status))
        {
            return "<hgetsize-error>";
        }

        std::vector<UCHAR> hash_memory;
        hash_memory.resize(hash_obj_size);

        BCRYPT_HASH_HANDLE hash_handle = {};
        status = BCryptCreateHash(hash_alg, &hash_handle, hash_memory.data(), hash_memory.size(), nullptr, 0, 0);
        if(!is_success(status))
        {
            return "<hcreate-error>";
        }

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

                status = BCryptHashData(hash_handle, tmp_buffer, bytes_read, 0);
                if(!is_success(status))
                {
                    return "<hdata-error>";
                }
            }
        }
        catch(const std::exception&)
        {
            return "<fileio-error>";
        }

        DWORD hash_out_size = 0;
        status = BCryptGetProperty(hash_alg, BCRYPT_HASH_LENGTH, (UCHAR*)&hash_out_size, sizeof(hash_out_size), &get_property_bytes_written, 0);
        if(!is_success(status))
        {
            return "<hgetlength-error>";
        }

        std::vector<UCHAR> hash_out;
        hash_out.resize(hash_out_size);
        status = BCryptFinishHash(hash_handle, hash_out.data(), hash_out.size(), 0);
        if(!is_success(status))
        {
            return "<hfinish-error>";
        }

        BCryptDestroyHash(hash_handle);
        BCryptCloseAlgorithmProvider(hash_alg, 0);

        char out[1024] = {};
        for(size_t i=0; i<hash_out_size; i++)
        {
            snprintf(&out[2*i], sizeof(out)-2*i, "%02x", hash_out[i]);
        }
        return out;
    };

    const std::string hash_str = get_openlyrics_dll_hash(abort);
    cJSON_AddStringToObject(json, "ol.version", hash_str.c_str());
    cJSON_AddNumberToObject(json, "ol.num_panels", double(num_lyric_panels()));
    cJSON_AddStringToObject(json, "ol.installed_since", "TODO: Date of installation of the plugin?");

    const auto get_source_name = [](GUID guid) -> std::string
    {
        LyricSourceBase* source = LyricSourceBase::get(guid);
        if(source == nullptr)
        {
            return "<unknown>";
        }

        return from_tstring(source->friendly_name());
    };
    const auto get_active_sources = [&get_source_name]() -> std::string
    {
        std::string result;
        for(GUID guid : preferences::searching::raw::active_sources_configured())
        {
            result += get_source_name(guid);
            result += ',';
        }
        return result;
    };
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
    const std::string sources_str = get_active_sources();
    const std::string auto_edit_str = get_auto_edits();
    const std::string save_src_name = get_source_name(preferences::saving::save_source());

    cJSON_AddStringToObject(json, "ol.cfg.search.sources", sources_str.c_str());
    cJSON_AddBoolToObject(json, "ol.cfg.search.exclude_brackets", preferences::searching::exclude_trailing_brackets());
    cJSON_AddStringToObject(json, "ol.cfg.search.auto_edit", auto_edit_str.c_str());
    cJSON_AddNumberToObject(json, "ol.cfg.save.autosave", int(preferences::saving::autosave_strategy()));
    cJSON_AddStringToObject(json, "ol.cfg.save.src", save_src_name.c_str());
    cJSON_AddNumberToObject(json, "ol.cfg.save.dir_type", int(preferences::saving::raw::directory_class()));
    cJSON_AddBoolToObject(json, "ol.cfg.save.merge_lrc_lines", preferences::saving::merge_equivalent_lrc_lines());
    cJSON_AddBoolToObject(json, "ol.cfg.display.font", preferences::display::raw::font_is_custom());
    cJSON_AddBoolToObject(json, "ol.cfg.display.fg_color", preferences::display::foreground_colour().has_value());
    cJSON_AddBoolToObject(json, "ol.cfg.display.bg_color", preferences::display::background_colour().has_value());
    cJSON_AddBoolToObject(json, "ol.cfg.display.hl_color", preferences::display::highlight_colour().has_value());
    cJSON_AddNumberToObject(json, "ol.cfg.display.scroll_dir", int(preferences::display::scroll_direction()));
    cJSON_AddNumberToObject(json, "ol.cfg.display.scroll_type", int(preferences::display::scroll_type()));
    cJSON_AddNumberToObject(json, "ol.cfg.display.scroll_time", preferences::display::scroll_time_seconds());
    cJSON_AddNumberToObject(json, "ol.cfg.display.linegap", preferences::display::linegap());
    cJSON_AddBoolToObject(json, "ol.cfg.display.debug_logs", preferences::display::debug_logs_enabled());

    char* json_str = cJSON_Print(json);
    std::string result(json_str);
    free(json_str);
    return result;
}

void submit_metrics(std::string metrics, abort_callback& abort)
{
    http_request_post_v2::ptr request;
    http_request::ptr request_generic = http_client::get()->create_request("POST");
    bool cast_success = request->cast(request_generic);
    assert(cast_success);

    std::string url = "https://some-aws-url-for-a-lambda-that-ingests-metrics.amazonaws.com"; // TODO
    LOG_INFO("Submitting metrics to %s...", url.c_str());

    try
    {
        request->set_post_data(metrics.c_str(), metrics.length(), "application/json");
        request->run(url.c_str(), abort);
        LOG_INFO("Successfully submitted metrics to %s", url.c_str());
    }
    catch(const std::exception& e)
    {
        LOG_WARN("Failed to submit metrics to %s: %s", url.c_str(), e.what());
    }
}

class foo_send_metrics_on_init : public initquit
{
    void on_init() override;
};

class AsyncMetricsCollectionAndSubmission : public threaded_process_callback
{
    std::string m_metrics;

public:
    void run(threaded_process_status& /*status*/, abort_callback& abort) override
    {
        try
        {
            m_metrics = collect_metrics(abort);
            LOG_INFO("Successfully collected metrics");
        }
        catch(const std::exception& ex)
        {
            LOG_WARN("Exception encountered during metrics collection: %s", ex.what());
        }
    }

    void on_done(ctx_t /*ctx*/, bool was_aborted)
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
        query.msg = "Would you like to send some basic usage metrics to the foo_openlyrics developer?\n\nTo effectively direct the limited time that I have to work on foo_openlyrics, I'd like to collect some basic data about usage of foo_openlyrics among the community.\n\nThis usage data will help to inform which features are added or enhanced, as well as potentially which features get removed (e.g if nobody uses them).\n\nNo uniquely-identifying information is collected, all information will be used exclusively to inform foo_openlyrics' development, will never be sold or used for marketting (by anybody), and will be deleted after 6 months.\n\nYou can click 'Retry' to view the exact data to be submitted.";
        query.buttons = popup_message_v3::buttonYes | popup_message_v3::buttonNo | popup_message_v3::buttonRetry;
        query.defButton = popup_message_v3::buttonNo;
        query.icon = popup_message_v3::iconQuestion;

        uint32_t popup_result = popup_message_v3::buttonRetry;
        do
        {
            // TODO: What happens with the result if we allow the user to say "don't ask again"?
            popup_result = popup_message_v3::get()->show_query_modal(query);
            if(popup_result == popup_message_v3::buttonRetry)
            {
                popup_message::g_show(m_metrics.c_str(), "OpenLyrics metrics");
            }
            else if(popup_result == popup_message_v3::buttonYes)
            {
                const auto upload_metrics = [metrics = this->m_metrics](threaded_process_status& /*status*/, abort_callback& abort)
                {
                    // TODO submit_metrics(metrics, abort);
                    popup_message_v2::g_show(core_api::get_main_window(), "Thank you for helping to inform the continue development effort!", "OpenLyrics metrics");
                };
                bool start_submit_success = threaded_process::g_run_modeless(threaded_process_callback_lambda::create(upload_metrics),
                                                                             threaded_process::flag_show_abort | threaded_process::flag_show_delayed,
                                                                             core_api::get_main_window(),
                                                                             "Sending metrics for foo_openlyrics...");
                if(!start_submit_success)
                {
                    LOG_WARN("Failed to initate metrics submission");
                }
            }
        } while(popup_result == popup_message_v3::buttonRetry);
    }
};

static initquit_factory_t<foo_send_metrics_on_init> metrics_factory;

void foo_send_metrics_on_init::on_init()
{
    LOG_INFO("on metrics init!");
    if(cfg_metrics_generation.get_value() >= current_metrics_generation)
    {
        return;
    }
    cfg_metrics_generation = current_metrics_generation;

    // TODO: Use our "installed-date" option to delay this by at least a week from first install
    //       (we don't want to bother somebody if they install it, try it out, and then
    //       immediately uninstall it...only if they actually keep it around!)
    //if(cfg_metrics_install_date.get_value() == 0)
    //{
    //    tm tm_epoch = {};
    //    tm_epoch.tm_year = 1970;
    //    tm_epoch.tm_mday = 1;
    //    tm_epoch.tm_mon = 1;
    //    time_t epoch = std::mktime(&tm_epoch);

    //    const time_t now = time(nullptr);

    //    // TODO: init
    //    //cfg_metrics_install_date = 1;
    //}

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