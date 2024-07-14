#include "foobar2000-sdk-pch.h"
#include "cfg_var.h"
#include "configStore.h"

namespace fb2k {
	pfc::string8 formatCfgVarName(const GUID& guid) {
		return pfc::format("cfg_var.", pfc::print_guid(guid));
	}
	pfc::string8 advconfig_autoName(const GUID& id) {
		return pfc::format("advconfig.unnamed.", pfc::print_guid(id));
	}
}
namespace cfg_var_modern {

#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	void cfg_string::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		pfc::string8_fastalloc temp;
		p_stream->read_string_raw(temp, p_abort);
		this->set(temp);
	}

	void cfg_int::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		switch (p_sizehint) {
		case 4:
		{ int32_t v; p_stream->read_lendian_t(v, p_abort); set(v); }
		break;
		case 8:
		{ int64_t v; p_stream->read_lendian_t(v, p_abort); set(v); }
		break;
		}
	}

	void cfg_bool::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		uint8_t b;
		if (p_stream->read(&b, 1, p_abort) == 1) {
			this->set(b != 0);
		}
	}
#endif

	int64_t cfg_int::get() {
		std::call_once(m_init, [this] {
			m_val = fb2k::configStore::get()->getConfigInt(formatName(), m_initVal);
			});
		return m_val;
	}

	void cfg_int::set(int64_t v) {
		if (v != get()) {
			m_val = v;
			fb2k::configStore::get()->setConfigInt(formatName(), v);
		}
	}

	bool cfg_bool::get() {
		std::call_once(m_init, [this] {
			m_val = fb2k::configStore::get()->getConfigBool(formatName(), m_initVal);
			});
		return m_val;
	}

	void cfg_bool::set(bool v) {
		if (v != get()) {
			m_val = v;
			fb2k::configStore::get()->setConfigBool(formatName(), v);
		}
	}

	void cfg_string::set(const char* v) {
		if (strcmp(v, get()) != 0) {
			pfc::string8 obj = v;

			{
				PFC_INSYNC_WRITE(m_valueGuard);
				m_value = std::move(obj);
			}

			fb2k::configStore::get()->setConfigString(formatName(), v);
		}
	}

	void cfg_string::get(pfc::string_base& out) {
		std::call_once(m_init, [this] {
			pfc::string8 v = fb2k::configStore::get()->getConfigString(formatName(), m_initVal)->c_str();
			PFC_INSYNC_WRITE(m_valueGuard);
			m_value = std::move(v);
			});

		PFC_INSYNC_READ(m_valueGuard);
		out = m_value;
	}

	pfc::string8 cfg_string::get() {
		pfc::string8 ret; get(ret); return ret;
	}


	pfc::string8 cfg_var_common::formatVarName(const GUID& guid) {
		return fb2k::formatCfgVarName( guid );
	}

	pfc::string8 cfg_var_common::formatName() const {
		return formatVarName(this->m_guid);
	}

	fb2k::memBlockRef cfg_blob::get() {
		std::call_once(m_init, [this] {
			auto v = fb2k::configStore::get()->getConfigBlob(formatName(), m_initVal);
			PFC_INSYNC_WRITE(m_dataGuard);
			m_data = std::move(v);
			});
		PFC_INSYNC_READ(m_dataGuard);
		return m_data;
	}

	void cfg_blob::set_(fb2k::memBlockRef arg) {
		{
			PFC_INSYNC_WRITE(m_dataGuard);
			m_data = arg;
		}

		auto api = fb2k::configStore::get();
		auto name = formatName();
		if (arg.is_valid()) api->setConfigBlob(name, arg);
		else api->deleteConfigBlob(name);
	}

	void cfg_blob::set(fb2k::memBlockRef arg) {
		auto old = get();
		if (!fb2k::memBlock::equals(old, arg)) {
			set_(arg);
		}
	}

	void cfg_blob::set(const void* ptr, size_t size) {
		set(fb2k::makeMemBlock(ptr, size));
	}

#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	void cfg_blob::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		pfc::mem_block block;
		block.resize(p_sizehint);
		p_stream->read_object(block.ptr(), p_sizehint, p_abort);
		set(fb2k::memBlock::blockWithData(std::move(block)));
	}
#endif

	double cfg_float::get() {
		std::call_once(m_init, [this] {
			m_val = fb2k::configStore::get()->getConfigFloat(formatName(), m_initVal);
			});
		return m_val;
	}

	void cfg_float::set(double v) {
		if (v != get()) {
			m_val = v;
			fb2k::configStore::get()->setConfigFloat(formatName(), v);
		}
	}

#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
	void cfg_float::set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		switch (p_sizehint) {
		case 4:
		{ float v; if (p_stream->read(&v, 4, p_abort) == 4) set(v); }
		break;
		case 8:
		{ double v; if (p_stream->read(&v, 8, p_abort) == 8) set(v); }
		break;
		}
	}
#endif
}
