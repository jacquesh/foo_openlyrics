#include "StdAfx.h"
#include "cue_parser.h"

using namespace cue_parser;
using namespace file_info_record_helper;

#define CUE_EMBED_DEBUG 0

#if CUE_EMBED_DEBUG
static void DEBUG_INFO(file_info const& p_info, const char* what) {
	FB2K_console_formatter() << "embeddedcue_metadata_manager : " << what << " >>>>";
	p_info.to_console();
	FB2K_console_formatter() << "<<<<\n";
}
static void DEBUG_INFO(const track_record_list& content, const char* what) {
	FB2K_console_formatter() << "embeddedcue_metadata_manager : " << what << " >>>>";
	for (auto iter = content.first(); iter.is_valid(); ++iter) {
		FB2K_console_formatter() << "track " << iter->m_key << " >>>>";
		file_info_impl temp; iter->m_value.m_info.to_info(temp); temp.to_console();
	}
	FB2K_console_formatter() << "<<<<\n";
}
#else
#define DEBUG_INFO(info, what)
#endif

static void build_cue_meta_name(const char * p_name,unsigned p_tracknumber,pfc::string_base & p_out) {
	p_out.reset();
	p_out << "cue_track" << pfc::format_uint(p_tracknumber % 100,2) << "_" << p_name;
}

static bool is_reserved_meta_entry(const char * p_name) {
	return file_info::field_name_comparator::compare(p_name,"cuesheet") == 0;
}

static bool is_global_meta_entry(const char * p_name) {
	static const char header[] = "cue_track";
	return pfc::stricmp_ascii_ex(p_name,strlen(header),header,~0) != 0;
}
static bool is_allowed_field(const char * p_name) {
	return !is_reserved_meta_entry(p_name) && is_global_meta_entry(p_name);
}
namespace {

	typedef pfc::avltree_t<pfc::string8,file_info::field_name_comparator> field_name_list;

	class __get_tag__enum_fields_enumerator {
	public:
		__get_tag__enum_fields_enumerator(field_name_list & p_out) : m_out(p_out) {}
		void operator() (unsigned p_trackno,const track_record & p_record) {
			if (p_trackno > 0) p_record.m_info.enumerate_meta(*this);
		}
		template<typename t_value> void operator() (const char * p_name,const t_value & p_value) {
			m_out.add(p_name);
		}
	private:
		field_name_list & m_out;
	};


	class __get_tag__is_field_global_check {
	private:
		typedef file_info_record::t_meta_value t_value;
	public:
		__get_tag__is_field_global_check(const char * p_field) : m_field(p_field), m_value(NULL), m_state(true) {}
		
		void operator() (unsigned p_trackno,const track_record & p_record) {
			if (p_trackno > 0 && m_state) {
				const t_value * val = p_record.m_info.meta_query_ptr(m_field);
				if (val == NULL) {m_state = false; return;}
				if (m_value == NULL) {
					m_value = val;
				} else {
					if (pfc::comparator_list<pfc::comparator_strcmp>::compare(*m_value,*val) != 0) {
						m_state = false; return;
					}
				}
			}
		}
		void finalize(file_info_record::t_meta_map & p_globals) {
			if (m_state && m_value != NULL) {
				p_globals.set(m_field,*m_value);
			}
		}
	private:
		const char * const m_field;
		const t_value * m_value;
		bool m_state;
	};

	class __get_tag__filter_globals {
	public:
		__get_tag__filter_globals(track_record_list const & p_tracks,file_info_record::t_meta_map & p_globals) : m_tracks(p_tracks), m_globals(p_globals) {}

		void operator() (const char * p_field) {
			if (is_allowed_field(p_field)) {
				__get_tag__is_field_global_check wrapper(p_field);
				m_tracks.enumerate(wrapper);
				wrapper.finalize(m_globals);
			}
		}
	private:
		const track_record_list & m_tracks;
		file_info_record::t_meta_map & m_globals;
	};

