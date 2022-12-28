#pragma once

#include <SDK/cfg_var.h>
#if FOOBAR2020
#include <SDK/configStore.h>
#include <mutex>

namespace cfg_var_modern {
	template<typename obj_t>
	class cfg_obj : public cfg_var_common {
	public:
		cfg_obj(const GUID& id) : cfg_var_common(id), m_initVal() {}
		template<typename arg_t>
		cfg_obj(const GUID& id, arg_t&& v) : cfg_var_common(id), m_initVal(std::forward<arg_t>(v)) {}

		obj_t get() {
			init();
			PFC_INSYNC_READ(m_sync);
			return m_val;
		}
		template<typename arg_t>
		void set(arg_t&& arg) {
			init();
			PFC_INSYNC_WRITE(m_sync);
			m_val = std::forward<arg_t>(arg);
			save();
		}
		template<typename arg_t>
		void operator=(arg_t&& arg) {
			set(std::forward<arg_t>(arg));
		}
	private:
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
			obj_t o;
			try {
				stream_reader_formatter<> fmt(*p_stream, p_abort);
				fmt >> o;
			} catch (exception_io_data) { return; }
			set(std::move(o));
		}
#endif
		void save() {
			// already in write sync
			stream_writer_formatter_simple<> out;
			out << this->m_val;
			fb2k::configStore::get()->setConfigBlob(formatName(), out.m_buffer.get_ptr(), out.m_buffer.get_size());
		}
		void init() {
			std::call_once(m_init, [this] {
				obj_t v = m_initVal;
				auto blob = fb2k::configStore::get()->getConfigBlob(formatName(), nullptr);
				if (blob.is_valid()) {
					try {
						stream_reader_formatter_simple<> source(blob->data(), blob->size());
						source >> v;
					} catch (exception_io_data) {
						v = m_initVal; // revert
					}
				}
				PFC_INSYNC_WRITE(m_sync);
				m_val = std::move(v);
				});
		}

		pfc::readWriteLock m_sync;
		obj_t m_val;
		obj_t m_initVal;
		std::once_flag m_init;
	};
}
#endif // FOOBAR2020