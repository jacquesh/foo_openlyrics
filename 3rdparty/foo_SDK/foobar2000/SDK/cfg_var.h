#pragma once
#include "cfg_var_legacy.h"

using cfg_var_legacy::cfg_var_reader;

#include <atomic>
#include <mutex>
#include <vector>

namespace cfg_var_modern {
	class cfg_var_common : public cfg_var_reader {
	protected:
		cfg_var_common(const GUID& id) : cfg_var_reader(id) {}
		pfc::string8 formatName() const;
	public:
		static pfc::string8 formatVarName(const GUID&);
		cfg_var_common(const cfg_var_common&) = delete;
		void operator=(const cfg_var_common&) = delete;
	};

	class cfg_string : private cfg_var_common {
	public:
		cfg_string(const GUID& id, const char* initVal) : cfg_var_common(id), m_initVal(initVal) {}

		void set(const char* v);
		pfc::string8 get();
		void get(pfc::string_base& out);

		void operator=(const char* v) { set(v); }

		pfc::string8 get_value() { return get(); }

		operator pfc::string8() { return get(); }
	private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

		const pfc::string8 m_initVal;
		pfc::readWriteLock m_valueGuard;
		pfc::string8 m_value;
		std::once_flag m_init;
	};

	typedef cfg_string cfg_string_mt;

	class cfg_int : private cfg_var_common {
	public:
		cfg_int(const GUID& id, int64_t initVal) : cfg_var_common(id), m_initVal(initVal) {}

		int64_t get();
		void set(int64_t v);
		operator int64_t() { return get(); }
		void operator=(int64_t v) { set(v); }

		int64_t get_value() { return get(); }
	private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

		const int64_t m_initVal;
		std::atomic_int64_t m_val = { 0 };
		std::once_flag m_init;
	};

	typedef cfg_int cfg_uint;

	class cfg_bool : private cfg_var_common {
	public:
		cfg_bool(const GUID& id, bool initVal) : cfg_var_common(id), m_initVal(initVal) {}

		bool get();
		void set(bool v);
		operator bool() { return get(); }
		void operator=(bool v) { set(v); }
	private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

		const bool m_initVal;
		std::atomic_bool m_val = { false };
		std::once_flag m_init;
	};

	class cfg_blob : private cfg_var_common {
	public:
		cfg_blob(const GUID& id) : cfg_var_common(id) {}
		cfg_blob(const GUID& id, const void* ptr, size_t size) : cfg_var_common(id) {
			m_initVal = fb2k::makeMemBlock(ptr, size);
		}

		fb2k::memBlockRef get();
		void set(fb2k::memBlockRef ref);
		void set(const void* ptr, size_t size);
	private:
		void set_(fb2k::memBlockRef);
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

		fb2k::memBlockRef m_data, m_initVal;
		pfc::readWriteLock m_dataGuard;
		std::once_flag m_init;
	};

	template<typename struct_t> class cfg_struct_t : private cfg_blob {
	public:
		cfg_struct_t(const GUID& id) : cfg_blob(id) {}
		cfg_struct_t(const GUID& id, const struct_t& init) : cfg_blob(id, &init, sizeof(init)) {}

		struct_t get() {
			struct_t data = {};
			auto v = cfg_blob::get();
			if (v.is_valid() && v->size() == sizeof(data)) {
				memcpy(&data, v->get_ptr(), sizeof(data));
			}
			return data;
		}
		operator struct_t() { return get(); }

		void set(const struct_t& data) { cfg_blob::set(&data, sizeof(data)); }
		void operator=(const struct_t& data) { set(data); }

		struct_t get_value() { return get(); }
	};

	typedef cfg_struct_t<GUID> cfg_guid;

	class cfg_float : private cfg_var_common {
	public:
		cfg_float(const GUID& id, double initVal) : cfg_var_common(id), m_initVal(initVal) {}

		double get();
		void set(double v);
		operator double() { return get(); }
		void operator=(double v) { set(v); }

		double get_value() { return get(); }
	private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override;
#endif

		const double m_initVal;
		std::atomic<double> m_val = { 0 };
		std::once_flag m_init;
	};
}

#if FOOBAR2000_TARGET_VERSION < 81
using namespace cfg_var_legacy;
#else
using namespace cfg_var_modern;
#endif

