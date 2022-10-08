#pragma once

#include "InPlaceEditTable.h"
#include "GDIUtils.h" // MakeTempBrush()

//! Implements inplace-edit functionality on top of TParent class. Must derive from CListControlHeaderImpl.
template<typename T_CListControl_EditImpl_Parent> class CListControl_EditImpl : public T_CListControl_EditImpl_Parent, protected InPlaceEdit::CTableEditHelperV2 {
public:
	BEGIN_MSG_MAP_EX(CListControl_EditImpl)
		CHAIN_MSG_MAP(T_CListControl_EditImpl_Parent)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT,OnCtlColor);
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC,OnCtlColor);
	END_MSG_MAP()

	// IMPLEMENT ME
	// virtual void TableEdit_SetField(t_size item, t_size subItem, const char * value) = 0;
protected:
	RECT TableEdit_GetItemRect(t_size item, t_size subItem) const override {
		return this->GetSubItemRect(item,subItem);
	}
	void TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount) override {
		lineCount = 1;
		if (!this->GetSubItemText(item,subItem,out)) {
			PFC_ASSERT(!"Should not get here."); out = "";
		}
	}
	HWND TableEdit_GetParentWnd() const override {return this->m_hWnd;}
	t_size TableEdit_GetItemCount() const override {return this->GetItemCount();}
	t_size TableEdit_GetColumnCount() const override {return this->GetColumnCount();}
	void TableEdit_SetItemFocus(t_size item, t_size subItem) override {
		this->TooltipRemove();
		this->SetFocusItem(item); this->SetSelection(pfc::bit_array_true(), pfc::bit_array_one(item));
		auto rcView = this->GetVisibleRectAbs();
		auto rcEdit = this->GetSubItemRectAbs(item,subItem);
		CRect rcTest;
		if (!rcTest.IntersectRect(rcView, rcEdit)) {
			// Only scroll to subitem if entirely invisible
			this->EnsureVisibleRectAbs( rcEdit );
		}
	}
	void TableEdit_GetColumnOrder(t_size * out, t_size count) const override {
		PFC_ASSERT( count == this->GetColumnCount() );
		if ( this->IsHeaderEnabled() ) {
			pfc::array_t<int> temp; temp.set_size(count);
			WIN32_OP_D( this->GetHeaderCtrl().GetOrderArray((int) temp.get_size(), temp.get_ptr()) );
			for(t_size walk = 0; walk < count; ++walk) out[walk] = (t_size) temp[walk];
		} else {
			for(t_size walk = 0; walk < count; ++walk) out[walk] = walk;
		}
	}

	void TableEdit_OnColorsChanged() {}
	bool TableEdit_GetDarkMode() const override {
		return this->GetDarkMode();
	}
	t_uint32 TableEdit_GetEditFlags(t_size item, t_size subItem) const override {
		auto ret = __super::TableEdit_GetEditFlags(item, subItem);
		if (this->GetCellTypeSupported()) {
			auto cell = this->GetCellType(item, subItem);
			if (cell != nullptr) ret |= cell->EditFlags();
		}
		return ret;
	}
	void RequestEditItem(size_t item, size_t subItem) override {
		this->TableEdit_Start(item, subItem);
	}
private:
	LRESULT OnCtlColor(UINT,WPARAM wp,LPARAM lp,BOOL&) {
		CDCHandle dc((HDC)wp);
		const COLORREF bkgnd = this->GetSysColorHook(CListControlImpl::colorBackground);
		dc.SetTextColor(this->GetSysColorHook(CListControlImpl::colorText));
		dc.SetBkColor(bkgnd);
		
		return (LRESULT) MakeTempBrush(dc, bkgnd);
	}
};
