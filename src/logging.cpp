#include "stdafx.h"

#include "logging.h"
#include "preferences.h"

enum class VerboseLogConfig
{
    Unknown,
    Enabled,
    Disabled
};
static std::atomic<VerboseLogConfig> g_verbose_logs = VerboseLogConfig::Unknown;
static std::atomic<bool> g_config_read_complete = false;

// We use this instead of console::printf because that function only supports a small subset of
// format specifiers. In particular it doesn't support 64-bit integers or floats.
void openlyrics_logging::printf(openlyrics_logging::Level lvl, const char* fmt, ...)
{
    if(!g_config_read_complete && (lvl == Level::Info))
    {
        if(g_verbose_logs == VerboseLogConfig::Unknown)
        {
            const std::string verbose_file_path = std::string(core_api::get_profile_path())
                                                  + "\\openlyrics-verbose-log.txt";
            const bool verbose_logs = filesystem::g_exists(verbose_file_path.c_str(), fb2k::noAbort);
            g_verbose_logs = verbose_logs ? VerboseLogConfig::Enabled : VerboseLogConfig::Disabled;
        }

        // If the verbose-log file isn't there then don't emit info logs until we've finished loading config,
        // so that we can tell whether or not we're supposed to emit info logs.
        if(g_verbose_logs != VerboseLogConfig::Enabled)
        {
            return;
        }
    }

    // Only skip printing debug logs if we're finished reading config, because if we haven't
    // read config in yet then we can't really tell if debug logs have been enabled in config.
    if(g_config_read_complete && !preferences::display::debug_logs_enabled() && (lvl == Level::Info))
    {
        return;
    }

    char short_buffer[512];
    va_list varargs;
    va_start(varargs, fmt);
    const int required_bytes = vsnprintf(short_buffer, sizeof(short_buffer), fmt, varargs);
    va_end(varargs);

    // While unlikely, its possible that our short buffer wasn't big enough for this log message.
    // In that case use the result that we have to allocate a large enough buffer and print that.
    if(required_bytes + 1 <= (int)sizeof(short_buffer)) // +1 for the null-terminator
    {
        console::print(short_buffer);
    }
    else
    {
        char* long_buffer = new char[required_bytes + 1]; // +1 for null terminator
        va_start(varargs, fmt);
        const int printed_bytes = vsnprintf(long_buffer, required_bytes + 1, fmt, varargs);
        va_end(varargs);

        assert(printed_bytes == required_bytes);
        console::print(long_buffer);
        delete[] long_buffer;
    }
}

FB2K_ON_INIT_STAGE([]() { LOG_INFO("Verbose logs are enabled"); }, init_stages::before_config_read)
FB2K_ON_INIT_STAGE([]() { g_config_read_complete = true; }, init_stages::after_config_read)
