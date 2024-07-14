#include "stdafx.h"
#include "CListAccessible.h"

#include "pp-COM-macros.h"

static size_t IndexFromChildId(LONG id) {return (size_t)(id-1);}
static LONG ChildIdFromIndex(size_t index) {return (LONG)(index+1);}

class IEnumVARIANT_selection : public ImplementCOMRefCounter<IEnumVARIANT> {
public:
	IEnumVARIANT_selection(const LONG * data,size_t dataCount, ULONG pos = 0) : m_pos(pos) {
		m_data.set_data_fromptr(data,dataCount);
	}

	COM_QI_BEGIN()
		COM_QI_ENTRY(IEnumVARIANT)
		COM_QI_ENTRY(IUnknown)
	COM_QI_END()

	// IEnumVARIANT methods
	STDMETHOD(Next)(ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT **ppEnum);
private:
	pfc::array_t<LONG> m_data;
	ULONG m_pos;
};


HRESULT IEnumVARIANT_selection::Next(ULONG celt, VARIANT *rgVar, ULONG *pceltFetched) {
	if (rgVar == NULL) return E_INVALIDARG;

	if (pceltFetched) *pceltFetched = 0;

	ULONG n;
	for (n = 0; n < celt; n++) VariantInit(&rgVar[n]);

	for (n = 0; n < celt && m_pos+n < m_data.get_size(); n++) {
		rgVar[n].vt = VT_I4;
		rgVar[n].lVal = m_data[m_pos+n];
	}

	if (pceltFetched) *pceltFetched = n;

	m_pos += n;

	return n == celt ? S_OK : S_FALSE;
}

HRESULT IEnumVARIANT_selection::Skip(ULONG celt) {
	m_pos += celt;
	if (m_pos > m_data.get_size()) {
		m_pos = (ULONG)m_data.get_size();
		return S_FALSE;
	}
	return S_OK;
}

HRESULT IEnumVARIANT_selection::Reset() {
	m_pos = 0;
	return S_OK;
}

HRESULT IEnumVARIANT_selection::Clone(IEnumVARIANT **ppEnum)
{
	if (ppEnum == NULL)
		return E_INVALIDARG;

	IEnumVARIANT * var;
	try {
		var = new IEnumVARIANT_selection(m_data.get_ptr(), m_data.get_size(), m_pos);
	} catch(std::bad_alloc) {return E_OUTOFMEMORY;}

	var->AddRef();
	*ppEnum = var;

	return S_OK;
}

namespace {
	class WeakRef {
	public:
		WeakRef( CListAccessible * ptr_, std::shared_ptr<bool> ks_ ) : ptr(ptr_), ks(ks_) {}

		CListAccessible * operator->() const { return ptr; }

		bool IsEmpty() const {
			return * ks;
		}
	private:
		CListAccessible * ptr;
		std::shared_ptr<bool> ks;
	};
}

class IAccessible_CListControl : public ImplementCOMRefCounter<IAccessible> {
public:
	IAccessible_CListControl(WeakRef owner) : m_owner(owner) {}
	COM_QI_BEGIN()
		COM_QI_ENTRY(IUnknown)
		COM_QI_ENTRY(IDispatch)
		COM_QI_ENTRY(IAccessible)
	COM_QI_END()

