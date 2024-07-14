#include "foobar2000-sdk-pch.h"
#include "cfg_var_legacy.h"
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
#include "configStore.h"
#include <mutex>
#endif


#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
namespace fb2k {
	pfc::string8 formatCfgVarName(const GUID&);
}
#endif // FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
namespace cfg_var_legacy {
	cfg_var_reader* cfg_var_reader::g_list = NULL;
	cfg_var_writer* cfg_var_writer::g_list = NULL;

	void cfg_var_reader::config_read_file(stream_reader* p_stream, abort_callback& p_abort)
	{
		pfc::map_t<GUID, cfg_var_reader*> vars;
		for (cfg_var_reader* walk = g_list; walk != NULL; walk = walk->m_next) {
			vars.set(walk->m_guid, walk);
		}
		for (;;) {

			GUID guid;
			t_uint32 size;

			if (p_stream->read(&guid, sizeof(guid), p_abort) != sizeof(guid)) break;
			guid = pfc::byteswap_if_be_t(guid);
			p_stream->read_lendian_t(size, p_abort);

			auto iter = vars.find(guid);
			if ( iter.is_valid() ) {
				stream_reader_limited_ref wrapper(p_stream, size);
				try {
					iter->m_value->set_data_raw(&wrapper, size, p_abort);
				} catch (exception_io_data) {}
				wrapper.flush_remaining(p_abort);
			} else {
				p_stream->skip_object(size, p_abort);
			}
		}
	}
#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	static std::once_flag downgrade_once;
	void cfg_var_reader::downgrade_main() {
		std::call_once(downgrade_once, [] {
			auto api = fb2k::configStore::tryGet();
			if (api.is_valid()) {
				for (cfg_var_reader* walk = g_list; walk != NULL; walk = walk->m_next) {
					walk->downgrade_check(api);
				}
			}
		});
	}
#endif
	void cfg_var_writer::config_write_file(stream_writer* p_stream, abort_callback& p_abort) {
		cfg_var_writer* ptr;
		pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> temp;
		for (ptr = g_list; ptr; ptr = ptr->m_next) {
			temp.set_size(0);
			{
				stream_writer_buffer_append_ref_t<pfc::array_t<t_uint8, pfc::alloc_fast_aggressive> > stream(temp);
				ptr->get_data_raw(&stream, p_abort);
			}
			p_stream->write_lendian_t(ptr->m_guid, p_abort);
			p_stream->write_lendian_t(pfc::downcast_guarded<t_uint32>(temp.get_size()), p_abort);
			if (temp.get_size() > 0) {
				p_stream->write_object(temp.get_ptr(), temp.get_size(), p_abort);
			}
		}
	}


	void cfg_string::get_data_raw(stream_writer* p_stream, abort_callback& p_abort) {
		p_stream->write_object(get_ptr(), length(), p_abort);
	}

	void cfg_string::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		pfc::string8_fastalloc temp;
		p_stream->read_string_raw(temp, p_abort);
		set_string(temp);
	}


#if FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE
	int64_t downgrade_this(fb2k::configStore::ptr api, const char * name, int64_t current) {
		int64_t v = api->getConfigInt(name, INT64_MAX);
		if (v == INT64_MAX) {
			v = api->getConfigInt(name, INT64_MIN);
			if ( v == INT64_MIN ) return current;
		}
		api->deleteConfigInt(name);
		return v;
	}
	uint64_t downgrade_this(fb2k::configStore::ptr api, const char * name, uint64_t current) {
		return (uint64_t) downgrade_this(api, name, (int64_t) current);
	}
	int32_t downgrade_this(fb2k::configStore::ptr api, const char * name, int32_t current) {
		return (int32_t) downgrade_this(api, name, (int64_t) current);
	}
	uint32_t downgrade_this(fb2k::configStore::ptr api, const char * name, uint32_t current) {
		return (uint32_t) downgrade_this(api, name, (int64_t) current);
	}
	bool downgrade_this(fb2k::configStore::ptr api, const char * name, bool current) {
		return downgrade_this(api, name, (int64_t)(current?1:0)) != 0;
	}
	double downgrade_this(fb2k::configStore::ptr api, const char * name, double current) {
		double v = api->getConfigFloat(name, -1);
		if (v == -1) {
			v = api->getConfigFloat(name, 0);
			if ( v == 0 ) return current;
		}
		api->deleteConfigFloat(name);
		return v;
	}
	GUID downgrade_this(fb2k::configStore::ptr api, const char * name, GUID current) {
		auto blob = api->getConfigBlob( name );
		if (blob.is_valid() && blob->size() == sizeof(GUID)) {
			GUID ret;
			memcpy(&ret, blob->get_ptr(), sizeof(ret));
			api->deleteConfigBlob( name );
			return ret;
		}
		return current;
	}

	void cfg_string::downgrade_check(fb2k::configStore::ptr api) {
		const auto name = this->downgrade_name();
		auto v = api->getConfigString(name, nullptr);
		if (v.is_valid()) {
			this->set(v->c_str());
			api->deleteConfigString(name);
		}
	}
	void cfg_string_mt::downgrade_check(fb2k::configStore::ptr api) {
		const auto name = this->downgrade_name();
		auto v = api->getConfigString(name, nullptr);
		if (v.is_valid()) {
			this->set(v->c_str());
			api->deleteConfigString(name);
		}
	}

	pfc::string8 cfg_var_reader::downgrade_name() const {
		if (m_downgrade_name.length() > 0) {
			return m_downgrade_name;
		} else {
			return fb2k::formatCfgVarName(this->m_guid);
		}
	}
#endif

}
#endif // FOOBAR2000_HAVE_CFG_VAR_LEGACY