	class __get_tag__local_field_filter {
	public:
		__get_tag__local_field_filter(const file_info_record::t_meta_map & p_globals,file_info_record::t_meta_map & p_output) : m_globals(p_globals), m_output(p_output), m_currenttrack(0) {}
		void operator() (unsigned p_trackno,const track_record & p_track) {
			if (p_trackno > 0) {
				m_currenttrack = p_trackno;
				p_track.m_info.enumerate_meta(*this);
			}
		}
		void operator() (const char * p_name,const file_info_record::t_meta_value & p_value) {
			PFC_ASSERT(m_currenttrack > 0);
			if (!m_globals.have_item(p_name)) {
				build_cue_meta_name(p_name,m_currenttrack,m_buffer);
				m_output.set(m_buffer,p_value);
			}
		}
	private:
		unsigned m_currenttrack;
		pfc::string8_fastalloc m_buffer;
		const file_info_record::t_meta_map & m_globals;
		file_info_record::t_meta_map & m_output;
	};
};

static bool meta_value_equals(const char* v1, const char* v2, bool asNumber) {
	if (asNumber) {
		// Special fix: leading zeros on track numbers
		while( *v1 == '0' ) ++ v1;
		while( *v2 == '0' ) ++ v2;
	}
	return strcmp(v1,v2) == 0;
}

static void strip_redundant_track_meta(unsigned p_tracknumber,const file_info & p_cueinfo,file_info_record::t_meta_map & p_meta,const char * p_metaname, bool asNumber) {
	const size_t metaindex = p_cueinfo.meta_find(p_metaname);
	if (metaindex == SIZE_MAX) return;
	pfc::string_formatter namelocal;
	build_cue_meta_name(p_metaname,p_tracknumber,namelocal);
	{
		const file_info_record::t_meta_value * val = p_meta.query_ptr(namelocal);
		if (val == NULL) return;
		file_info_record::t_meta_value::const_iterator iter = val->first();
		for(t_size valwalk = 0, valcount = p_cueinfo.meta_enum_value_count(metaindex); valwalk < valcount; ++valwalk) {
			if (iter.is_empty()) return;

			if (!meta_value_equals(*iter,p_cueinfo.meta_enum_value(metaindex,valwalk), asNumber)) return;
			++iter;
		}
		if (!iter.is_empty()) return;
	}
	//success
	p_meta.remove(namelocal);
}

// Named get_tag() for backwards compat - it doesn't just get tag, it builds the intended tag from current metadata
// Called prior to file update
void embeddedcue_metadata_manager::get_tag(file_info & p_info) const {
	if (!have_cuesheet()) {
		m_content.query_ptr(0u)->m_info.to_info(p_info);
		p_info.meta_remove_field("cuesheet");
		return;
	}

	cue_creator::t_entry_list entries;
    m_content.enumerate([&entries] (unsigned p_trackno,const track_record & p_record) {
		if (p_trackno > 0) {
			cue_creator::t_entry_list::iterator iter = entries.insert_last();
			iter->m_trackType = "AUDIO";
			iter->m_file = p_record.m_file;
			iter->m_flags = p_record.m_flags;
			iter->m_index_list = p_record.m_index_list;
			iter->m_track_number = p_trackno;
			p_record.m_info.to_info(iter->m_infos);
		}
	} );
	pfc::string_formatter cuesheet;
	cue_creator::create(cuesheet,entries);
	entries.remove_all();
	//parse it back to see what info got stored in the cuesheet and what needs to be stored outside cuesheet in the tags
	cue_parser::parse_full(cuesheet,entries);
	file_info_record output;

	{
		file_info_record::t_meta_map globals;
		//1. find global infos and forward them
		{
			field_name_list fields;
			{ __get_tag__enum_fields_enumerator e(fields); m_content.enumerate(e);}
            { __get_tag__filter_globals e(m_content,globals); fields.enumerate(e); }
		}
			
		output.overwrite_meta(globals);

		//2. find local infos
        {__get_tag__local_field_filter e(globals,output.m_meta); m_content.enumerate(e);}
	}
		

	//strip redundant titles, artists and tracknumbers that the cuesheet already contains
	for(cue_creator::t_entry_list::const_iterator iter = entries.first(); iter.is_valid(); ++iter) {
		strip_redundant_track_meta(iter->m_track_number,iter->m_infos,output.m_meta,"tracknumber", true);
		strip_redundant_track_meta(iter->m_track_number,iter->m_infos,output.m_meta,"title", false);
		strip_redundant_track_meta(iter->m_track_number,iter->m_infos,output.m_meta,"artist", false);
	}


	//add tech infos etc

	{
		const track_record * rec = m_content.query_ptr((unsigned)0);
		if (rec != NULL) {
			output.set_length(rec->m_info.get_length());
			output.set_replaygain(rec->m_info.get_replaygain());
			output.overwrite_info(rec->m_info.m_info);
		}
	}
	output.meta_set("cuesheet",cuesheet);
	output.to_info(p_info);

	DEBUG_INFO( p_info, "get_tag" );
}

