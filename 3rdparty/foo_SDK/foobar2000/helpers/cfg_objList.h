#pragma once

#include <SDK/cfg_var.h>

#if FOOBAR2020
#include <SDK/configStore.h>
namespace cfg_var_modern {
	template<typename obj_t>
	class cfg_objList : private cfg_var_common {
	public:
		cfg_objList(const GUID& guid) : cfg_var_common(guid) { }
        template<typename source_t, unsigned source_count> cfg_objList(const GUID& guid, const source_t(&source)[source_count]) : cfg_var_common(guid) {
            m_defaults.resize(source_count);
            for( unsigned walk = 0; walk < source_count; ++ walk) m_defaults[walk] = source[walk];
        }

		void clear() { set(std::vector<obj_t>()); }
		void remove_all() { clear(); }
		size_t remove_mask(pfc::bit_array const& arg) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);
			size_t gone = pfc::remove_mask_t(m_list, arg);
			if (gone > 0) save();
			return gone;
		}
		//! Returns number of items removed.
		template<typename pred_t> size_t remove_if(pred_t&& p) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);
			const auto nBefore = m_list.size();
			const size_t nAfter = pfc::remove_if_t(m_list, std::forward<pred_t>(p));
			const auto gone = nAfter - nBefore;
			if (gone > 0) save();
			return gone;
		}
		bool remove_item(obj_t const& v) {
			return remove_if([&v](const obj_t& arg) { return v == arg; }) != 0;
		}
		size_t size() { init(); PFC_INSYNC_READ(m_listGuard);return m_list.size(); }
		size_t get_count() { return size(); }
		size_t get_size() { return size(); }

		obj_t get(size_t idx) { init(); PFC_INSYNC_READ(m_listGuard); return m_list[idx]; }

		obj_t operator[] (size_t idx) { return get(idx); }

		template<typename arg_t>
		void set(size_t idx, arg_t&& v) {
			init();
			PFC_INSYNC_WRITE(m_listGuard); m_list[idx] = std::forward<arg_t>(v); save();
		}
		bool have_item(obj_t const& arg) {
			return find_item(arg) != SIZE_MAX;
		}
		size_t find_item(obj_t const& arg) {
			init();
			PFC_INSYNC_READ(m_listGuard);
			for (size_t idx = 0; idx < m_list.size(); ++idx) {
				if (m_list[idx] == arg) return idx;
			}
			return SIZE_MAX;
		}
		template<typename arg_t>
		void insert_item(size_t idx, arg_t&& arg) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);
			m_list.insert(m_list.begin() + idx, std::forward<arg_t>(arg));
			save();
		}
		template<typename arg_t>
		void set_items(arg_t&& arg) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);
			m_list.clear();
			for (auto& item : arg) m_list.push_back(item);
			save();
		}
		template<typename arg_t>
		void insert_items(arg_t&& arg, size_t at) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);
			m_list.insert(m_list.begin() + at, arg.begin(), arg.end());
			save();
		}
		template<typename arg_t>
		void add_items(arg_t&& arg) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);

			m_list.insert(m_list.end(), arg.begin(), arg.end());
			save();
		}
		template<typename arg_t>
		void add_item(arg_t&& arg) {
			init();
			PFC_INSYNC_WRITE(m_listGuard);
			m_list.push_back(std::forward<arg_t>(arg));
			save();
		}
		template<typename arg_t>
		void set(arg_t&& arg) {
			init();
			set_(std::forward<arg_t>(arg));
		}
		std::vector<obj_t> get() {
			init();
			PFC_INSYNC_READ(m_listGuard);
			return m_list;
		}


	private:
		void init() {
			std::call_once(m_init, [this] {
				auto blob = fb2k::configStore::get()->getConfigBlob(formatName(), nullptr);
				if (blob.is_valid()) try {
					stream_reader_formatter_simple<> reader(blob->data(), blob->size());
					std::vector<obj_t> data;
					uint32_t count; reader >> count; data.resize(count);
					for (auto& v : data) reader >> v;
					set_(std::move(data), false);
					return;
				} catch(...) {} // fall through, set defaults
				set_(m_defaults, false);
			});
		}
		template<typename arg_t>
		void set_(arg_t&& arg, bool bSave = true) {
			PFC_INSYNC_WRITE(m_listGuard);
			m_list = std::forward<arg_t>(arg);
			if (bSave) save();
		}
		void save() {
			// assumes write sync
			stream_writer_formatter_simple<> out;
			out << (uint32_t)m_list.size();
			for (auto& v : m_list) out << v;
			fb2k::configStore::get()->setConfigBlob(formatName(), out.m_buffer.get_ptr(), out.m_buffer.get_size());
		}
#ifdef FOOBAR2000_HAVE_CFG_VAR_LEGACY
		void set_data_raw(stream_reader* p_stream, t_size p_sizehint, abort_callback& p_abort) override {
			stream_reader_formatter<> reader(*p_stream, p_abort);
			std::vector<obj_t> data;
			uint32_t count; reader >> count; data.resize(count);
			for (auto& v : data) reader >> v;
			set(std::move(data));
		}
#endif

		std::vector<obj_t> m_list;
		pfc::readWriteLock m_listGuard;

		std::once_flag m_init;
        
        std::vector<obj_t> m_defaults;
	};

}
#endif // FOOBAR2020

