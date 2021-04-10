#pragma once

#define LOG_ERROR(MSG, ...) console::printf("ERROR-OpenLyrics: " MSG, ##__VA_ARGS__)
#define LOG_WARN(MSG, ...) console::printf("WARN-OpenLyrics: " MSG, ##__VA_ARGS__)

#ifdef NDEBUG
#define LOG_INFO(MSG, ...) TRACK_CALL_TEXT(MSG)
#else // NDEBUG
#define LOG_INFO(MSG, ...) console::printf("INFO-OpenLyrics: " MSG, ##__VA_ARGS__)
#endif // NDEBUG