static bool resolve_cue_meta_name(const char * p_name,pfc::string_base & p_outname,unsigned & p_tracknumber) {
	//"cue_trackNN_fieldname"
	static const char header[] = "cue_track";
	if (pfc::stricmp_ascii_ex(p_name,strlen(header),header,~0) != 0) return false;
	p_name += strlen(header);
	if (!pfc::char_is_numeric(p_name[0]) || !pfc::char_is_numeric(p_name[1]) || p_name[2] != '_') return false;
	unsigned tracknumber = pfc::atoui_ex(p_name,2);
	if (tracknumber == 0) return false;
	p_name += 3;
	p_tracknumber = tracknumber;
	p_outname = p_name;
	return true;
}

void embeddedcue_metadata_manager::set_tag(file_info const & p_info) {

	DEBUG_INFO( p_info, "set_tag" );

	m_content.remove_all();
	
	{
		track_record & track0 = m_content.find_or_add((unsigned)0);
		track0.m_info.from_info(p_info);
		track0.m_info.m_info.set("cue_embedded","no");
	}
	
	

	const char * cuesheet = p_info.meta_get("cuesheet",0);
	if (cuesheet == NULL) {
		return;
	}

	//processing order
	//1. cuesheet content
	//2. supplement with global metadata from the tag
	//3. overwrite with local metadata from the tag

	{
		cue_creator::t_entry_list entries;
		try {
			cue_parser::parse_full(cuesheet,entries);
		} catch(exception_io_data const & e) {
			console::complain("Attempting to embed an invalid cuesheet", e.what());
			return;
		}

		{
			const double length = p_info.get_length();
			for(cue_creator::t_entry_list::const_iterator iter = entries.first(); iter.is_valid(); ++iter ) {
				if (iter->m_index_list.start() > length) {
					console::info("Invalid cuesheet - index outside allowed range");
					return;
				}
			}
		}

		for(cue_creator::t_entry_list::const_iterator iter = entries.first(); iter.is_valid(); ) {
			cue_creator::t_entry_list::const_iterator next = iter;
			++next;
			track_record & entry = m_content.find_or_add(iter->m_track_number);
			entry.m_file = iter->m_file;
			entry.m_flags = iter->m_flags;
			entry.m_index_list = iter->m_index_list;
			entry.m_info.from_info(iter->m_infos);
			DEBUG_INFO(iter->m_infos, "set_tag cue track info" );
			entry.m_info.from_info_overwrite_info(p_info);
			entry.m_info.m_info.set("cue_embedded","yes");
			double begin = entry.m_index_list.start(), end = next.is_valid() ? next->m_index_list.start() : p_info.get_length();
			if (end <= begin) throw exception_io_data();
			entry.m_info.set_length(end - begin);
			iter = next;
		}
	}
	
	DEBUG_INFO( m_content, "set_tag part 1");

	// == GLOBALS ==
	for(t_size metawalk = 0, metacount = p_info.meta_get_count(); metawalk < metacount; ++metawalk) {
		const char * name = p_info.meta_enum_name(metawalk);
		const t_size valuecount = p_info.meta_enum_value_count(metawalk);
		if (valuecount > 0 && !is_reserved_meta_entry(name) && is_global_meta_entry(name)) {
			m_content.enumerate( [&p_info, metawalk, name, valuecount] ( unsigned p_trackno, track_record & p_record ) {
				if (p_trackno > 0) {
					// Supplement, not overwrite
					// 2021-02-12 fix: prefer whatever has more values
					if (valuecount > p_record.m_info.meta_value_count(name)) {
						p_record.m_info.transfer_meta_entry(name, p_info, metawalk);
					}
				}
			} );
		}
	}

	DEBUG_INFO( m_content, "set_tag part 2");

	// == TRACK LOCALS ==
	{
		pfc::string8_fastalloc namebuffer;
		for(t_size metawalk = 0, metacount = p_info.meta_get_count(); metawalk < metacount; ++metawalk) {
			const char * name = p_info.meta_enum_name(metawalk);
			const t_size valuecount = p_info.meta_enum_value_count(metawalk);
			unsigned trackno;
			if (valuecount > 0 && !is_reserved_meta_entry(name) && resolve_cue_meta_name(name,namebuffer,trackno)) {
				track_record * rec = m_content.query_ptr(trackno);
				if (rec != NULL) {
					rec->m_info.transfer_meta_entry(namebuffer,p_info,metawalk);
				}
			}
		}
	}

	DEBUG_INFO( m_content, "set_tag part 3");
}

