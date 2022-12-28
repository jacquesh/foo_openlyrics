#pragma once
#include <memory>
#include "pp-COM-macros.h"

namespace PP {

	class CEnumString : public IEnumString {
	public:
		typedef pfc::chain_list_v2_t<pfc::array_t<TCHAR> > t_data;


		struct shared_t {
			t_data m_data;
			pfc::mutex m_sync;
		};
		typedef std::shared_ptr<shared_t> shared_ptr_t;

		CEnumString(t_data && in) {
			m_shared = std::make_shared<shared_t>();
			m_shared->m_data = std::move(in);
			myReset();
		}
		CEnumString(const t_data & in) { 
			m_shared = std::make_shared<shared_t>();
			m_shared->m_data = in;
			myReset();
		}
		CEnumString() {
			m_shared = std::make_shared< shared_t >();
		}

		void SetStrings(t_data && data) {
			PFC_INSYNC(m_shared->m_sync);
			m_shared->m_data = std::move(data);
			myReset();
		}

		static pfc::array_t<TCHAR> stringToBuffer(const char * in) {
			pfc::array_t<TCHAR> arr;
			arr.set_size(pfc::stringcvt::estimate_utf8_to_wide(in));
			pfc::stringcvt::convert_utf8_to_wide_unchecked(arr.get_ptr(), in);
			return arr;
		}

		void AddString(const TCHAR * in) {
			PFC_INSYNC(m_shared->m_sync);
			m_shared->m_data.insert_last()->set_data_fromptr(in, _tcslen(in) + 1);
			myReset();
		}
		void AddStringU(const char * in, t_size len) {
			PFC_INSYNC(m_shared->m_sync);
			pfc::array_t<TCHAR> & arr = *m_shared->m_data.insert_last();
			arr.set_size(pfc::stringcvt::estimate_utf8_to_wide(in, len));
			pfc::stringcvt::convert_utf8_to_wide(arr.get_ptr(), arr.get_size(), in, len);
			myReset();
		}
		void AddStringU(const char * in) {
			PFC_INSYNC(m_shared->m_sync);
			*m_shared->m_data.insert_last() = stringToBuffer(in);
			myReset();
		}
		void ResetStrings() {
			PFC_INSYNC(m_shared->m_sync);
			m_shared->m_data.remove_all();
			myReset();
		}

		typedef ImplementCOMRefCounter<CEnumString> TImpl;
		COM_QI_BEGIN()
			COM_QI_ENTRY(IUnknown)
			COM_QI_ENTRY(IEnumString)
		COM_QI_END()

		HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched) override {
			PFC_INSYNC(m_shared->m_sync);
			if (rgelt == NULL) return E_INVALIDARG;
			ULONG done = 0;
			while (done < celt && m_walk.is_valid()) {
				rgelt[done] = CoStrDup(m_walk->get_ptr());
				++m_walk; ++done;
			}
			if (pceltFetched != NULL) *pceltFetched = done;
			return done == celt ? S_OK : S_FALSE;
		}

		HRESULT STDMETHODCALLTYPE Skip(ULONG celt) override {
			PFC_INSYNC(m_shared->m_sync);
			while (celt > 0) {
				if (m_walk.is_empty()) return S_FALSE;
				--celt; ++m_walk;
			}
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Reset() override {
			PFC_INSYNC(m_shared->m_sync);
			myReset();
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum) override {
			PFC_INSYNC(m_shared->m_sync);
			*ppenum = new TImpl(*this); return S_OK;
		}
	private:
		void myReset() { m_walk = m_shared->m_data.first(); }

		static TCHAR * CoStrDup(const TCHAR * in) {
			const size_t lenBytes = (_tcslen(in) + 1) * sizeof(TCHAR);
			TCHAR * out = reinterpret_cast<TCHAR*>(CoTaskMemAlloc(lenBytes));
			if (out) memcpy(out, in, lenBytes);
			return out;
		}

		shared_ptr_t m_shared;
		t_data::const_iterator m_walk;
	};
}
