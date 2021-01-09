#pragma once
#include <vector>
#include <list>

class text_file_loader_v2 {
public:
	bool m_forceUTF8 = false;
	
	void load(file::ptr f, abort_callback & abort);

	std::vector< const char * > m_lines;

	pfc::string8 m_data;
};