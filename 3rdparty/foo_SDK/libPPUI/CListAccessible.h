#pragma once

#include <memory>

#pragma comment(lib, "oleacc.lib")

#include "CListControl-Cell.h"
#include "CListControlWithSelection.h"

//! Internal class interfacing with Windows accessibility APIs. \n
//! This class is not tied to any specific control and requires most of its methods to be overridden. \n
//! With CListControl you want to use CListControlAccImpl<> template instead of using CListAccessible directly.
class CListAccessible {
public:
	void AccInitialize(CWindow wnd);
	void AccCleanup();
	LRESULT AccGetObject(WPARAM wp,LPARAM lp);
	CWindow AccGetWnd() const {return m_wnd;}
	virtual size_t AccGetItemCount() const {return 0;}
	virtual LONG AccGetItemRole( size_t index ) const {return ROLE_SYSTEM_LISTITEM;}
	virtual void AccGetItemName(size_t index, pfc::string_base & out) const {out = "";}
	virtual void AccGetName(pfc::string_base & out) const;
	virtual size_t AccGetFocusItem() const {return ~0;}
	virtual bool AccIsItemSelected(size_t index) const {return false;}
	virtual bool AccIsItemChecked( size_t index ) const { return false; }
	virtual bool AccIsItemVisible(size_t index) const {return false;}
	virtual bool AccGetItemDefaultAction(pfc::string_base & out) const {return false;}
	virtual bool AccExecuteItemDefaultAction(size_t index) {return false;}
	virtual void AccSetSelection(pfc::bit_array const & affected, pfc::bit_array const & state) {}
	virtual void AccSetFocusItem(size_t index) {}
	virtual bool AccGetItemRect(size_t index, CRect & out) const {return false;}
	virtual bool AccGetItemDescription(size_t index, pfc::string_base & out) const {return false;}
	virtual size_t AccItemHitTest(CPoint const & pt) const {return ~0;}
	
	virtual DWORD AccGetOtherRole(size_t index) {return 0;}
	virtual size_t AccGetOtherCount() const {return 0;}
	virtual void AccGetOtherName(size_t index, pfc::string_base & out) const {out = "";}
	virtual size_t AccGetFocusOther() const {return ~0;}
	virtual void AccSetFocusOther(size_t index) {}
	virtual bool AccIsOtherVisible(size_t index) const {return false;}
	virtual bool AccGetOtherDescription(size_t index, pfc::string_base & out) const {return false;}
	virtual size_t AccOtherHitTest(CPoint const & pt) const {return ~0;}
	virtual bool AccIsOtherFocusable(size_t index) const {return false;}
	virtual bool AccGetOtherDefaultAction(size_t index, pfc::string_base & out) const {return false;}
	virtual bool AccExecuteOtherDefaultAction(size_t index) {return false;}
	virtual bool AccGetOtherRect(size_t index, CRect & out) const {return false;}
	virtual CWindow AccGetOtherChildWnd(size_t index) const {return NULL;}

	void AccItemNamesChanged( pfc::bit_array const & mask);
	void AccReloadItems(pfc::bit_array const & mask );
	void AccRefreshItems(pfc::bit_array const & mask, UINT what);
	void AccStateChange(pfc::bit_array const & mask);
	void AccStateChange(size_t index);
	void AccItemLayoutChanged();
	void AccFocusItemChanged(size_t index);
	void AccFocusOtherChanged(size_t index);
	void AccSelectionChanged(const pfc::bit_array & affected, const pfc::bit_array & status);
	void AccLocationChange();
protected:
	~CListAccessible() { * m_killSwitch = true; }
private:
	CWindow m_wnd;
	std::shared_ptr<bool> m_killSwitch = std::make_shared<bool>();
	pfc::com_ptr_t<IAccessible> m_interface;
};

