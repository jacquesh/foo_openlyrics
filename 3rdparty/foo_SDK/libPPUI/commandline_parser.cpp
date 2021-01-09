#include "stdafx.h"
#include "commandline_parser.h"

commandline_parser::commandline_parser() {
	init(pfc::stringcvt::string_utf8_from_os(GetCommandLine()));
}

void commandline_parser::init(const char * cmd)
{
	pfc::string8_fastalloc temp;
	pfc::chain_list_v2_t<pfc::string8> out;
	while(*cmd)
	{
		temp.reset();
		while(*cmd && *cmd!=' ')
		{
			if (*cmd=='\"')
			{
				cmd++;
				while(*cmd && *cmd!='\"') temp.add_byte(*(cmd++));
				if (*cmd == '\"') cmd++;
			}
			else temp.add_byte(*(cmd++));
		}
		out.insert_last(temp);
		while(*cmd==' ') cmd++;
	}
	pfc::list_to_array(m_data,out);
}

size_t commandline_parser::find_param(const char * ptr) const {
	for(size_t n=1;n<m_data.get_size();n++)
	{
		const char * cmd = m_data[n];
		if (cmd[0]=='/') {
			if (!strcmp(cmd+1,ptr)) return n;
		}
	}
	return SIZE_MAX;
}
bool commandline_parser::check_param(const char * ptr) const
{
	return find_param(ptr) != SIZE_MAX;
}

void commandline_parser::build_string(pfc::string_base & out)
{
	out.reset();
	unsigned n;
	for(n=0;n<m_data.get_size();n++)
	{
		const char * cmd = m_data[n];
		if (!out.is_empty()) out += " ";
		if (strchr(cmd,' '))
		{
			out += "\"";
			out += cmd;
			out += "\"";
		}
		else
		{
			out += cmd;
		}
	}
}
