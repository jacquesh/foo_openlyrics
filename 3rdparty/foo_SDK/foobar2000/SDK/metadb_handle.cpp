#include "foobar2000-sdk-pch.h"
#include "titleformat.h"
#include "file_info_impl.h"
#include "input.h"

double metadb_handle::get_length()
{
	return this->get_info_ref()->info().get_length();
}

t_filetimestamp metadb_handle::get_filetimestamp()
{
	return get_filestats().m_timestamp;
}

t_filesize metadb_handle::get_filesize()
{
	return get_filestats().m_size;
}

bool metadb_handle::format_title_legacy(titleformat_hook * p_hook,pfc::string_base & p_out,const char * p_spec,titleformat_text_filter * p_filter)
{
	service_ptr_t<titleformat_object> script;
	if (titleformat_compiler::get()->compile(script,p_spec)) {
		return format_title(p_hook,p_out,script,p_filter);
	} else {
		p_out.reset();
		return false;
	}
}


bool metadb_handle::g_should_reload_ex(const t_filestats& p_old_stats, const t_filestats& p_new_stats, t_filetimestamp p_readtime) {
	if (p_new_stats.m_timestamp == filetimestamp_invalid) {
		return p_readtime != filetimestamp_invalid;//SNAFU: some locations don't have timestamps at all, let's always accept hints for those when readtime is valid
	} else if (p_old_stats.m_timestamp == filetimestamp_invalid) {
		return true;
	} else if (p_new_stats.m_timestamp > p_old_stats.m_timestamp) {
		return true;
	} else if (p_new_stats.m_timestamp < p_old_stats.m_timestamp) {
		//special case - file has possibly been replaced with older version - check read time
		if (p_readtime == filetimestamp_invalid) return false;
		else return p_readtime > p_old_stats.m_timestamp;
	} else {//timestamps match
		return false;
	}
}
bool metadb_handle::g_should_reload(const t_filestats & p_old_stats,const t_filestats & p_new_stats,bool p_fresh)
{
	if (p_new_stats.m_timestamp == filetimestamp_invalid) return p_fresh;
	else if (p_fresh) return p_old_stats!= p_new_stats;
	else return p_old_stats.m_timestamp < p_new_stats.m_timestamp;
}

bool metadb_handle::should_reload(const t_filestats & p_new_stats, bool p_fresh) const
{
	if (!is_info_loaded_async()) return true;
	else return g_should_reload(get_filestats(),p_new_stats,p_fresh);
}


bool metadb_handle::get_browse_info_merged(file_info & infoMerged) const {
	bool rv = true;
	metadb_info_container::ptr info, browse;
	this->get_browse_info_ref(info, browse);
	if (info.is_valid() && browse.is_valid()) {
		infoMerged = info->info();
		infoMerged.merge_fallback( browse->info() );
	} else if (info.is_valid()) {
		infoMerged = info->info();
	} else if (browse.is_valid()) {
		infoMerged = browse->info();
	} else {
		infoMerged.reset();
		rv = false;
	}
	return rv;
}

namespace {
	class metadb_info_container_impl : public metadb_info_container {
	public:
		metadb_info_container_impl() : m_stats( filestats_invalid ), m_partial() {}
		file_info const & info() {
			return m_info;
		}
		t_filestats const & stats() {
			return m_stats;
		}
		bool isInfoPartial() {
			return m_partial;
		}

		file_info_impl m_info;
		t_filestats m_stats;
		bool m_partial;
		
	};
}

metadb_info_container::ptr metadb_handle::get_full_info_ref( abort_callback & aborter ) const {
	{
		metadb_info_container::ptr info;
		if (this->get_info_ref( info ) ) {
			if (!info->isInfoPartial()) return info;
		}
	}


	input_info_reader::ptr reader;
	input_entry::g_open_for_info_read( reader, NULL, this->get_path(), aborter );
	
	service_ptr_t< metadb_info_container_impl > obj = new service_impl_t<metadb_info_container_impl>();
	obj->m_stats = reader->get_file_stats( aborter );
	reader->get_info( this->get_subsong_index(), obj->m_info, aborter );
	return obj;
}

t_filestats2 metadb_info_container::stats2_() {
	t_filestats2 ret;
	metadb_info_container_v2::ptr v2;
	if (v2 &= this) ret = v2->stats2();
	else {
		auto & s = this->stats();
		ret.m_size = s.m_size;
		ret.m_timestamp = s.m_timestamp;
	}
	return ret;
}

namespace fb2k {
	pfc::string_formatter formatTrackList( metadb_handle_list_cref lst ) {
		pfc::string_formatter ret;
		auto cnt = lst.get_count();
		if ( cnt == 0 ) ret << "[Empty track list]\n";
		else {
			if (cnt == 1) ret << "[Track list: 1 track]\n";
			else ret << "[Track list: " << cnt << " tracks]\n";
			for( size_t walk = 0; walk < cnt; ++ walk ) {
				ret << "  " << lst[walk]->get_location() << "\n";
			}
			ret << "[Track list end]";
		}
		return ret;
	}
	pfc::string_formatter formatTrackTitle(metadb_handle_ptr item, const char * script ) {
		pfc::string_formatter ret;
		item->format_title_legacy(NULL,ret,script,NULL);
		return ret;
	}
	pfc::string_formatter formatTrackTitle(metadb_handle_ptr item,service_ptr_t<class titleformat_object> script) {
		pfc::string_formatter ret;
		item->format_title(NULL,ret,script,NULL);
		return ret;
	}
}

t_filestats2 metadb_handle::get_stats2_() const {
	metadb_handle_v2::ptr v2;
	if (v2 &= const_cast<metadb_handle*>(this)) return v2->get_stats2();
	else return t_filestats2::from_legacy(this->get_filestats());
}

metadb_v2_rec_t metadb_handle::query_v2_() {
#if FOOBAR2000_TARGET_VERSION >= 81
	return static_cast<const metadb_handle_v2*>(this)->query_v2();
#else
	metadb_handle_v2::ptr v2;
	if (v2 &= this) return v2->query_v2();

	metadb_v2_rec_t ret;
	this->get_browse_info_ref(ret.info, ret.infoBrowse);
	return ret;
#endif
}

void metadb_handle::formatTitle_v2_(const metadb_v2_rec_t& rec, titleformat_hook* p_hook, pfc::string_base& p_out, const service_ptr_t<titleformat_object>& p_script, titleformat_text_filter* p_filter) {
#if FOOBAR2000_TARGET_VERSION >= 81
	static_cast<metadb_handle_v2*>(this)->formatTitle_v2(rec, p_hook, p_out, p_script, p_filter);
#else
	metadb_handle_v2::ptr v2;
	if (v2 &= this) {
		v2->formatTitle_v2(rec, p_hook, p_out, p_script, p_filter); return;
	}

	// closest approximate using old APIs
	if (rec.info.is_valid()) {
		this->format_title_from_external_info(rec.info->info(), p_hook, p_out, p_script, p_filter);
	} else {
		this->format_title(p_hook, p_out, p_script, p_filter);
	}
#endif
}