//! Basic wrapper implementing CListAccessible methods on top of CListControl. Leaves many methods to be overridden by calle
template<typename TBaseClass> class CListControlAccImpl : public TBaseClass, protected CListAccessible {
public:
	template<typename ... arg_t> CListControlAccImpl( arg_t && ... arg ) : TBaseClass(std::forward<arg_t>(arg) ... ) {}

	BEGIN_MSG_MAP_EX(CListControlAccImpl)
		MESSAGE_HANDLER(WM_GETOBJECT,OnGetObject)
		MESSAGE_HANDLER(WM_DESTROY,OnDestroyPassThru)
		MESSAGE_HANDLER(WM_CREATE,OnCreatePassThru)
		CHAIN_MSG_MAP(TBaseClass)
	END_MSG_MAP()
public:
	void ReloadData() {
		TBaseClass::ReloadData();
		AccItemLayoutChanged();
	}
	void ReloadItems(pfc::bit_array const & mask) {
		TBaseClass::ReloadItems(mask);
		AccReloadItems(mask);
	}
protected:
	size_t AccGetItemCount() const {return this->GetItemCount();}
	size_t AccGetFocusItem() const {return this->GetFocusItem();}
	bool AccIsItemSelected(size_t index) const {return this->IsItemSelected(index);}
	bool AccIsItemVisible(size_t index) const {
		if (index >= AccGetItemCount()) return false;
		return this->IsRectVisible(this->GetItemRect(index));
	}
	bool AccExecuteItemDefaultAction(size_t index) {if (index < AccGetItemCount()) {this->ExecuteDefaultAction(index);return true;} else return false;}
	bool AccGetItemRect(size_t index, CRect & out) const {
		if (index >= AccGetItemCount()) return false;
		out = this->GetItemRect(index);
		return true;
	}
	size_t AccItemHitTest(CPoint const & pt) const {
		size_t item;
		if (!this->ItemFromPoint(pt,item)) return ~0;
		return item;
	}

	void OnViewOriginChange(CPoint p_delta) override {
		TBaseClass::OnViewOriginChange(p_delta);
		AccLocationChange();
	}

	/* overrideme, optional
	void AccGetName(pfc::string_base & out) const;
	bool AccGetItemDefaultAction(pfc::string_base & out) const;
	*/

	LONG AccRoleAt(size_t idx, size_t sub) const {
		auto cell = this->GetCellType(idx, sub);
		if (cell == nullptr) return 0;
		return cell->AccRole();
	}
	bool isCellText(size_t idx, size_t sub) const {
		switch (AccRoleAt(idx, sub)) {
		case ROLE_SYSTEM_TEXT:
		case ROLE_SYSTEM_STATICTEXT:
		case ROLE_SYSTEM_LISTITEM:
			return true;
		default:
			return false;
		}
	}
	bool useCellForDescription(size_t idx, size_t sub) const {
		return sub > 0 && isCellText(idx, sub);
	}
	bool AccGetItemDescription(size_t index, pfc::string_base& out) const {
		pfc::string_formatter ret, temp, temp2;
		const size_t total = this->GetColumnCount();
		for (size_t walk = 0; walk < total; ) {
			if (useCellForDescription(index, walk) && this->GetSubItemText(index, walk, temp) && temp.length() > 0) {
				if (ret.length() > 0) ret << "; ";
				this->GetColumnText(walk, temp2);
				if (temp2.length() > 0) ret << temp2 << ": ";
				ret << temp;
			}
			size_t d = this->GetSubItemSpan(index, walk);
			if (d < 1) d = 1;
			walk += d;
		}
		bool rv = (ret.length() > 0);
		if (rv) out = ret;
		return ret;
	}

	void AccGetItemNameAlt(size_t index, pfc::string_base & out) const {
		pfc::string_formatter ret, temp;
		const size_t total = this->GetColumnCount();
		for (size_t walk = 0; walk < total; ) {
			if (this->isCellText(index, walk) && this->GetSubItemText(index, walk, temp) && temp.length() > 0) {
				if (ret.length() > 0) ret << "; ";
				ret << temp;
			}
			size_t d = this->GetSubItemSpan(index, walk);
			if (d < 1) d = 1;
			walk += d;
		}
		out = ret;
	}

	// Item name by default taken from column 0, override this if you supply another
	void AccGetItemName(size_t index, pfc::string_base & out) const {
		this->GetSubItemText(index, 0, out);
	}

	void AccSetSelection(pfc::bit_array const & affected, pfc::bit_array const & state) override {
		this->SetSelection(affected, state); 
}
	void AccSetFocusItem(size_t index) override {
		this->SetFocusItem(index); 
	}
	
	void OnFocusChangedGroup(int iGroup) override {
		TBaseClass::OnFocusChangedGroup(iGroup);
		AccFocusOtherChanged((size_t)iGroup);
	}
	void OnFocusChanged(size_t f) override {
		TBaseClass::OnFocusChanged(f);
		AccFocusItemChanged(f);
	}

	void OnSelectionChanged(pfc::bit_array const & affected, pfc::bit_array const & status) override {
		TBaseClass::OnSelectionChanged(affected, status);
		AccSelectionChanged(affected, status);
	}

	CWindow AccGetOtherChildWnd(size_t index) const {return index == 0 ? CWindow(this->GetHeaderCtrl()) : CWindow(NULL) ;}
	virtual DWORD AccGetOtherRole(size_t index) {return index > 0 ? ROLE_SYSTEM_GROUPING : ROLE_SYSTEM_WINDOW;}//FIXME?
	virtual bool AccGetOtherDescription(size_t index, pfc::string_base & out) const {return false;}//FIXME??

	size_t AccGetOtherCount() const {return 1 + this->GetGroupCount();}
	void AccGetOtherName(size_t index, pfc::string_base & out) const override {
		if (index == 0) out = "Columns Header";
		else if (!this->GetGroupHeaderText((int)index, out)) out = "";
	}
	size_t AccGetFocusOther() const override {
		int focus = this->GetGroupFocus();
		if (focus > 0) return (size_t) focus;
		else return SIZE_MAX;
	}
	void AccSetFocusOther(size_t index) override {
		if (index > 0) this->SetGroupFocus((int)index);
	}
	bool AccIsOtherVisible(size_t index) const override {
		if (index == 0) return true;
		CRect rc;
		if (!this->GetGroupHeaderRect((int)index,rc)) return false;
		return IsRectVisible(rc);
	}
	
	size_t AccOtherHitTest(CPoint const & pt) const {
		{
			CPoint s(pt);
			if (this->ClientToScreen(&s)) {
				CRect rc;
				auto hdr = this->GetHeaderCtrl();
				if (hdr != NULL && hdr.GetWindowRect(rc)) {
					if (rc.PtInRect(s)) {
						return 0;
					}
				}
			}
		}
		int group;
		if (this->GroupHeaderFromPoint(pt,group)) return (size_t) group;
		return ~0;
	}
	bool AccIsOtherFocusable(size_t index) const {return index > 0;}
	bool AccGetOtherDefaultAction(size_t index, pfc::string_base & out) const {return false;}
	bool AccExecuteOtherDefaultAction(size_t index) {return false;}
	bool AccGetOtherRect(size_t index, CRect & out) const {
		if (index == 0) {
			CRect rc, client;

			auto hdr = this->GetHeaderCtrl();
			if ( hdr == NULL ) return false;
			if (!hdr.GetWindowRect(rc)) return false;
			if (!this->ScreenToClient(rc)) return false;
			if (!this->GetClientRect(client)) return false;
			return !! out.IntersectRect(rc, client);
		} else {
			return this->GetGroupHeaderRect((int)index, out);
		}
	}
	LONG AccGetItemRole( size_t index ) const override {
		auto type = this->GetCellType( index, 0 );
		if ( type != nullptr ) {
			return type->AccRole();
		}
		return ROLE_SYSTEM_LISTITEM;
	}
	void SetCellCheckState(size_t item, size_t subItem, bool value) override {
		__super::SetCellCheckState(item, subItem, value);
		this->AccStateChange(item);
	}
	bool AccIsItemChecked( size_t index ) const override {
		auto type = this->GetCellType( index, 0 );
		if ( type != nullptr && type->IsToggle() ) {
			return this->GetCellCheckState( index, 0 );
		}
		return false;
	}
private:
	bool IsRectVisible(CRect const & rc) const {
		return !!CRect().IntersectRect(this->GetClientRectHook(),rc);
	}
	LRESULT OnCreatePassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		bHandled = FALSE;
		try { AccInitialize(*this); } catch(...) {AccCleanup();}
		return 0;
	}
	LRESULT OnDestroyPassThru(UINT,WPARAM,LPARAM,BOOL& bHandled) {
		bHandled = FALSE;
		AccCleanup();
		return 0;
	}
	LRESULT OnGetObject(UINT,WPARAM wp, LPARAM lp, BOOL & ) {
		return this->AccGetObject(wp,lp);
	}
	
};

class CListControlWithSelectionImpl;
typedef CListControlAccImpl< CListControlWithSelectionImpl > CListControlWithSelectionAccImpl;
