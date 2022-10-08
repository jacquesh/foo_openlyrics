#pragma once

#include <functional>

#ifdef FOOBAR2000_MODERN
#include "metadb_compat.h"
#include <SDK/titleformat.h>
#endif

namespace create_directory_helper {
	typedef std::function<const char* (char)> charReplace_t;

	void create_path(const char * p_path,abort_callback & p_abort);
	void make_path(const char * parent,const char * filename,const char * extension,bool allow_new_dirs,pfc::string8 & out,bool b_really_create_dirs,abort_callback & p_dir_create_abort);
	void format_filename(const metadb_handle_ptr & handle,titleformat_hook * p_hook,const char * spec,pfc::string_base & out);
	void format_filename(const metadb_handle_ptr & handle,titleformat_hook * p_hook,titleformat_object::ptr spec,pfc::string_base & out);
	void format_filename_ex(const metadb_handle_ptr & handle,titleformat_hook * p_hook,titleformat_object::ptr spec,const char * suffix, pfc::string_base & out);
	void format_filename_ex(const metadb_handle_ptr & handle, titleformat_hook * p_hook, titleformat_object::ptr spec, const char * suffix, pfc::string_base & out, charReplace_t replace);


	pfc::string sanitize_formatted_path(pfc::stringp str, bool allowWC = false);
	pfc::string sanitize_formatted_path_ex(pfc::stringp str, bool allowWC, charReplace_t replace);

	class titleformat_text_filter_myimpl : public titleformat_text_filter {
	public:
		charReplace_t m_replace;
		void write(const GUID & p_inputType,pfc::string_receiver & p_out,const char * p_data,t_size p_dataLength);
	};

};
