#include "StdAfx.h"
#include "text_file_loader_v2.h"
#include "text_file_loader.h"

void text_file_loader_v2::load(file::ptr f, abort_callback & abort) {
	m_lines.clear();
	bool dummy;
	text_file_loader::read_v2(f, abort, m_data, dummy, m_forceUTF8);

	m_lines.reserve(128);

	char * p = const_cast<char*>(m_data.get_ptr());
	bool line = false;
	const size_t len = m_data.length();
	for (size_t walk = 0; walk < len; ++walk) {
		char & c = p[walk];
		if (c == '\n' || c == '\r') {
			c = 0;
			line = false;
		} else if (!line) {
			m_lines.push_back(&c);
			line = true;
		}
	}
}