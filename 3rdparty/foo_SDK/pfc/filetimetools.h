#pragma once

namespace pfc {
	typedef uint64_t t_filetimestamp;
	static constexpr t_filetimestamp filetimestamp_invalid = 0;
	static constexpr t_filetimestamp filetimestamp_1second_increment = 10000000;

	t_filetimestamp filetimestamp_from_string(const char * date);
	t_filetimestamp filetimestamp_from_string_utc(const char* date);
	// From ISO 8601 time
	t_filetimestamp filetimestamp_from_string_ISO_8601(const char* date);

	//! Warning: this formats according to system timezone settings, created strings should be used for display only, never for storage.
	pfc::string_formatter format_filetimestamp(t_filetimestamp p_timestamp);
	//! UTC timestamp
	pfc::string_formatter format_filetimestamp_utc(t_filetimestamp p_timestamp);
	//! Local timestamp with milliseconds
	pfc::string_formatter format_filetimestamp_ms(t_filetimestamp p_timestamp);

}
