#pragma once
#include <memory>
#include "pp-COM-macros.h"

namespace PP {

	class CEnumString : public IEnumString {
	public:
		typedef pfc::chain_list_v2_t<pfc::array_t<TCHAR> > t_data;
		typedef std::shared_ptr<t_data> shared_t;

		CEnumString(t_data && in) {
			m_shared = std::make_shared<t_data>(std::move(in));
			Reset();
		}
		CEnumString(const t_data & in) { 
			m_shared = std::make_shared<t_data>(in);
			Reset();
		}
		CEnumString() {
			m_shared = std::make_shared< t_data >();
		}

		void SetStrings(t_data && data) {
			*m_shared = std::move(data);
			Reset();
		}

		static pfc::array_t<TCHAR> stringToBuffer(const char * in) {
			pfc::array_t<TCHAR> arr;
			arr.set_size(pfc::stringcvt::estimate_utf8_to_wide(in));
			pfc::stringcvt::convert_utf8_to_wide_unchecked(arr.get_ptr(), in);
			return arr;
		}

		void AddString(const TCHAR * in) {
			m_shared->insert_last()->set_data_fromptr(in, _tcslen(in) + 1);
			Reset();
		}
		void AddStringU(const char * in, t_size len) {
			pfc::array_t<TCHAR> & arr = *m_shared->insert_last();
			arr.set_size(pfc::stringcvt::estimate_utf8_to_wide(in, len));
			pfc::stringcvt::convert_utf8_to_wide(arr.get_ptr(), arr.get_size(), in, len);
			Reset();
		}
		void AddStringU(const char * in) {
			*m_shared->insert_last() = stringToBuffer(in);
			Reset();
		}
		void ResetStrings() {
			m_shared->remove_all();
			Reset();
		}

		typedef ImplementCOMRefCounter<CEnumString> TImpl;
		COM_QI_BEGIN()
			COM_QI_ENTRY(IUnknown)
			COM_QI_ENTRY(IEnumString)
		COM_QI_END()

		HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched) override {
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

		HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override {
			while (celt > 0) {
				if (m_walk.is_empty()) return S_FALSE;
				--celt; ++m_walk;
			}
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Reset() override {
			m_walk = m_shared->first();
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum) override {
			*ppenum = new TImpl(*this); return S_OK;
		}
	private:
		shared_t m_shared;
		t_data::const_iterator m_walk;
	};
}