void embeddedcue_metadata_manager::get_track_info(unsigned p_track,file_info & p_info) const {
	const track_record * rec = m_content.query_ptr(p_track);
	if (rec == NULL) throw exception_io_data();
	rec->m_info.to_info(p_info);
	DEBUG_INFO( p_info, pfc::format("get_track_info(", p_track, ")" ));
}

void embeddedcue_metadata_manager::set_track_info(unsigned p_track,file_info const & p_info) {
	DEBUG_INFO( p_info, pfc::format("set_track_info(", p_track, ")" ));
	track_record * rec = m_content.query_ptr(p_track);
	if (rec == NULL) throw exception_io_data();
	rec->m_info.from_info_set_meta(p_info);
	rec->m_info.set_replaygain(p_info.get_replaygain());
}

void embeddedcue_metadata_manager::query_track_offsets(unsigned p_track,double & p_begin,double & p_length) const {
	const track_record * rec = m_content.query_ptr(p_track);
	if (rec == NULL) throw exception_io_data();
	p_begin = rec->m_index_list.start();
	p_length = rec->m_info.get_length();
}

bool embeddedcue_metadata_manager::have_cuesheet() const {
	return m_content.get_count() > 1;
}

namespace {
	class _remap_trackno_enumerator {
	public:
		_remap_trackno_enumerator(unsigned p_index) : m_countdown(p_index), m_result(0) {}
		template<typename t_blah> void operator() (unsigned p_trackno,const t_blah&) {
			if (p_trackno > 0 && m_result == 0) {
				if (m_countdown == 0) {
					m_result = p_trackno;
				} else {
					--m_countdown;
				}
			}
		}
		unsigned result() const {return m_result;}
	private:
		unsigned m_countdown;
		unsigned m_result;
	};
};

unsigned embeddedcue_metadata_manager::remap_trackno(unsigned p_index) const {
	if (have_cuesheet()) {
		_remap_trackno_enumerator wrapper(p_index);
		m_content.enumerate(wrapper);
		return wrapper.result();
	} else {
		return 0;
	}	
}

t_size embeddedcue_metadata_manager::get_cue_track_count() const {
	return m_content.get_count() - 1;
}

pfc::string8 embeddedcue_metadata_manager::build_minimal_cuesheet() const {
	cue_creator::t_entry_list entries;
	m_content.enumerate([&entries](unsigned p_trackno, const track_record& p_record) {
		if (p_trackno > 0) {
			cue_creator::t_entry_list::iterator iter = entries.insert_last();
			iter->m_trackType = "AUDIO";
			iter->m_file = "Image.wav";
			iter->m_flags = p_record.m_flags;
			iter->m_index_list = p_record.m_index_list;
			iter->m_track_number = p_trackno;
		}
	});
	pfc::string_formatter cuesheet;
	cue_creator::create(cuesheet, entries);
	return cuesheet;
}