	//IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT * pcTInfo);
	STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr);
	//IAccessible
	STDMETHOD(get_accParent)(IDispatch **ppdispParent); // required
    STDMETHOD(get_accChildCount)(long *pcountChildren);
	STDMETHOD(get_accChild)(VARIANT varChild, IDispatch **ppdispChild);
    STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName); // required
    STDMETHOD(get_accValue)(VARIANT varChild, BSTR *pszValue);
    STDMETHOD(get_accDescription)(VARIANT varChild, BSTR *pszDescription);
    STDMETHOD(get_accRole)(VARIANT varChild, VARIANT *pvarRole); // required
    STDMETHOD(get_accState)(VARIANT varChild, VARIANT *pvarState); // required
    STDMETHOD(get_accHelp)(VARIANT varChild, BSTR *pszHelp);
    STDMETHOD(get_accHelpTopic)(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varChild, BSTR *pszKeyboardShortcut);
    STDMETHOD(get_accFocus)(VARIANT *pvarChild);
    STDMETHOD(get_accSelection)(VARIANT *pvarChildren);
    STDMETHOD(get_accDefaultAction)(VARIANT varChild, BSTR *pszDefaultAction);
    STDMETHOD(accSelect)(long flagsSelect, VARIANT varChild);
    STDMETHOD(accLocation)(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild); // required
    STDMETHOD(accNavigate)(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt);
    STDMETHOD(accHitTest)(long xLeft, long yTop, VARIANT *pvarChild);
    STDMETHOD(accDoDefaultAction)(VARIANT varChild);
    STDMETHOD(put_accName)(VARIANT varChild, BSTR szName);
    STDMETHOD(put_accValue)(VARIANT varChild, BSTR szValue);
private:
	const WeakRef m_owner;
};

void CListAccessible::AccInitialize(CWindow wnd) {
	m_wnd = wnd;
}
void CListAccessible::AccCleanup() {
	if (m_interface.is_valid()) {
		NotifyWinEvent(EVENT_OBJECT_DESTROY, m_wnd, OBJID_CLIENT, CHILDID_SELF);
		m_interface.release();
	}
	m_wnd = NULL;
}

LRESULT CListAccessible::AccGetObject(WPARAM wp,LPARAM lp) {
	const auto dwFlags = (DWORD) wp;
	const auto dwObjId = (DWORD) lp;

	if (dwObjId == OBJID_CLIENT)
	{
		if (m_interface.is_empty())
		{
			try {
				m_interface = new IAccessible_CListControl( WeakRef(this, m_killSwitch) );
			} catch(...) {
				//bah
				return DefWindowProc(m_wnd,WM_GETOBJECT,wp,lp);
			}
			NotifyWinEvent(EVENT_OBJECT_CREATE, m_wnd, OBJID_CLIENT, CHILDID_SELF);
		}
		return LresultFromObject(IID_IAccessible, dwFlags, m_interface.get_ptr());
	}
	else return DefWindowProc(m_wnd,WM_GETOBJECT,wp,lp);
}

