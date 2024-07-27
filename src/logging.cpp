#include "stdafx.h"

#include "logging.h"

// We use this instead of console::printf because that function only supports a small subset of
// format specifiers. In particular it doesn't support 64-bit integers or floats.
void openlyrics_logging::printf(const char* fmt, ...)
{
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
        char* long_buffer = new char[required_bytes+1]; // +1 for null terminator
        va_start(varargs, fmt);
        const int printed_bytes = vsnprintf(long_buffer, required_bytes+1, fmt, varargs);
        va_end(varargs);

        assert(printed_bytes == required_bytes);
        console::print(long_buffer);
        delete[] long_buffer;
    }
}
