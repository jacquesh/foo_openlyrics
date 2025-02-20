#pragma once

// NOTE: These macro expand to *two* statements:
//       The first declares the callstack tracker (which adds an entry to the "Call path" at the top of the crash dump text file).
//       The second is the actual log.
//       This means in particular that we can *never* do something like `if(condition) LOG_INFO("foo")` because the second statement will always run then.
//       We must always use braces to denote the scope in which the log macro is placed.
//       We should be doing that anyways but this is the only instance where it would actually be problematic.
//       The reason for this is that if we put both statements inside their own scope then the callstack tracker would be
//       constructed and immediately destructed. This would nullify its use as it would not show up in the call path for a
//       crash that happened later in that scope.

// NOTE: This is effectively the same as `TRACK_CALL_TEXT(MSG)` except multiple statements in the same scope do not cause a name clash
#define LOG_INFO_TRACKER_CONCAT(prefix, suffix) prefix##suffix
#define LOG_INFO_TRACKER_NAME(id) LOG_INFO_TRACKER_CONCAT(TRACKER_, id)

#define LOG_ERROR(MSG, ...) uCallStackTracker LOG_INFO_TRACKER_NAME(__LINE__)(#MSG); do { openlyrics_logging::printf(openlyrics_logging::Level::Error, "ERROR-OpenLyrics: " MSG, ##__VA_ARGS__); }while(false)
#define LOG_WARN(MSG, ...) uCallStackTracker LOG_INFO_TRACKER_NAME(__LINE__)(#MSG); do { openlyrics_logging::printf(openlyrics_logging::Level::Warn, "WARN-OpenLyrics: " MSG, ##__VA_ARGS__); }while(false)
#define LOG_INFO(MSG, ...) uCallStackTracker LOG_INFO_TRACKER_NAME(__LINE__)(#MSG); do { openlyrics_logging::printf(openlyrics_logging::Level::Info, "INFO-OpenLyrics: " MSG, ##__VA_ARGS__); }while(false)

namespace openlyrics_logging
{
    enum class Level
    {
        Info,
        Warn,
        Error
    };

    void printf(Level lvl, const char* fmt, ...);

    struct LogDisabler
    {
        LogDisabler();
        ~LogDisabler();
    };
}
