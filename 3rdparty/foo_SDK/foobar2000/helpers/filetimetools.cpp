#include "StdAfx.h"

#include "filetimetools.h"

#include <pfc/filetimetools.h>

// Stub - functionality moved to PFC

namespace foobar2000_io {
	t_filetimestamp filetimestamp_from_string(const char* date) {
		return pfc::filetimestamp_from_string( date );
	}
	t_filetimestamp filetimestamp_from_string_utc(const char* date) {
		return pfc::filetimestamp_from_string_utc( date );
	}

	pfc::string_formatter format_filetimestamp(t_filetimestamp p_timestamp) {
		return pfc::format_filetimestamp( p_timestamp );
	}

	pfc::string_formatter format_filetimestamp_utc(t_filetimestamp p_timestamp) {
		return pfc::format_filetimestamp_utc( p_timestamp );
	}

	pfc::string_formatter format_filetimestamp_ms(t_filetimestamp p_timestamp) {
		return pfc::format_filetimestamp_ms( p_timestamp );
	}
}
