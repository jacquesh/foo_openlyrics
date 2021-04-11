#pragma once

#define LOG_ERROR(MSG, ...) console::printf("ERROR-OpenLyrics: " MSG, ##__VA_ARGS__)
#define LOG_WARN(MSG, ...) console::printf("WARN-OpenLyrics: " MSG, ##__VA_ARGS__)

#ifdef NDEBUG
// NOTE: This is effectively the same as `TRACK_CALL_TEXT(MSG)` except multiple statements in the same scope do not cause a name clash
#define LOG_INFO_TRACKER_CONCAT(prefix, suffix) prefix##suffix
#define LOG_INFO_TRACKER_NAME(id) LOG_INFO_TRACKER_CONCAT(TRACKER_, id)
#define LOG_INFO(MSG, ...) uCallStackTracker LOG_INFO_TRACKER_NAME(__LINE__)(#MSG)
#else // NDEBUG
#define LOG_INFO(MSG, ...) console::printf("INFO-OpenLyrics: " MSG, ##__VA_ARGS__)
#endif // NDEBUG
