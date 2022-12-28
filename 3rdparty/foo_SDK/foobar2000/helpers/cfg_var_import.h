#pragma once
#include "../SDK/cfg_var.h"
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY

class cfg_var_import_common : public cfg_var_reader {
protected:
	const char* const m_varName;
public:
	cfg_var_import_common(const char* name, const GUID& guid) : cfg_var_reader(guid), m_varName(name) {}

	template<typename float_t>
	void importFloat(stream_reader* s, abort_callback& a) {
		float_t f;
		s->read_lendian_t(f, a);
		fb2k::configStore::get()->setConfigFloat(m_varName, f);
	}
	template<typename int_t>
	void importInt(stream_reader* s, abort_callback& a) {
		int_t i;
		s->read_lendian_t(i, a);
		fb2k::configStore::get()->setConfigInt(m_varName, (int64_t)i);
	}

	void importBlob(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) {
		pfc::mem_block temp; temp.resize(p_sizehint);
		p_stream->read_object(temp.ptr(), p_sizehint, p_abort);
		fb2k::configStore::get()->setConfigBlob(m_varName, fb2k::memBlock::blockWithData(std::move(temp)));
	}
};

class cfg_var_import_blob_ : public cfg_var_import_common {
public:
	cfg_var_import_blob_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}
	
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		importBlob(p_stream, p_sizehint, p_abort);
	}
};

class cfg_var_import_int_ : public cfg_var_import_common {
public:
	cfg_var_import_int_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}

	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		switch (p_sizehint) {
		case 1:
			importInt<int8_t>(p_stream, p_abort); break;
		case 2:
			importInt<int16_t>(p_stream, p_abort); break;
		case 4:
			importInt<int32_t>(p_stream, p_abort); break;
		case 8:
			importInt<int64_t>(p_stream, p_abort); break;
		default:
			PFC_ASSERT(!"???");
		}
	}
};

class cfg_var_import_uint_ : public cfg_var_import_common {
public:
	cfg_var_import_uint_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}

	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		switch (p_sizehint) {
		case 1:
			importInt<uint8_t>(p_stream, p_abort); break;
		case 2:
			importInt<uint16_t>(p_stream, p_abort); break;
		case 4:
			importInt<uint32_t>(p_stream, p_abort); break;
		case 8:
			importInt<uint64_t>(p_stream, p_abort); break;
		default:
			PFC_ASSERT(!"???");
		}
	}
};

class cfg_var_import_string_ : public cfg_var_import_common {
public:
	cfg_var_import_string_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}

	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		pfc::string8_fastalloc temp;
		p_stream->read_string_raw(temp, p_abort);
		fb2k::configStore::get()->setConfigString(m_varName, temp);
	}
};

class cfg_var_import_float_ : public cfg_var_import_common {
public:
	cfg_var_import_float_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}
	
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		switch (p_sizehint) {
		case 4:
			importFloat<float>(p_stream, p_abort); break;
		case 8:
			importFloat<double>(p_stream, p_abort); break;
		default: 
			PFC_ASSERT(!"???");
		}
	}
};

class cfg_var_import_bool_ : public cfg_var_import_common {
public:
	cfg_var_import_bool_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}
	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		uint8_t foo[8] = {};
		p_stream->read(foo, 8, p_abort);
		uint8_t acc = 0;
		for (auto b : foo) acc |= b;
		fb2k::configStore::get()->setConfigBool(m_varName, acc != 0);
	}
};

class cfg_var_import_guid_ : public cfg_var_import_common {
public:
	cfg_var_import_guid_(const char* name, const GUID& guid) : cfg_var_import_common(name, guid) {}

	void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
		GUID guid;
		p_stream->read_lendian_t(guid, p_abort);
		fb2k::configStore::get()->setConfigGUID(m_varName, guid);
	}
};

#define FB2K_IMPORT__(name, guid, importer) static importer _FB2K_UNIQUE_NAME(g_cfg_var_import_) ( name, guid );

#define FB2K_IMPORT_INT(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_int_)
#define FB2K_IMPORT_UINT(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_uint_)
#define FB2K_IMPORT_BLOB(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_blob_)
#define FB2K_IMPORT_GUID(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_guid_)
#define FB2K_IMPORT_STRING(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_string_)
#define FB2K_IMPORT_BOOL(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_bool_)
#define FB2K_IMPORT_FLOAT(name, guid) FB2K_IMPORT__(name, guid, cfg_var_import_float_)

#else // FOOBAR2000_HAVE_CFG_VAR_LEGACY

#define FB2K_IMPORT_INT(name, guid)
#define FB2K_IMPORT_UINT(name, guid)
#define FB2K_IMPORT_BLOB(name, guid)
#define FB2K_IMPORT_GUID(name, guid)
#define FB2K_IMPORT_STRING(name, guid)
#define FB2K_IMPORT_BOOL(name, guid)
#define FB2K_IMPORT_FLOAT(name, guid)

#endif // FOOBAR2000_HAVE_CFG_VAR_LEGACY
