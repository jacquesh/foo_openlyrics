#include "stdafx.h"
#include "AutoComplete.h"
#include "pp-COM-macros.h"

#include <ShlGuid.h> // CLSID_AutoComplete

#include "CEnumString.h"
using PP::CEnumString;

HRESULT InitializeSimpleAC(HWND edit, IUnknown * vals, DWORD opts) {
	pfc::com_ptr_t<IAutoComplete> ac;
	HRESULT hr;
	hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_ALL, IID_IAutoComplete, (void**)ac.receive_ptr());
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - CoCreateInstance/IAutoComplete fail!"); return hr;
	}
	hr = ac->Init(edit, vals, NULL, NULL);
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - ac->Init fail!"); return hr;
	}

	pfc::com_ptr_t<IAutoComplete2> ac2;
	hr = ac->QueryInterface(ac2.receive_ptr());
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - ac->QueryInterface fail!"); return hr;
	}
	hr = ac2->SetOptions(opts);
	if (FAILED(hr)) {
		PFC_ASSERT(!"Should not get here - ac2->SetOptions fail!"); return hr;
	}
	return S_OK;
}

pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<pfc::string8> valueEnum) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	while (valueEnum.is_valid()) {
		acl->AddStringU(*valueEnum); ++valueEnum;
	}
	return acl;
}
pfc::com_ptr_t<IUnknown> CreateACList(pfc::const_iterator<const char *> valueEnum) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	while (valueEnum.is_valid()) {
		acl->AddStringU(*valueEnum); ++valueEnum;
	}
	return acl;
}

pfc::com_ptr_t<IUnknown> CreateACList() {
	return new CEnumString::TImpl();
}

void CreateACList_AddItem(IUnknown * theList, const char * item) {
	static_cast<CEnumString*>(theList)->AddStringU(item);
}

HRESULT InitializeEditAC(HWND edit, pfc::const_iterator<pfc::string8> valueEnum, DWORD opts) {
	pfc::com_ptr_t<IUnknown> acl = CreateACList(valueEnum);
	return InitializeSimpleAC(edit, acl.get_ptr(), opts);
}
HRESULT InitializeEditAC(HWND edit, const char * values, DWORD opts) {
	pfc::com_ptr_t<CEnumString> acl = new CEnumString::TImpl();
	for (const char * walk = values;;) {
		const char * next = strchr(walk, '\n');
		if (next == NULL) { acl->AddStringU(walk, SIZE_MAX); break; }
		acl->AddStringU(walk, next - walk);
		walk = next + 1;
	}
	return InitializeSimpleAC(edit, acl.get_ptr(), opts);
}