void CListAccessible::AccItemNamesChanged(pfc::bit_array const & mask) {
	this->AccRefreshItems(mask, EVENT_OBJECT_NAMECHANGE);
}
void CListAccessible::AccReloadItems(pfc::bit_array const & mask) {
	this->AccRefreshItems(mask, EVENT_OBJECT_NAMECHANGE);
}
void CListAccessible::AccStateChange(pfc::bit_array const & mask) {
	this->AccRefreshItems(mask, EVENT_OBJECT_STATECHANGE);
}
void CListAccessible::AccStateChange(size_t index) {
	this->AccStateChange(pfc::bit_array_one(index));
}
void CListAccessible::AccRefreshItems(pfc::bit_array const & affected, UINT what) {
	if (m_wnd == NULL || m_interface.is_empty()) return;
	//if (GetFocus() != m_hWnd) return;
	const size_t total = AccGetItemCount();
	for (size_t walk = affected.find_first(true, 0, total); walk < total; walk = affected.find_next(true, walk, total)) {
		NotifyWinEvent(what, m_wnd, OBJID_CLIENT, ChildIdFromIndex(walk));
	}
}
void CListAccessible::AccItemLayoutChanged() {
	if (m_wnd == NULL || m_interface.is_empty()) return;
	NotifyWinEvent(EVENT_OBJECT_REORDER, m_wnd, OBJID_CLIENT, CHILDID_SELF);
}
void CListAccessible::AccFocusItemChanged(size_t index) {
	if (m_wnd == NULL || m_interface.is_empty()) return;
	if (GetFocus() != m_wnd) return;
	NotifyWinEvent(EVENT_OBJECT_FOCUS, m_wnd, OBJID_CLIENT, index != SIZE_MAX ? ChildIdFromIndex(index) : CHILDID_SELF);
}
void CListAccessible::AccFocusOtherChanged(size_t index) {
	if (m_wnd == NULL || m_interface.is_empty()) return;
	if (GetFocus() != m_wnd) return;
	const size_t itemCount = this->AccGetItemCount();
	index += itemCount;
	NotifyWinEvent(EVENT_OBJECT_FOCUS, m_wnd, OBJID_CLIENT, index != SIZE_MAX ? ChildIdFromIndex(index) : CHILDID_SELF);
}
void CListAccessible::AccSelectionChanged(const pfc::bit_array & affected, const pfc::bit_array & state) {
	if (m_wnd == NULL || m_interface.is_empty()) return;
	//if (GetFocus() != m_wnd) return;
	const size_t itemCount = this->AccGetItemCount();

	const size_t limit = 20;
	if (affected.calc_count(true, 0, itemCount, limit) == limit) {
		NotifyWinEvent(EVENT_OBJECT_SELECTIONWITHIN, m_wnd, OBJID_CLIENT, CHILDID_SELF);
	} else for(size_t walk = affected.find_first(true,0,itemCount); walk < itemCount; walk = affected.find_next(true,walk,itemCount)) {
		NotifyWinEvent(state[walk] ? EVENT_OBJECT_SELECTIONADD : EVENT_OBJECT_SELECTIONREMOVE, m_wnd, OBJID_CLIENT, ChildIdFromIndex(walk));
	}
}
void CListAccessible::AccLocationChange() {
	if (m_wnd == NULL || m_interface.is_empty()) return;
	NotifyWinEvent(EVENT_OBJECT_LOCATIONCHANGE,m_wnd,OBJID_CLIENT, CHILDID_SELF);
}

HRESULT IAccessible_CListControl::GetTypeInfoCount(UINT * pcTInfo) {
	(void)pcTInfo;
	return E_NOTIMPL;
}

HRESULT IAccessible_CListControl::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo) {
	(void)iTInfo; (void)lcid; (void)ppTInfo;
	return E_NOTIMPL;
}

HRESULT IAccessible_CListControl::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) {
	(void)riid; (void)rgszNames; (void)cNames; (void)lcid; (void)rgDispId;
	return E_NOTIMPL;
}

HRESULT IAccessible_CListControl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pDispParams, VARIANT * pVarResult, EXCEPINFO * pExcepInfo, UINT * puArgErr) {
	(void)dispIdMember; (void)riid; (void)lcid; (void)wFlags; (void)pDispParams; (void)pVarResult; (void)pExcepInfo; (void)puArgErr;
	return E_NOTIMPL;
}

