#pragma once

class commandline_parser
{
public:
	commandline_parser(const char * p_cmd) { init(p_cmd); }
	commandline_parser();

	inline size_t get_count() const {return m_data.get_size();}
	inline const char * get_item(size_t n) const {return m_data[n];}
	inline const char * operator[](size_t n) const {return m_data[n];}
	bool check_param(const char * p_ptr) const;
	void build_string(pfc::string_base & p_out);
	size_t find_param(const char * ptr) const;
private:
	void init(const char * cmd);
	pfc::array_t<pfc::string8> m_data;
};
