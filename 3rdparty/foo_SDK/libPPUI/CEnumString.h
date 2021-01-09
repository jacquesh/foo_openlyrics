#pragma once

namespace PP {
	class CEnumString : public IEnumString {
	public:
		typedef pfc::chain_list_v2_t<pfc::array_t<TCHAR> > t_data;
		CEnumString(const t_data & in) : m_data(in) { m_walk = m_data.first(); }
		CEnumString() {}

		void AddString(const TCHAR * in) {
			m_data.insert_last()->set_data_fromptr(in, _tcslen(in) + 1);
			m_walk = m_data.first();
		}
		void AddStringU(const char * in, t_size len) {
			pfc::array_t<TCHAR> & arr = *m_data.insert_last();
			arr.set_size(pfc::stringcvt::estimate_utf8_to_wide(in, len));
			pfc::stringcvt::convert_utf8_to_wide(arr.get_ptr(), arr.get_size(), in, len);
			m_walk = m_data.first();
		}
		void AddStringU(const char * in) {
			pfc::array_t<TCHAR> & arr = *m_data.insert_last();
			arr.set_size(pfc::stringcvt::estimate_utf8_to_wide(in));
			pfc::stringcvt::convert_utf8_to_wide_unchecked(arr.get_ptr(), in);
			m_walk = m_data.first();
		}
		void ResetStrings() { m_walk.invalidate(); m_data.remove_all(); }

		typedef ImplementCOMRefCounter<CEnumString> TImpl;
		COM_QI_BEGIN()
			COM_QI_ENTRY(IUnknown)
			COM_QI_ENTRY(IEnumString)
			COM_QI_END()

			HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched) {
			if (rgelt == NULL) return E_INVALIDARG;
			ULONG done = 0;
			while (done < celt && m_walk.is_valid()) {
				rgelt[done] = CoStrDup(m_walk->get_ptr());
				++m_walk; ++done;
			}
			if (pceltFetched != NULL) *pceltFetched = done;
			return done == celt ? S_OK : S_FALSE;
		}

		static TCHAR * CoStrDup(const TCHAR * in) {
			const size_t lenBytes = (_tcslen(in) + 1) * sizeof(TCHAR);
			TCHAR * out = reinterpret_cast<TCHAR*>(CoTaskMemAlloc(lenBytes));
			if (out) memcpy(out, in, lenBytes);
			return out;
		}

		HRESULT STDMETHODCALLTYPE Skip(ULONG celt) {
			while (celt > 0) {
				if (m_walk.is_empty()) return S_FALSE;
				--celt; ++m_walk;
			}
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Reset() {
			m_walk = m_data.first();
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum) {
			*ppenum = new TImpl(*this); return S_OK;
		}
	private:
		t_data m_data;
		t_data::const_iterator m_walk;
	};
}