HRESULT IAccessible_CListControl::get_accParent(IDispatch **ppdispParent) // required
{
	if (ppdispParent == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	
	*ppdispParent = NULL;
	HRESULT hResult = AccessibleObjectFromWindow(m_owner->AccGetWnd(), OBJID_WINDOW, IID_IDispatch, (void**)ppdispParent);
	return (hResult  == S_OK) ? S_OK : S_FALSE;
}

HRESULT IAccessible_CListControl::get_accChildCount(long *pcountChildren) {
	if (pcountChildren == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	*pcountChildren = (long)( m_owner->AccGetItemCount() + m_owner->AccGetOtherCount() );
	return S_OK;
}
HRESULT IAccessible_CListControl::get_accChild(VARIANT varChild, IDispatch **ppdispChild) {
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (ppdispChild == NULL) return E_INVALIDARG;
	if (varChild.lVal == CHILDID_SELF) {
		*ppdispChild = this; AddRef();
		return S_OK;
	} else {
		if (m_owner.IsEmpty()) return E_FAIL;
		const size_t index = IndexFromChildId(varChild.lVal);
		const size_t itemCount = m_owner->AccGetItemCount();
		if (index < itemCount) {
			return S_FALSE;
		} else if (index < itemCount + m_owner->AccGetOtherCount()) {
			CWindow wnd = m_owner->AccGetOtherChildWnd(index - itemCount);
			if (wnd == NULL) return S_FALSE;
			if (AccessibleObjectFromWindow(wnd, OBJID_WINDOW, IID_IDispatch, (void**)ppdispChild) != S_OK) return S_FALSE;
			return S_OK;
		} else {
			return E_INVALIDARG;
		}
	}
}


HRESULT IAccessible_CListControl::get_accName(VARIANT varChild, BSTR *pszName) // required
{
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (pszName == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	pfc::string8_fastalloc name;
	if (varChild.lVal == CHILDID_SELF) {
		m_owner->AccGetName(name);
	} else {
		const size_t index = IndexFromChildId(varChild.lVal);
		const size_t itemCount = m_owner->AccGetItemCount();
		if (index < itemCount) {
			m_owner->AccGetItemName(index,name);
		} else if (index < itemCount + m_owner->AccGetOtherCount()) {
			m_owner->AccGetOtherName(index - itemCount, name);
		} else {
			return E_INVALIDARG;
		}
	}
	*pszName = SysAllocString(pfc::stringcvt::string_wide_from_utf8(name));
	return S_OK;
}
HRESULT IAccessible_CListControl::get_accValue(VARIANT varChild, BSTR *pszValue) {
	if (varChild.vt != VT_I4)
		return E_INVALIDARG;
	if (pszValue == NULL)
		return E_INVALIDARG;

	return S_FALSE;
}
HRESULT IAccessible_CListControl::get_accDescription(VARIANT varChild, BSTR *pszDescription) {
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (pszDescription == NULL) return E_INVALIDARG;
	if (varChild.lVal == CHILDID_SELF) return S_FALSE;
	if (m_owner.IsEmpty()) return E_FAIL;
	
	*pszDescription = NULL;

	const size_t itemCount = m_owner->AccGetItemCount();
	const size_t index = IndexFromChildId(varChild.lVal);

	pfc::string_formatter temp;
	if (index < itemCount) {
		if (!m_owner->AccGetItemDescription(index, temp)) return S_FALSE;
	} else if (index < itemCount + m_owner->AccGetOtherCount()) {
		if (!m_owner->AccGetOtherDescription(index - itemCount, temp)) return S_FALSE;
	} else {
		return E_INVALIDARG;
	}
	*pszDescription = SysAllocString(pfc::stringcvt::string_os_from_utf8(temp));
	return S_OK;
}

HRESULT IAccessible_CListControl::get_accRole(VARIANT varChild, VARIANT *pvarRole) // required
{
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (pvarRole == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	VariantClear(pvarRole);
	pvarRole->vt = VT_I4;
	if (varChild.lVal == CHILDID_SELF) {
		pvarRole->lVal = ROLE_SYSTEM_LIST;
	} else {
		const size_t itemCount = m_owner->AccGetItemCount();
		const size_t index = IndexFromChildId(varChild.lVal);
		if (index < itemCount) {
			pvarRole->lVal = m_owner->AccGetItemRole( index );
		} else if (index < itemCount + m_owner->AccGetOtherCount()) {
			pvarRole->lVal = m_owner->AccGetOtherRole(index - itemCount);
		} else {
			return E_INVALIDARG;
		}
	}
	return S_OK;
}

HRESULT IAccessible_CListControl::get_accState(VARIANT varChild, VARIANT *pvarState) // required
{
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (pvarState == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;

	VariantClear(pvarState);
	pvarState->vt = VT_I4;

	if (varChild.lVal == CHILDID_SELF) {
		pvarState->lVal = /*STATE_SYSTEM_NORMAL*/ 0;
		if (GetFocus() == m_owner->AccGetWnd())
			pvarState->lVal |= STATE_SYSTEM_FOCUSED;
	}
	else
	{
		const size_t index = IndexFromChildId(varChild.lVal);
		const size_t itemCount = m_owner->AccGetItemCount();
		if (index < itemCount) {
			pvarState->lVal = STATE_SYSTEM_MULTISELECTABLE | STATE_SYSTEM_SELECTABLE;
			if (GetFocus() == m_owner->AccGetWnd()) {
				pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
				if (m_owner->AccGetFocusItem() == index) pvarState->lVal |= STATE_SYSTEM_FOCUSED;
			}
			if (m_owner->AccIsItemSelected(index)) pvarState->lVal |= STATE_SYSTEM_SELECTED;
			if (!m_owner->AccIsItemVisible(index)) pvarState->lVal |= STATE_SYSTEM_OFFSCREEN | STATE_SYSTEM_INVISIBLE;
			if (m_owner->AccIsItemChecked(index)) pvarState->lVal |= STATE_SYSTEM_CHECKED;
		} else if (index < itemCount + m_owner->AccGetOtherCount()) {
			const size_t indexO = index - itemCount;
			pvarState->lVal = 0;
			if (m_owner->AccIsOtherFocusable(indexO) && GetFocus() == m_owner->AccGetWnd()) {
				pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
				if (m_owner->AccGetFocusOther() == indexO) pvarState->lVal |= STATE_SYSTEM_FOCUSED;
			}
			if (!m_owner->AccIsOtherVisible(indexO)) pvarState->lVal |= STATE_SYSTEM_OFFSCREEN | STATE_SYSTEM_INVISIBLE;
		} else {
			return E_INVALIDARG;
		}
	}
	return S_OK;
}
HRESULT IAccessible_CListControl::get_accHelp(VARIANT varChild, BSTR *pszHelp)
{
	if (varChild.vt != VT_I4)
		return E_INVALIDARG;
	if (pszHelp == NULL)
		return E_INVALIDARG;
	return S_FALSE;
}
HRESULT IAccessible_CListControl::get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic)
{
	if (varChild.vt != VT_I4)
		return E_INVALIDARG;
	if (pszHelpFile == NULL)
		return E_INVALIDARG;
	if (pidTopic == NULL)
		return E_INVALIDARG;
	return S_FALSE;
}
HRESULT IAccessible_CListControl::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{
	(void)varChild;
	if (pszKeyboardShortcut == NULL)
		return E_INVALIDARG;
	return S_FALSE;
}
HRESULT IAccessible_CListControl::get_accFocus(VARIANT *pvarChild)
{
	if (pvarChild == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	VariantClear(pvarChild);
	if (GetFocus() != m_owner->AccGetWnd())
		pvarChild->vt = VT_EMPTY;
	else {
		pvarChild->vt = VT_I4;
		size_t index = m_owner->AccGetFocusItem();
		if (index != ~0) {
			pvarChild->lVal = ChildIdFromIndex(index);
		} else {
			index = m_owner->AccGetFocusOther();
			if (index != ~0) {
				pvarChild->lVal = ChildIdFromIndex(index + m_owner->AccGetItemCount());
			} else {
				pvarChild->lVal = CHILDID_SELF;
			}
		}
	}
	return S_OK;
}
HRESULT IAccessible_CListControl::get_accSelection(VARIANT *pvarChildren)
{
	if (pvarChildren == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	VariantClear(pvarChildren);
	try {
	
		const size_t itemCount = m_owner->AccGetItemCount();
		size_t selCount = 0;
		pfc::bit_array_bittable mask(itemCount);
		for(size_t walk = 0; walk < itemCount; ++walk) {
			bool state = m_owner->AccIsItemSelected(walk);
			mask.set(walk,state);
			if (state) selCount++;
		}

		if (selCount == 0) {
			pvarChildren->vt = VT_EMPTY;
		} else if (selCount == 1) {
			pvarChildren->vt = VT_I4;
			pvarChildren->lVal = ChildIdFromIndex(mask.find_first(true, 0, itemCount));
		} else {
			pfc::array_t<LONG> data; data.set_size(selCount); size_t dataWalk = 0;
			for(size_t walk = mask.find_first(true,0,itemCount); walk < itemCount; walk = mask.find_next(true,walk,itemCount)) {
				data[dataWalk++] = ChildIdFromIndex(walk);
			}
			IEnumVARIANT * ptr = new IEnumVARIANT_selection(data.get_ptr(),data.get_size());
			ptr->AddRef();
			pvarChildren->vt = VT_UNKNOWN;
			pvarChildren->punkVal = ptr;
		}
	} catch(std::bad_alloc) {
		return E_OUTOFMEMORY;
	}
	return S_OK;
}
HRESULT IAccessible_CListControl::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
{
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (pszDefaultAction == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	*pszDefaultAction = NULL;
	if (varChild.lVal == CHILDID_SELF) {
		return S_FALSE;
	} else {
		pfc::string_formatter temp;
		const size_t index = IndexFromChildId(varChild.lVal);
		const size_t itemCount = m_owner->AccGetItemCount();
		if (index < itemCount) {
			if (!m_owner->AccGetItemDefaultAction(temp)) return S_FALSE;
		} else if (index < itemCount + m_owner->AccGetOtherCount()) {
			if (!m_owner->AccGetOtherDefaultAction(index - itemCount, temp)) return false;
		} else {
			return E_INVALIDARG;
		}

		*pszDefaultAction = SysAllocString(pfc::stringcvt::string_os_from_utf8(temp));
		return S_OK;
	}
}
HRESULT IAccessible_CListControl::accSelect(long flagsSelect, VARIANT varChild)
{
	if (varChild.vt != VT_EMPTY && varChild.vt != VT_I4) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	if (varChild.vt == VT_EMPTY || varChild.lVal == CHILDID_SELF)
	{
		switch (flagsSelect)
		{
		case SELFLAG_TAKEFOCUS:
			m_owner->AccGetWnd().SetFocus();
			return S_OK;
		default:
			return DISP_E_MEMBERNOTFOUND;
		}
	}
	else
	{
		const size_t count = m_owner->AccGetItemCount();
		const size_t index = IndexFromChildId(varChild.lVal);
		if (index < count) {
			if (flagsSelect & SELFLAG_TAKESELECTION)
			{
				if (flagsSelect & (SELFLAG_ADDSELECTION | SELFLAG_REMOVESELECTION | SELFLAG_EXTENDSELECTION))
					return E_INVALIDARG;
				m_owner->AccSetSelection(pfc::bit_array_true(), pfc::bit_array_one(index));
			}
			else if (flagsSelect & SELFLAG_EXTENDSELECTION)
			{
				if (flagsSelect & SELFLAG_TAKESELECTION)
					return E_INVALIDARG;
				size_t focus = m_owner->AccGetFocusItem();
				if (focus == ~0) return S_FALSE;
				bool state;
				if (flagsSelect & SELFLAG_ADDSELECTION)
				{
					if (flagsSelect & SELFLAG_REMOVESELECTION)
						return E_INVALIDARG;
					state = true;
				}
				else if (flagsSelect & SELFLAG_REMOVESELECTION)
				{
					if (flagsSelect & SELFLAG_ADDSELECTION)
						return E_INVALIDARG;
					state = false;
				}
				else
				{
					state = m_owner->AccIsItemSelected(focus);
				}
				m_owner->AccSetSelection(pfc::bit_array_range(min(index, focus), max(index, focus)-min(index, focus)+1), pfc::bit_array_val(state));
			}
			else if (flagsSelect & SELFLAG_ADDSELECTION)
			{
				if (flagsSelect & (SELFLAG_REMOVESELECTION | SELFLAG_EXTENDSELECTION))
					return E_INVALIDARG;
				m_owner->AccSetSelection(pfc::bit_array_one(index), pfc::bit_array_true());
			}
			else if (flagsSelect & SELFLAG_REMOVESELECTION)
			{
				if (flagsSelect & (SELFLAG_ADDSELECTION | SELFLAG_EXTENDSELECTION))
					return E_INVALIDARG;
				m_owner->AccSetSelection(pfc::bit_array_one(index), pfc::bit_array_false());
			}

			if (flagsSelect & SELFLAG_TAKEFOCUS) {
				m_owner->AccSetFocusItem(index);
			}
		} else if (index < count + m_owner->AccGetOtherCount()) {
			const size_t indexO = index - count;
			if (flagsSelect & SELFLAG_TAKEFOCUS) {
				m_owner->AccSetFocusOther(indexO);
			}
		} else {
			return E_INVALIDARG;
		}
		return S_OK;
	}
}
HRESULT IAccessible_CListControl::accLocation(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild) // required
{
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (pxLeft == NULL || pyTop == NULL || pcxWidth == NULL || pcyHeight == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	CRect rc;
	const CWindow ownerWnd = m_owner->AccGetWnd();
	if (varChild.lVal == CHILDID_SELF) {
		if (!ownerWnd.GetClientRect(&rc)) return E_FAIL;
	} else {
		const size_t index = IndexFromChildId(varChild.lVal);
		const size_t itemCount = m_owner->AccGetItemCount();
		if (index < itemCount) {
			if (!m_owner->AccGetItemRect(index,rc)) return S_FALSE;
		} else if (index < itemCount + m_owner->AccGetOtherCount()) {
			if (!m_owner->AccGetOtherRect(index - itemCount, rc)) return S_FALSE;
		} else {
			return E_INVALIDARG;
		}
	}

	if (!ownerWnd.ClientToScreen(rc)) return E_FAIL;
	*pxLeft = rc.left;
	*pyTop = rc.top;
	*pcxWidth = rc.right-rc.left;
	*pcyHeight = rc.bottom-rc.top;
	return S_OK;
}
HRESULT IAccessible_CListControl::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt)
{
	if (varStart.vt != VT_I4 && varStart.vt != VT_EMPTY) return E_INVALIDARG;
	if (pvarEndUpAt == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	VariantClear(pvarEndUpAt);
	pvarEndUpAt->vt = VT_EMPTY;
	if (varStart.vt == VT_EMPTY || varStart.lVal == CHILDID_SELF)
	{
		switch (navDir)
		{
		case NAVDIR_LEFT:
		case NAVDIR_RIGHT:
		case NAVDIR_UP:
		case NAVDIR_DOWN:
		case NAVDIR_PREVIOUS:
		case NAVDIR_NEXT:
			// leave empty
			break;
		case NAVDIR_FIRSTCHILD:
			if (m_owner->AccGetItemCount() > 0)
			{
				pvarEndUpAt->vt = VT_I4;
				pvarEndUpAt->lVal = ChildIdFromIndex(0);
			}
			break;
		case NAVDIR_LASTCHILD:
			if (m_owner->AccGetItemCount() > 0)
			{
				pvarEndUpAt->vt = VT_I4;
				pvarEndUpAt->lVal = ChildIdFromIndex(m_owner->AccGetItemCount()-1);
			}
			break;
		}
	}
	else
	{
		const size_t index = IndexFromChildId(varStart.lVal);
		const size_t itemCount = m_owner->AccGetItemCount();
		if (index >= itemCount) {
			if (index < itemCount + m_owner->AccGetOtherCount()) return S_FALSE;
			else return E_INVALIDARG;
		}
		switch (navDir)
		{
		case NAVDIR_LEFT:
		case NAVDIR_RIGHT:
		case NAVDIR_FIRSTCHILD:
		case NAVDIR_LASTCHILD:
			// leave empty
			break;
		case NAVDIR_UP:
		case NAVDIR_PREVIOUS:
			if (index > 0)
			{
				pvarEndUpAt->vt = VT_I4;
				pvarEndUpAt->lVal = ChildIdFromIndex(index-1);
			}
			break;
		case NAVDIR_DOWN:
		case NAVDIR_NEXT:
			if (index+1 < itemCount)
			{
				pvarEndUpAt->vt = VT_I4;
				pvarEndUpAt->lVal = ChildIdFromIndex(index+1);
			}
			break;
		}
	}
	return (pvarEndUpAt->vt != VT_EMPTY) ? S_OK : S_FALSE;
}
HRESULT IAccessible_CListControl::accHitTest(long xLeft, long yTop, VARIANT *pvarChild)
{
	if (pvarChild == NULL) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	VariantClear(pvarChild);
	CPoint pt (xLeft, yTop);
	const CWindow ownerWnd = m_owner->AccGetWnd();
	if (!ownerWnd.ScreenToClient(&pt)) return E_FAIL;
	CRect rcClient;
	if (!ownerWnd.GetClientRect(&rcClient)) return E_FAIL;
	if (PtInRect(&rcClient, pt)) {
		size_t index = m_owner->AccItemHitTest(pt);
		if (index != ~0) {
			pvarChild->vt = VT_I4;
			pvarChild->lVal = ChildIdFromIndex(index);
		} else {
			index = m_owner->AccOtherHitTest(pt);
			if (index != ~0) {
				pvarChild->vt = VT_I4;
				pvarChild->lVal = ChildIdFromIndex(m_owner->AccGetItemCount() + index);
				CWindow wnd = m_owner->AccGetOtherChildWnd(index);
				if (wnd != NULL) {
					IDispatch * obj;
					if (AccessibleObjectFromWindow(wnd,OBJID_WINDOW, IID_IDispatch, (void**)&obj) == S_OK) {
						pvarChild->vt = VT_DISPATCH;
						pvarChild->pdispVal = obj;
					}
				}
			} else {
				pvarChild->vt = VT_I4;
				pvarChild->lVal = CHILDID_SELF;
			}
		}
	} else {
		pvarChild->vt = VT_EMPTY;
	}
	return S_OK;
}
HRESULT IAccessible_CListControl::accDoDefaultAction(VARIANT varChild)
{
	if (varChild.vt != VT_I4) return E_INVALIDARG;
	if (m_owner.IsEmpty()) return E_FAIL;
	if (varChild.lVal == CHILDID_SELF) return S_FALSE;
	const size_t index = IndexFromChildId(varChild.lVal);
	const size_t itemCount = m_owner->AccGetItemCount();
	if (index < itemCount) {
		if (!m_owner->AccExecuteItemDefaultAction(index)) return S_FALSE;
	} else if (index < itemCount + m_owner->AccGetOtherCount()) {
		if (!m_owner->AccExecuteOtherDefaultAction(index - itemCount)) return S_FALSE;
	} else {
		return E_INVALIDARG;
	}
	return S_OK;
}
HRESULT IAccessible_CListControl::put_accName(VARIANT varChild, BSTR szName) {
	(void)varChild; (void)szName;
	return DISP_E_MEMBERNOTFOUND;
}
HRESULT IAccessible_CListControl::put_accValue(VARIANT varChild, BSTR szValue) {
	(void)varChild; (void)szValue;
	return DISP_E_MEMBERNOTFOUND;
}

void CListAccessible::AccGetName(pfc::string_base & out) const {
	auto str = pfc::getWindowText(m_wnd);
	if ( str.length() > 0 ) out = str;
	else out = "List Control";
}
