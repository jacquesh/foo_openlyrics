#include "stdafx.h"
#include "CListControlComplete.h"
#include "CListControl-Subst.h"
#include "CWindowCreateAndDelete.h"
#include <pfc/string-conv-lite.h>
#include "ImplementOnFinalMessage.h"
#include "CListControl-Cells.h"
#include "windowLifetime.h"

#define I_IMAGEREALLYNONE (-3)

namespace {

	static int safeFillText(wchar_t* psz, int cch, pfc::wstringLite const& text) {
		int len = (int)text.length();
		int lim = cch - 1;
		if (len > lim) len = lim;
		for (int i = 0; i < len; ++i) psz[i] = text[i];
		psz[len] = 0;
		return len;
	}

	class CListControl_ListViewBase : public CListControlComplete {
	protected:
		const DWORD m_style;
		DWORD m_listViewExStyle = 0;
		CImageList m_imageLists[4];
		bool m_groupViewEnabled = false;
	public:
		CListControl_ListViewBase(DWORD style) : m_style(style) {
			if (style & LVS_SINGLESEL) this->SetSelectionModeSingle();
			else this->SetSelectionModeMulti();
		}

		BEGIN_MSG_MAP_EX(CListControl_ListViewBase)
			MSG_WM_CREATE(OnCreate)
			MESSAGE_HANDLER_EX(LVM_INSERTCOLUMN, OnInsertColumn)
			MESSAGE_HANDLER_EX(LVM_DELETECOLUMN, OnDeleteColumn)
			MESSAGE_HANDLER_EX(LVM_SETCOLUMN, OnSetColumn)
			MESSAGE_HANDLER_EX(LVM_SETCOLUMNWIDTH, OnSetColumnWidth)
			MESSAGE_HANDLER_EX(LVM_GETCOLUMNWIDTH, OnGetColumnWidth)
			MESSAGE_HANDLER_EX(LVM_GETCOLUMNORDERARRAY, OnGetColumnOrderArray)
			MESSAGE_HANDLER_EX(LVM_SETCOLUMNORDERARRAY, OnSetColumnOrderArray)
			MESSAGE_HANDLER_EX(LVM_GETITEMCOUNT, OnGetItemCount)
			MESSAGE_HANDLER_EX(LVM_GETITEMRECT, OnGetItemRect)
			MESSAGE_HANDLER_EX(LVM_GETSUBITEMRECT, OnGetSubItemRect)
			MESSAGE_HANDLER_EX(LVM_HITTEST, OnHitTest)
			MESSAGE_HANDLER_EX(LVM_GETITEMSTATE, OnGetItemState)
			MESSAGE_HANDLER_EX(LVM_SETITEMSTATE, OnSetItemState)
			MESSAGE_HANDLER_EX(LVM_GETNEXTITEM, OnGetNextItem)
			MESSAGE_HANDLER_EX(LVM_GETNEXTITEMINDEX, OnGetNextItemIndex)
			MESSAGE_HANDLER_EX(LVM_SUBITEMHITTEST, OnSubItemHitTest)
			MESSAGE_HANDLER_EX(LVM_GETSELECTEDCOUNT, OnGetSelCount)
			MESSAGE_HANDLER_EX(LVM_GETHEADER, OnGetHeader)
			MESSAGE_HANDLER_EX(LVM_SETEXTENDEDLISTVIEWSTYLE, OnSetExtendedListViewStyle)
			MESSAGE_HANDLER_EX(LVM_GETEXTENDEDLISTVIEWSTYLE, OnGetExtendedListViewStyle)
			MESSAGE_HANDLER_EX(LVM_ENSUREVISIBLE, OnEnsureVisible)
			MESSAGE_HANDLER_EX(LVM_SETIMAGELIST, OnSetImageList)
			MESSAGE_HANDLER_EX(LVM_GETIMAGELIST, OnGetImageList)
			MESSAGE_HANDLER_EX(LVM_EDITLABEL, OnEditLabel)
			MESSAGE_HANDLER_EX(LVM_GETSTRINGWIDTH, OnGetStringWidth)
			MESSAGE_HANDLER_EX(LVM_ENABLEGROUPVIEW, OnEnableGroupView)
			MESSAGE_HANDLER_EX(LVM_SCROLL, OnScroll)
			MESSAGE_HANDLER_EX(LVM_REDRAWITEMS, OnRedrawItems)
			CHAIN_MSG_MAP(CListControlComplete)
		END_MSG_MAP()

		LRESULT OnCreate(LPCREATESTRUCT) {
			SetMsgHandled(FALSE);
			// Adopt style flags of the original control to keep various ATL checks happy
			DWORD style = GetStyle();
			style = (style & 0xFFFF0000) | (m_style & 0xFFFF);
			SetWindowLong(GWL_STYLE, style);
			return 0;
		}

		LRESULT OnGetColumnWidth(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t)wp;
			return GetSubItemWidth(idx);
		}
		LRESULT OnSetColumnWidth(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx >= GetColumnCount()) return FALSE;
			// undocumented: low 16 bits must be used, or else testing against LVSCW_AUTOSIZE fails hard
			// all macros sending LVM_SETCOLUMNWIDTH use low 16 bits of LPARAM
			int w = (short)lp; 
			if (this->IsHeaderless()) {
				if (w < 0) {
					m_headerlessColumns[idx] = UINT32_MAX;
				} else {
					m_headerlessColumns[idx] = (uint32_t)w;
					this->OnColumnsChanged();
				}
			} else {
				uint32_t pass = (uint32_t)w;
				if (w < 0) {
					if (w == LVSCW_AUTOSIZE_USEHEADER) pass = columnWidthAuto;
					else pass = columnWidthAutoUseContent;
				}
				this->ResizeColumn(idx, pass);
			}
			return TRUE;
		}
		LRESULT OnGetColumnOrderArray(UINT, WPARAM wp, LPARAM lp) {
			int* out = (int*)lp;
			auto arr = this->GetColumnOrderArray();
			if ((size_t)wp != arr.size()) return 0;
			for (size_t w = 0; w < arr.size(); ++w) out[w] = arr[w];
			return 1;
		}
		LRESULT OnSetColumnOrderArray(UINT, WPARAM wp, LPARAM lp) {
			if (this->IsHeaderless()) {
				PFC_ASSERT(!"Implement and test me");
				return 0;
			} else {
				auto hdr = GetHeaderCtrl();
				WIN32_OP_D(hdr.SetOrderArray((int)wp, (int*)lp));
			}
			ReloadData();
			return 1;
		}
		LRESULT OnInsertColumn(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			auto col = reinterpret_cast<LVCOLUMN*>(lp);

			if (IsHeaderless()) {
				if (idx > m_headerlessColumns.size()) idx = m_headerlessColumns.size();
				uint32_t width = 0;
				if (col->mask & LVCF_WIDTH) width = col->cx;
				m_headerlessColumns.insert(m_headerlessColumns.begin() + idx, width);
				this->OnColumnsChanged();
				return 0;
			}
			if (this->GetHeaderCtrl() == NULL) {
				if (m_style & LVS_NOSORTHEADER) {
					this->InitializeHeaderCtrl();
				} else {
					this->InitializeHeaderCtrlSortable();
				}
			}

			PFC_ASSERT(this->GetHeaderCtrl().GetItemCount() == (int)this->GetColumnCount());

			if (idx != this->GetColumnCount()) {
				PFC_ASSERT(!"Arbitrary column insert not implemented, please add columns in order");
				return -1;
			}

			pfc::string8 label; int width = 0; DWORD format = HDF_LEFT;
			if (col->mask & LVCF_TEXT) label = pfc::utf8FromWide(col->pszText);
			if (col->mask & LVCF_WIDTH) width = col->cx;
			if (col->mask & LVCF_FMT) {
				format = col->fmt & LVCFMT_JUSTIFYMASK;
			}
			this->AddColumn(label, width, format);

			PFC_ASSERT(this->GetHeaderCtrl().GetItemCount() == (int)this->GetColumnCount());

			return idx;
		}
		LRESULT OnDeleteColumn(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t)wp;
			if (idx >= this->GetColumnCount()) {
				PFC_ASSERT(!"???"); return FALSE;
			}
			this->DeleteColumn(idx);
			return TRUE;
		}
		LRESULT OnSetColumn(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx >= this->GetColumnCount()) {
				PFC_ASSERT(!"???"); return FALSE;
			}
			auto col = reinterpret_cast<LVCOLUMN*>(lp);
			
			if (col->mask & (LVCF_TEXT | LVCF_FMT)) {
				const char* pText = nullptr; DWORD format = UINT32_MAX;
				pfc::string8 strText;
				if (col->mask & LVCF_TEXT) {
					strText = pfc::utf8FromWide(col->pszText);
					pText = strText.c_str();
				}
				if (col->mask & LVCF_FMT) {
					format = col->fmt & LVCFMT_JUSTIFYMASK;
				}
				this->SetColumn(idx, pText, format);
			}

			if (col->mask & LVCF_WIDTH) {
				this->ResizeColumn(idx, col->cx);
			}

			return TRUE;
		}
		LRESULT OnGetItemCount(UINT, WPARAM, LPARAM) {
			return (LRESULT)this->GetItemCount();
		}

		LRESULT OnGetItemRect(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			RECT* rc = (RECT*)lp;
			if (idx < GetItemCount()) {
				// LVIR_* not supported
				* rc = GetItemRect(idx);
				return TRUE;
			}
			return FALSE;
		}

		LRESULT OnGetSubItemRect(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			RECT* rc = (RECT*)lp;
			if (idx < GetItemCount()) {
				size_t subItem = (size_t)rc->top;
				// LVIR_* not supported
				*rc = GetSubItemRect(idx, subItem);
				return TRUE;
			}
			return FALSE;
		}

		LRESULT OnHitTest(UINT, WPARAM, LPARAM lp) {
			auto info = (LVHITTESTINFO*)lp;
			CPoint pt = this->PointClientToAbs(info->pt);
			size_t item = this->ItemFromPointAbs(pt);
			size_t subItem = SIZE_MAX;
			if (item != SIZE_MAX) {
				info->iItem = (int)item;
				size_t subItem = this->SubItemFromPointAbs(pt);
				info->iSubItem = (subItem != SIZE_MAX) ? (int)subItem : -1;
			}
			return item != SIZE_MAX ? (LRESULT)item : (LRESULT)-1;
		}
		LRESULT OnSubItemHitTest(UINT, WPARAM, LPARAM lp) {
			auto info = (LVHITTESTINFO*)lp;
			CPoint pt = this->PointClientToAbs(info->pt);
			size_t item = this->ItemFromPointAbs(pt);
			size_t subItem = SIZE_MAX;
			if (item != SIZE_MAX) {
				info->iItem = (int)item;
				subItem = this->SubItemFromPointAbs(pt);
				info->iSubItem = (subItem != SIZE_MAX) ? (int)subItem : -1;
			}
			return subItem != SIZE_MAX ? (LRESULT)subItem : (LRESULT)-1;
		}

		virtual void SetItemState(size_t idx, DWORD mask, DWORD state) {
			if (idx >= GetItemCount()) return;
			if (mask & LVIS_FOCUSED) {
				if (state & LVIS_FOCUSED) {
					this->SetFocusItem(idx);
				} else {
					if (this->GetFocusItem() == idx) {
						this->SetFocusItem(SIZE_MAX);
					}
				}
			}
			if (mask & LVIS_SELECTED) {
				if (this->IsSingleSelect()) {
					if (state & LVIS_SELECTED) this->SelectSingle(idx);
				} else {
					this->SetSelectionAt(idx, (state & LVIS_SELECTED) != 0);
				}
			}
		}
		LRESULT OnSetItemState(UINT, WPARAM wp, LPARAM lp) {
			LVITEM* pItem = (LVITEM*)lp;
			if ((LONG_PTR)wp < 0) {
				if (pItem->stateMask & LVIS_FOCUSED) {
					if (pItem->state & LVIS_FOCUSED) {
						// makes no sense
					} else {
						this->SetFocusItem(SIZE_MAX);
					}
				}
				if (pItem->stateMask & LVIS_SELECTED) {
					if (pItem->state & LVIS_SELECTED) {
						this->SelectAll();
					} else {
						this->SelectNone();
					}
				}
			} else {
				size_t idx = (size_t)wp;
				SetItemState(idx, pItem->stateMask, pItem->state);
			}
			return TRUE;
		}

		virtual UINT GetItemState(size_t idx) const {
			UINT ret = 0;
			if (idx == this->GetFocusItem()) ret |= LVIS_FOCUSED;
			if (this->IsItemSelected(idx)) ret |= LVIS_SELECTED;
			return ret;
		}
		LRESULT OnGetItemState(UINT, WPARAM wp, LPARAM lp) {
			LRESULT ret = 0;
			size_t idx = (size_t)wp;
			if (idx < GetItemCount()) {
				ret |= (this->GetItemState(idx) & lp);
			}
			return ret;
		}
		
		LRESULT OnGetNextItemIndex(UINT, WPARAM wp, LPARAM lp) {
			LVITEMINDEX* info = (LVITEMINDEX*)wp;
			int item = (int)OnGetNextItem(LVM_GETNEXTITEM, info->iItem, lp);
			info->iItem = item;
			return item >= 0;
		}
		LRESULT OnGetNextItem(UINT, WPARAM wp, LPARAM lp) {
			// GetSelectedIndex() :
			// return (int)::SendMessage(this->m_hWnd, LVM_GETNEXTITEM, (WPARAM)-1, MAKELPARAM(LVNI_ALL | LVNI_SELECTED, 0));
			
			const bool bVisible = (lp & LVNI_VISIBLEONLY) != 0;
			std::pair<size_t, size_t> range;
			if ( bVisible ) range = this->GetVisibleRange();

			if ((lp & LVNI_STATEMASK) == LVNI_FOCUSED) {
				size_t ret = this->GetFocusItem();
				if (bVisible) {
					if (ret < range.first || ret >= range.second) ret = SIZE_MAX;
				}
				return (LRESULT)ret;
			}

			const auto count = this->GetItemCount();

			if (wp == (WPARAM)-1 && bVisible) {
				// Simplified visible range search
				for (size_t idx = range.first; idx < range.second; ++idx) {
					if (this->GetItemState(idx) & lp) {
						return (LRESULT)idx;
					}
				}
				return -1;
			}
			
			size_t base;
			if ((LONG_PTR)wp < 0) base = 0;
			else base = wp + 1;
			for (size_t idx = base; idx < count; ++idx) {
				if (bVisible) {
					if (idx < range.first || idx >= range.second) continue;
				}
				if (this->GetItemState(idx) & lp) {
					return (LRESULT)idx;
				}
			}
			return -1;
		}
		LRESULT OnGetSelCount(UINT, WPARAM, LPARAM) {
			return (LRESULT) this->GetSelectedCount();
		}
		LRESULT OnGetHeader(UINT, WPARAM, LPARAM) {
			return (LRESULT)GetHeaderCtrl().m_hWnd;
		}
		LRESULT OnSetExtendedListViewStyle(UINT, WPARAM wp, LPARAM lp) {
			DWORD ret = m_listViewExStyle;
			DWORD set = (DWORD)lp, mask = (DWORD)wp;
			if (mask == 0) mask = 0xFFFFFFFF;
			m_listViewExStyle = (m_listViewExStyle & ~mask) | (set & mask);

			this->SetRowStyle((m_listViewExStyle & LVS_EX_GRIDLINES) ? rowStyleGrid : rowStyleDefault);

			if (m_listViewExStyle != ret) ReloadData();
			return ret;
		}
		LRESULT OnGetExtendedListViewStyle(UINT, WPARAM, LPARAM) {
			return m_listViewExStyle;
		}
		LRESULT OnEnsureVisible(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx < this->GetItemCount()) {
				(void)lp; // FIX ME partialOK?
				this->EnsureItemVisible(idx);
				return TRUE;
			}
			return FALSE;
		}
		LRESULT OnSetImageList(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			LRESULT ret = 0;
			if (idx < std::size(m_imageLists)) {
				auto& ref = m_imageLists[idx];
				ret = (LRESULT)ref.m_hImageList;
				ref = (HIMAGELIST)lp;
			}
			return ret;
		}
		LRESULT OnGetImageList(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			LRESULT ret = 0;
			if (idx < std::size(m_imageLists)) {
				ret = (LRESULT)m_imageLists[idx].m_hImageList;
			}
			return ret;
		}
		LRESULT OnEditLabel(UINT, WPARAM wp, LPARAM) {
			int index = (int)wp;
			HWND ret = NULL;
			if (index < 0) {
				this->TableEdit_Abort(false);
			}  else {
				ret = this->TableEdit_Start((size_t)index, 0);
			}
			return (LRESULT)ret;
		}
		LRESULT OnGetStringWidth(UINT, WPARAM, LPARAM lp) {
			auto str = (const wchar_t*)lp;
			return this->GetOptimalColumnWidthFixed(pfc::utf8FromWide(str), false /* no pad */);
		}
		LRESULT OnEnableGroupView(UINT, WPARAM wp, LPARAM) {
			bool bEnable = (wp != 0);
			if (bEnable == m_groupViewEnabled) return 0;
			m_groupViewEnabled = bEnable;
			ReloadData();
			return 1;
		}
		LRESULT OnScroll(UINT, WPARAM, LPARAM) {
			PFC_ASSERT(!"Implement me");
			return 0;
		}
		LRESULT OnRedrawItems(UINT, WPARAM wp, LPARAM lp) {
			size_t base = wp;
			size_t last = lp;
			if (last < base) return FALSE;
			size_t count = last + 1 - base;
			this->UpdateItems(pfc::bit_array_range(base, count));
			return TRUE;
		}
		bool TableEdit_CanAdvanceHere(size_t item, size_t subItem, uint32_t whatHappened) const override {
			// Block line cycling with enter key
			auto code = (whatHappened & InPlaceEdit::KEditMaskReason);
			if (code == InPlaceEdit::KEditEnter) return false;
			return __super::TableEdit_CanAdvanceHere(item, subItem, whatHappened);
		}
		bool TableEdit_IsColumnEditable(t_size subItem) const override {
			return subItem == 0;
		}
		NMHDR setupHdr(UINT code) const {
			return { m_hWnd, (UINT_PTR)this->GetDlgCtrlID(), code };
		}
		LRESULT sendNotify(void* data) const {
			auto hdr = reinterpret_cast<const NMHDR*>(data);
			PFC_ASSERT(hdr->idFrom == (UINT_PTR) GetDlgCtrlID());
			return GetParent().SendMessage(WM_NOTIFY, hdr->idFrom, reinterpret_cast<LPARAM>(data));
		}
		int getDispInfoImage(size_t item, size_t subItem) const {
			NMLVDISPINFO info = { setupHdr(LVN_GETDISPINFO) };
			info.item.mask = LVIF_IMAGE;
			info.item.iItem = (int)item;
			info.item.iSubItem = (int)subItem;
			sendNotify(&info);
			PFC_ASSERT(info.item.iImage != I_IMAGECALLBACK);
			return info.item.iImage;
		}
		pfc::string8 getDispInfoText(size_t item, size_t subItem) const {
			NMLVDISPINFO info = { setupHdr(LVN_GETDISPINFO) };
			info.item.mask = LVIF_TEXT;
			info.item.iItem = (int)item;
			info.item.iSubItem = (int)subItem;

			wchar_t buffer[1024] = {};
			info.item.pszText = buffer;
			info.item.cchTextMax = (int) std::size(buffer);
			sendNotify(&info);
			if (info.item.pszText == LPSTR_TEXTCALLBACK || info.item.pszText == nullptr) {
				PFC_ASSERT(!"WTF??"); return "";
			}
			return pfc::utf8FromWide(info.item.pszText);
		}
		virtual LPARAM GetItemParam(size_t) { return 0; }
		void ExecuteDefaultAction(size_t idx) override {
			NMITEMACTIVATE info = { setupHdr(NM_DBLCLK) };
			info.iItem = (int) idx;
			info.lParam = GetItemParam(idx);
			sendNotify(&info);
		}
		void OnSubItemClicked(t_size item, t_size subItem, CPoint pt) override {
			__super::OnSubItemClicked(item, subItem, pt);
			NMITEMACTIVATE info = { setupHdr(NM_CLICK) };
			info.iItem = (int)item;
			info.iSubItem = (int)subItem;
			info.ptAction = pt;
			sendNotify(&info);
		}
		void OnFocusChanged(size_t oldFocus, size_t newFocus) override {
			__super::OnFocusChanged(oldFocus, newFocus);
			const auto count = this->GetItemCount();

			if (oldFocus < count) {
				auto idx = oldFocus;
				NMLISTVIEW info = { setupHdr(LVN_ITEMCHANGED) };
				info.iItem = (int)idx;
				info.lParam = this->GetItemParam(idx);
				auto base = this->GetItemState(idx);
				info.uOldState = base | LVIS_FOCUSED;
				info.uNewState = base;
				info.uChanged = LVIF_STATE;
				this->sendNotify(&info);
			}
			if (newFocus < count) {
				auto idx = newFocus;
				NMLISTVIEW info = { setupHdr(LVN_ITEMCHANGED) };
				info.iItem = (int)idx;
				info.lParam = this->GetItemParam(idx);
				auto base = this->GetItemState(idx);
				info.uOldState = base & ~LVIS_FOCUSED;
				info.uNewState = base;
				info.uChanged = LVIF_STATE;
				this->sendNotify(&info);
			}
		}

		void OnSelectionChanged(pfc::bit_array const& affected, pfc::bit_array const& status) override {

			__super::OnSelectionChanged(affected, status);
			const auto count = this->GetItemCount();

			const size_t focus = this->GetFocusItem();

			// LVN_ITEMCHANGING not supported, CListControl currently doesn't hand this info

			affected.for_each(true, 0, count, [&](size_t idx) {
				bool sel_new = status[idx];
				bool sel_old = !sel_new;
				NMLISTVIEW info = { setupHdr(LVN_ITEMCHANGED) };
				info.iItem = (int)idx;
				info.lParam = this->GetItemParam(idx);
				info.uOldState = (sel_old ? LVIS_SELECTED : 0) | (idx == focus ? LVIS_FOCUSED : 0);
				info.uNewState = (sel_new ? LVIS_SELECTED : 0) | (idx == focus ? LVIS_FOCUSED : 0);
				info.uChanged = LVIF_STATE;
				this->sendNotify(&info);
			});
		}
		void RequestReorder(size_t const* order, size_t count) override {}
		void RequestRemoveSelection() override {}

		void OnColumnHeaderClick(t_size index) override {
			NMLISTVIEW info = { setupHdr(LVN_COLUMNCLICK) };
			info.iItem = -1;
			info.iSubItem = (int)index;
			this->sendNotify(&info);
		}

		bool IsHeaderless() const {
			return (m_style & LVS_NOCOLUMNHEADER) != 0;
		}

		std::vector< uint32_t > m_headerlessColumns;

		size_t GetColumnCount() const override {
			if (IsHeaderless()) {
				return m_headerlessColumns.size();
			}
			return __super::GetColumnCount();
		}
		uint32_t GetSubItemWidth(size_t subItem) const override {
			if (IsHeaderless()) {
				if (subItem < m_headerlessColumns.size()) {
					auto v = m_headerlessColumns[subItem];
					if (v == UINT32_MAX) {
						uint32_t nAuto = 0, wNormal = 0;
						for (auto walk : m_headerlessColumns) {
							if (walk == UINT32_MAX) ++nAuto;
							else wNormal += walk;
						}
						PFC_ASSERT(nAuto > 0);
						uint32_t wTotal = this->GetClientRectHook().Width();
						if (wTotal > wNormal) {
							uint32_t autoTotal = wTotal - wNormal;
							v = autoTotal / nAuto;
						} else {
							v = 0;
						}
					}
					return v;
				}
				else return 0;
			}
			return __super::GetSubItemWidth(subItem);
		}

		bool GetCellTypeSupported() const override { 
			return useCheckBoxes();
		}

		cellType_t GetCellType(size_t item, size_t subItem) const override {
			if (useCheckBoxes() && subItem == 0) {
				return &PFC_SINGLETON(CListCell_Checkbox);
			}
			return __super::GetCellType(item, subItem);
		}

		bool useCheckBoxes() const {
			return (m_listViewExStyle & LVS_EX_CHECKBOXES) != 0;
		}

		void TableEdit_SetField(t_size item, t_size subItem, const char* value) override {
			if (subItem != 0) {
				PFC_ASSERT(!"subItem should be zero");
				return;
			}
			auto textW = pfc::wideFromUTF8(value);
			NMLVDISPINFO info = { setupHdr(LVN_ENDLABELEDIT) };
			info.item.iItem = (int)item;
			info.item.mask = LVIF_TEXT;
			info.item.pszText = const_cast<wchar_t*>( textW.c_str() );

			if (sendNotify(&info) != 0) {
				SetSubItemText(item, subItem, value);
			}
		}

		virtual void SetSubItemText(size_t item, size_t subItem, const char* text) {}

		virtual int GetItemImage(size_t item, size_t subItem) const {
			return I_IMAGEREALLYNONE;
		}
		CImageList GetImageList() const {
			return m_imageLists[LVSIL_SMALL];
		}
		bool RenderCellImageTest(size_t item, size_t subItem) const override { 
			return GetImageList() != NULL && GetItemImage(item, subItem) != I_IMAGEREALLYNONE;
		}
		void RenderCellImage(size_t item, size_t subItem, CDCHandle dc, const CRect& rc) const override {
			auto img = GetItemImage(item, subItem);
			auto imgList = GetImageList();
			if (img >= 0 && imgList) {
				CSize size;
				WIN32_OP_D(imgList.GetIconSize(size));
				CRect rc2 = rc;
				if (size.cx <= rc.Width() && size.cy <= rc.Height()) {
					auto cp = rc.CenterPoint();
					rc2.left = cp.x - size.cx / 2;
					rc2.top = cp.y - size.cy / 2;
					rc2.right = rc2.left + size.cx;
					rc2.bottom = rc2.top + size.cy;
				}
				imgList.DrawEx(img, dc, rc2, CLR_NONE, CLR_NONE, ILD_SCALE);
			}
		}

		bool m_notifyItemDraw = false;

		UINT GetItemCDState(size_t which) const {
			UINT ret = 0;
			DWORD state = GetItemState(which);
			if (state & LVIS_FOCUSED) ret |= CDIS_FOCUS;
			if (state & LVIS_SELECTED) ret |= CDIS_SELECTED;
			return ret;
		}

		void RenderRect(const CRect& p_rect, CDCHandle p_dc) override {
			NMCUSTOMDRAW cd = { setupHdr(NM_CUSTOMDRAW) };
			cd.dwDrawStage = CDDS_PREPAINT;
			cd.hdc = p_dc;
			cd.rc = p_rect;
			cd.dwItemSpec = UINT32_MAX;
			cd.uItemState = 0;
			cd.lItemlParam = 0;
			LRESULT status = sendNotify(&cd);
			m_notifyItemDraw = (status & CDRF_NOTIFYITEMDRAW) != 0;

			if ((status & CDRF_SKIPDEFAULT) != 0) {
				return;
			}
			__super::RenderRect(p_rect, p_dc);

			cd.dwDrawStage = CDDS_POSTPAINT;
			sendNotify(&cd);
		}
		void RenderItem(t_size item, const CRect& itemRect, const CRect& updateRect, CDCHandle dc) override {
			NMCUSTOMDRAW cd = {};
			if (m_notifyItemDraw) {
				cd = { setupHdr(NM_CUSTOMDRAW) };
				cd.dwDrawStage = CDDS_ITEMPREPAINT;
				cd.hdc = dc;
				cd.rc = itemRect;
				cd.dwItemSpec = (DWORD)item;
				cd.uItemState = GetItemCDState(item);
				cd.lItemlParam = GetItemParam(item);
				LRESULT status = sendNotify(&cd);
				if (status & CDRF_SKIPDEFAULT) return;
			}
			
			__super::RenderItem(item, itemRect, updateRect, dc);


			if (m_notifyItemDraw) {
				cd.dwDrawStage = CDDS_ITEMPOSTPAINT;
				sendNotify(&cd);
			}
		}
#if 0
		void RenderSubItemText(t_size item, t_size subItem, const CRect& subItemRect, const CRect& updateRect, CDCHandle dc, bool allowColors) override {

			__super::RenderSubItemText(item, subItem, subItemRect, updateRect, dc, allowColors);
		}
#endif

	};

	class CListControl_ListViewOwnerData : public CListControl_ListViewBase {
	public:
		CListControl_ListViewOwnerData(DWORD style) : CListControl_ListViewBase(style) {}

		BEGIN_MSG_MAP_EX(CListControl_ListViewOwnerData)
			MESSAGE_HANDLER_EX(LVM_SETITEMCOUNT, OnSetItemCount)
			CHAIN_MSG_MAP(CListControl_ListViewBase)
		END_MSG_MAP()
	private:
		size_t m_itemCount = 0;
		LRESULT OnSetItemCount(UINT, WPARAM wp, LPARAM) {
			auto count = (size_t)wp;
			if (m_itemCount != count) {
				m_itemCount = count; ReloadData();
			}
			return 0;
		}

		t_size GetItemCount() const override {
			return m_itemCount;
		}
		bool GetSubItemText(t_size item, t_size subItem, pfc::string_base& out) const override {
			if (item < m_itemCount) {
				out = this->getDispInfoText(item, subItem);
				return true;
			}
			return false;
		}
		int GetItemImage(size_t item, size_t subItem) const override {
			int ret = I_IMAGEREALLYNONE;
			if (subItem == 0) {
				ret = this->getDispInfoImage(item, subItem);
			}
			return ret;
		}
		void SetSubItemText(size_t item, size_t subItem, const char* text) override {
			auto textW = pfc::wideFromUTF8(text);
			NMLVDISPINFO info = { setupHdr(LVN_SETDISPINFO) };
			info.item.iItem = (int)item;
			info.item.iSubItem = (int)subItem;
			info.item.mask = LVIF_TEXT;
			info.item.pszText = const_cast<wchar_t*>(textW.c_str());
			sendNotify(&info);
			ReloadItem(item);
		}
	};

	class CListControl_ListView : public CListControl_ListViewBase {
	public:
		CListControl_ListView(DWORD style) : CListControl_ListViewBase(style) {}

		BEGIN_MSG_MAP_EX(CListControl_ListView)
			MESSAGE_HANDLER_EX(LVM_INSERTITEM, OnInsertItem)
			MESSAGE_HANDLER_EX(LVM_SETITEM, OnSetItem)
			MESSAGE_HANDLER_EX(LVM_SETITEMCOUNT, OnSetItemCount)
			MESSAGE_HANDLER_EX(LVM_GETITEM, OnGetItem)
			MESSAGE_HANDLER_EX(LVM_GETITEMTEXT, OnGetItemText)
			MESSAGE_HANDLER_EX(LVM_INSERTGROUP, OnInsertGroup)
			MESSAGE_HANDLER_EX(LVM_REMOVEGROUP, OnRemoveGroup)
			MESSAGE_HANDLER_EX(LVM_GETGROUPCOUNT, OnGetGroupCount)
			MESSAGE_HANDLER_EX(LVM_GETGROUPINFO, OnGetGroupInfo)
			MESSAGE_HANDLER_EX(LVM_SETGROUPINFO, OnSetGroupInfo)
			MESSAGE_HANDLER_EX(LVM_GETGROUPINFOBYINDEX, OnGetGroupInfoByIndex)
			MESSAGE_HANDLER_EX(LVM_REMOVEALLGROUPS, OnRemoveAllGroups)
			MESSAGE_HANDLER_EX(LVM_DELETEITEM, OnDeleteItem)
			MESSAGE_HANDLER_EX(LVM_DELETEALLITEMS, OnDeleteAllItems)
			CHAIN_MSG_MAP(CListControl_ListViewBase)
		END_MSG_MAP()
	private:
		struct text_t {
			pfc::string8 text;
			bool callback = false;
		};
		struct rec_t {
			std::vector< text_t > text;
			LPARAM param = 0;
			int image = I_IMAGEREALLYNONE;
			bool checked = false;
			groupID_t groupID = 0;
			int LVGroupID = 0;
		};

		std::vector<rec_t> m_content;

		groupID_t m_groupIDGen = 0;
		groupID_t groupIDGen() { return ++m_groupIDGen; }
		struct group_t {
			int LVGroupID = 0;
			groupID_t GroupID = 0;
			pfc::string8 header;
		};
		std::vector<group_t> m_groups;

		groupID_t groupFromLV(int LV) const {
			for (auto& g : m_groups) {
				if (g.LVGroupID == LV) return g.GroupID;
			}
			return 0;
		}

		t_size GetItemCount() const override {
			return m_content.size();
		}
		bool GetSubItemText(t_size item, t_size subItem, pfc::string_base& out) const override {
			if (item < m_content.size()) {
				auto& r = m_content[item];
				if (subItem < r.text.size()) {
					auto& t = r.text[subItem];
					if (t.callback) {
						out = getDispInfoText(item, subItem);
					} else {
						out = t.text;
					}
					return true;
				}
			}
			return false;
		}
		void SetSubItemText(size_t item, size_t subItem, const char* text) override {
			if (item < m_content.size()) {
				auto& r = m_content[item];
				if (subItem < r.text.size()) {
					auto& t = r.text[subItem];
					t.callback = false; t.text = text;
					ReloadItem(item);
				}
			}
		}

		int GetItemImage(size_t item, size_t subItem) const override {
			int ret = I_IMAGEREALLYNONE;
			if (subItem == 0 && item < m_content.size()) {
				auto& r = m_content[item];
				ret = r.image;
				if (ret == I_IMAGECALLBACK) ret = this->getDispInfoImage(item, subItem);
			}
			return ret;
		}
		LPARAM GetItemParam(size_t item) override {
			LPARAM ret = 0;
			if (item < m_content.size()) {
				ret = m_content[item].param;
			}
			return ret;
		}
		
		static text_t makeText(const wchar_t* p) {
			text_t text;
			if (p == LPSTR_TEXTCALLBACK) {
				text.callback = true;
			} else {
				text.text = pfc::utf8FromWide(p);
			}
			return text;
		}
		LRESULT OnSetItem(UINT, WPARAM, LPARAM lp) {
			auto pItem = reinterpret_cast<LVITEM*>(lp);
			size_t item = (size_t)pItem->iItem;
			const size_t total = GetItemCount();
			if (item > total) return FALSE;
			size_t subItem = (size_t)pItem->iSubItem;
			if (subItem > 1024) return FALSE; // quick sanity
			auto& rec = m_content[item];
			size_t width = subItem + 1;
			if (rec.text.size() < width) rec.text.resize(width);
			bool bReload = false, bReloadAll = false;
			if (pItem->mask & LVIF_TEXT) {
				rec.text[subItem] = makeText(pItem->pszText);
				bReload = true;
			}
			if (pItem->mask & LVIF_IMAGE) {
				rec.image = pItem->iImage;
				bReload = true;
			}
			if (pItem->mask & LVIF_PARAM) {
				rec.param = pItem->lParam;
			}
			if (pItem->mask & LVIF_GROUPID) {
				rec.LVGroupID = pItem->iGroupId;
				rec.groupID = groupFromLV(rec.LVGroupID);
				if (m_groupViewEnabled) bReloadAll = true;
			}
			if (bReloadAll) {
				this->ReloadData();
			} else if (bReload) {
				this->ReloadItem(item);
			}
			if (pItem->mask & LVIF_STATE) {
				this->SetItemState(item, pItem->stateMask, pItem->state);
			}
			return TRUE;
		}
		LRESULT OnInsertItem(UINT, WPARAM, LPARAM lp) {
			auto pItem = reinterpret_cast<LVITEM*>(lp);

			size_t item = (size_t)pItem->iItem;
			const size_t total = GetItemCount();
			if (item > total) item = total;

			rec_t r;
			if (pItem->mask & LVIF_TEXT) {
				r.text = { makeText(pItem->pszText) };
			}
			if (pItem->mask & LVIF_GROUPID) {
				r.LVGroupID = pItem->iGroupId;
				r.groupID = groupFromLV(r.LVGroupID);
			}
			if (pItem->mask & LVIF_IMAGE) {
				r.image = pItem->iImage;
			}
			if (pItem->mask & LVIF_PARAM) {
				r.param = pItem->lParam;
			}

			m_content.insert(m_content.begin() + item, std::move(r));
			if (m_groupViewEnabled) ReloadData();
			else this->OnItemsInserted(item, 1, false);
			
			return (LRESULT)item;
		}
		LRESULT OnSetItemCount(UINT, WPARAM wp, LPARAM) {
			// It's not actually supposed to be called like this, LVM_SETITEMCOUNT is meant for ownerdata listviews
			auto count = (size_t)wp;
			if (count != m_content.size()) {
				m_content.resize(wp);
				ReloadData();
			}
			return 0;
		}

		LRESULT OnInsertGroup(UINT, WPARAM wp, LPARAM lp) {
			LVGROUP* arg = (LVGROUP*)lp;
			if ((arg->mask & (LVGF_GROUPID | LVGF_HEADER)) != (LVGF_GROUPID | LVGF_HEADER)) return -1;

			auto LVGroupID = arg->iGroupId;
			for (auto& existing : m_groups) {
				if (existing.LVGroupID == LVGroupID) return -1;
			}

			size_t idx = (size_t)wp;
			if (idx > m_groups.size()) idx = m_groups.size();

			group_t grp;
			grp.LVGroupID = LVGroupID;
			grp.GroupID = this->groupIDGen();
			grp.header = pfc::utf8FromWide(arg->pszHeader);
			m_groups.insert(m_groups.begin() + idx, std::move(grp));
			// No reload data - adding of items does it
			return (LRESULT)idx;
		}
		LRESULT OnRemoveGroup(UINT, WPARAM wp, LPARAM) {
			int LVGroupID = (int)wp;
			for (size_t walk = 0; walk < m_groups.size(); ++walk) {
				if (m_groups[walk].LVGroupID == LVGroupID) {
					m_groups.erase(m_groups.begin() + walk);
					return walk;
				}
			}
			return -1;
		}
		void getGroupInfo(group_t const& in, LVGROUP* out) {
			if (out->mask & LVGF_HEADER) {
				auto text = pfc::wideFromUTF8(in.header.c_str());
				safeFillText(out->pszHeader, out->cchHeader, text);
			}
			if (out->mask & LVGF_GROUPID) {
				out->iGroupId = in.LVGroupID;
			}
		}
		void setGroupInfo(group_t& grp, LVGROUP* in) {
			bool bReload = false;
			if (in->mask & LVGF_HEADER) {
				grp.header = pfc::utf8FromWide(in->pszHeader);
				bReload = true;
			}
			if (in->mask & LVGF_GROUPID) {
				grp.LVGroupID = in->iGroupId;
				bReload = true;
			}

			if (bReload) this->ReloadData();
		}
		LRESULT OnGetGroupCount(UINT, WPARAM, LPARAM) {
			return (LRESULT)m_groups.size();
		}
		LRESULT OnGetGroupInfo(UINT, WPARAM wp, LPARAM lp) {
			int LVGroupID = (int)wp;
			auto out = (LVGROUP*)lp;
			for (auto& grp : m_groups) {
				if (grp.LVGroupID == LVGroupID) {
					getGroupInfo(grp, out);
					return LVGroupID;
				}
			}
			return -1;
		}
		LRESULT OnSetGroupInfo(UINT, WPARAM wp, LPARAM lp) {
			int LVGroupID = (int)wp;
			auto in = (LVGROUP*)lp;
			int ret = -1;
			for (auto& grp : m_groups) {
				if (grp.LVGroupID == LVGroupID) {
					setGroupInfo(grp, in);
					ret = grp.LVGroupID;
				}
			}
			return ret;
		}
		LRESULT OnGetGroupInfoByIndex(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			LVGROUP* out = (LVGROUP*)lp;
			if (idx < m_groups.size()) {
				getGroupInfo(m_groups[idx], out);
			}
			return FALSE;
		}
		LRESULT OnRemoveAllGroups(UINT, WPARAM, LPARAM) {
			m_groups.clear(); return 0;
		}
		LRESULT OnDeleteItem(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t)wp;
			LRESULT rv = FALSE;
			const size_t oldCount = m_content.size();
			if (idx < oldCount) {
				m_content.erase(m_content.begin() + idx);
				this->OnItemsRemoved(pfc::bit_array_one(idx), oldCount);
				rv = TRUE;
			}
			return rv;
		}
		LRESULT OnDeleteAllItems(UINT, WPARAM, LPARAM) {
			m_content.clear(); ReloadData();
			return TRUE;
		}
		int fillText(size_t item, size_t subItem, LVITEM* pItem) const {
			pfc::wstringLite text;
			if (item < m_content.size()) {
				auto& r = m_content[item];
				if (subItem < r.text.size()) {
					auto& t = r.text[subItem];
					if (!t.callback) {
						text = pfc::wideFromUTF8(t.text);
					}
				}
			}
			return safeFillText(pItem->pszText, pItem->cchTextMax, text);
		}
		LRESULT OnGetItem(UINT, WPARAM, LPARAM lp) {
			auto pItem = reinterpret_cast<LVITEM*>(lp);
			if (pItem->mask & LVIF_TEXT) {
				fillText(pItem->iItem, pItem->iSubItem, pItem);
			}
			if (pItem->mask & LVIF_IMAGE) {
				size_t idx = pItem->iItem;
				if (idx < m_content.size()) {
					auto& line = m_content[idx];
					pItem->iImage = line.image;
				}
			}
			if (pItem->mask & LVIF_PARAM) {
				pItem->lParam = GetItemParam(pItem->iItem);
			}
			
			return TRUE;
		}
		LRESULT OnGetItemText(UINT, WPARAM wp, LPARAM lp) {
			auto item = (size_t)wp;
			auto pItem = reinterpret_cast<LVITEM*>(lp);
			size_t subItem = (size_t)pItem->iSubItem;
			return fillText(item, subItem, pItem);
		}

		bool GetCellCheckState(size_t item, size_t sub) const override {
			bool rv = false;
			if (sub == 0 && item < m_content.size()) {
				rv = m_content[item].checked;
			}
			return rv;
		}
		void SetCellCheckState(size_t item, size_t sub, bool value) override {
			if (sub == 0 && item < m_content.size()) {
				auto& rec = m_content[item];
				if (rec.checked != value) {
					auto oldState = this->GetItemState(item);
					rec.checked = value;
					__super::SetCellCheckState(item, sub, value);

					NMLISTVIEW info = { this->setupHdr(LVN_ITEMCHANGED) };
					info.iItem = (int)item;
					info.lParam = this->GetItemParam(item);
					info.uChanged = LVIF_STATE;
					info.uOldState = oldState;
					info.uNewState = this->GetItemState(item);
					this->sendNotify(&info);
				}
			}
		}
		UINT GetItemState(size_t idx) const override {
			UINT ret = __super::GetItemState(idx);
			if (useCheckBoxes() && idx < m_content.size()) {
				int nCheck = m_content[idx].checked ? 2 : 1;
				ret |= INDEXTOSTATEIMAGEMASK(nCheck);
			}
			return ret;
		}
		void SetItemState(size_t idx, DWORD mask, DWORD state) override {
			__super::SetItemState(idx, mask, state);
			if (useCheckBoxes() && (mask & LVIS_STATEIMAGEMASK) != 0) {
				int nCheck = ((state & LVIS_STATEIMAGEMASK) >> 12);
				this->SetCellCheckState(idx, 0, nCheck == 2);
			}
		}
		groupID_t GetItemGroup(t_size p_item) const override {
			if (m_groupViewEnabled && p_item < m_content.size()) {
				return m_content[p_item].groupID;
			}
			return 0;
		}
		bool GetGroupHeaderText2(size_t baseItem, pfc::string_base& out) const override {
			auto id = GetItemGroup(baseItem);
			if (id != 0) {
				for (auto& g : m_groups) {
					if (id == g.GroupID) {
						out = g.header;
						return true;
					}
				}
			}
			return false;
		}
	};
}

HWND CListControl_ReplaceListView(HWND wndReplace) {
	CListViewCtrl src(wndReplace);
	const auto style = src.GetStyle();
	HWND ret = NULL;
	if (style & LVS_REPORT) {
		const auto ctrlID = src.GetDlgCtrlID();
		CWindow parent = src.GetParent();
		DWORD headerStyle = 0;
		if ((style & LVS_NOCOLUMNHEADER) == 0) {
			auto header = src.GetHeader(); 
			if (header) { 
				headerStyle = header.GetStyle(); 
			}
		}
		if (style & LVS_OWNERDATA) {
			auto obj = PP::newWindowObj<CListControl_ListViewOwnerData>(style);
			ret = obj->CreateInDialog(parent, ctrlID, src);
			PFC_ASSERT(ret != NULL);
			if (headerStyle != 0 && obj->GetHeaderCtrl() == NULL) {
				obj->InitializeHeaderCtrl((headerStyle&(HDS_FULLDRAG | HDS_BUTTONS)));
			}
		} else {
			PFC_ASSERT(src.GetItemCount() == 0); // transferring of items not yet implemented
			auto obj = PP::newWindowObj<CListControl_ListView>(style);
			ret = obj->CreateInDialog(parent, ctrlID, src);
			PFC_ASSERT(ret != NULL);
			if (headerStyle != 0 && obj->GetHeaderCtrl() == NULL) {
				obj->InitializeHeaderCtrl((headerStyle & (HDS_FULLDRAG | HDS_BUTTONS)));
			}
		}
	}
	return ret;
}

namespace {
	// FIX ME WM_DELETEITEM

	class CListControl_ListBoxBase : public CListControlReadOnly {
	protected:
		const DWORD m_style;
	public:
		CListControl_ListBoxBase(DWORD style) : m_style(style) {

			// owner data not implemented
			PFC_ASSERT((m_style & LBS_NODATA) == 0);

			if (m_style & LBS_NOSEL) {
				this->SetSelectionModeNone();
			} else if (m_style & LBS_MULTIPLESEL) {
				this->SetSelectionModeMulti();
			} else {
				this->SetSelectionModeSingle();
			}
		}

		BEGIN_MSG_MAP_EX(CListControl_ListBoxBase)
			MSG_WM_SETFOCUS(OnSetFocus)
			MSG_WM_KILLFOCUS(OnKillFocus)
			MESSAGE_HANDLER_EX(LB_SETCURSEL, OnSetCurSel)
			MESSAGE_HANDLER_EX(LB_GETCURSEL, OnGetCurSel)
			MESSAGE_HANDLER_EX(LB_GETITEMRECT, OnGetItemRect)
			MESSAGE_HANDLER_EX(LB_SELECTSTRING, OnSelectString)
			MESSAGE_HANDLER_EX(LB_ITEMFROMPOINT, OnItemFromPoint)
			MESSAGE_HANDLER_EX(LB_GETSEL, OnGetSel)
			MESSAGE_HANDLER_EX(LB_SETSEL, OnSetSel)
			MESSAGE_HANDLER_EX(LB_GETSELCOUNT, OnGetSelCount)
			MESSAGE_HANDLER_EX(LB_GETSELITEMS, OnGetSelItems)
			MESSAGE_HANDLER_EX(LB_SETCARETINDEX, OnSetCaretIndex)
			MESSAGE_HANDLER_EX(LB_GETCARETINDEX, OnGetCaretIndex)
			MSG_WM_CREATE(OnCreate)
			CHAIN_MSG_MAP(CListControlReadOnly)
		END_MSG_MAP()

		LRESULT OnCreate(LPCREATESTRUCT) {
			SetMsgHandled(FALSE);
			// Adopt style flags of the original control to keep various ATL checks happy
			DWORD style = GetStyle();
			style = (style & 0xFFFF0000) | (m_style & 0xFFFF);
			SetWindowLong(GWL_STYLE, style);
			return 0;
		}
		void notifyParent(WORD code) {
			if (m_style & LBS_NOTIFY) {
				GetParent().PostMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), code), (LPARAM)m_hWnd);
			}
		}
		LRESULT OnGetCurSel(UINT, WPARAM, LPARAM) {
			return (LRESULT)this->GetSingleSel();
		}
		LRESULT OnSetCurSel(UINT, WPARAM wp, LPARAM) {
			this->SelectSingle((size_t)wp);
			return LB_OKAY;
		}
		LRESULT OnGetItemRect(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx < this->GetItemCount()) {
				*reinterpret_cast<RECT*>(lp) = this->GetItemRect(idx);
				return LB_OKAY;
			} else {
				return LB_ERR;
			}
		}
		LRESULT OnSelectString(UINT, WPARAM wp, LPARAM lp) {
			auto idx = this->SendMessage(LB_FINDSTRING, wp, lp);
			if (idx != LB_ERR) this->SelectSingle((size_t)idx);
			return idx;
		}
		LRESULT OnItemFromPoint(UINT, WPARAM, LPARAM lp) {
			CPoint pt(lp);
			size_t ret;
			if (!this->ItemFromPoint(pt, ret)) return LB_ERR;
			return (LRESULT)ret;
		}
		LRESULT OnGetSel(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t)wp;
			if (idx < this->GetItemCount()) {
				return this->IsItemSelected(idx) ? 1 : 0;
			} else {
				return LB_ERR;
			}

		}
		LRESULT OnSetSel(UINT, WPARAM wp, LPARAM lp) {
			// Contrary to the other methods, index comes in LPARAM
			if (lp < 0) {
				if (wp) this->SelectAll();
				else this->SelectNone();
			} else {
				this->SetSelection(pfc::bit_array_one((size_t)lp), pfc::bit_array_val(wp != 0));
			}
			return LB_OKAY;
		}
		LRESULT OnGetSelItems(UINT, WPARAM wp, LPARAM lp) {
			int* out = reinterpret_cast<int*>(lp);
			size_t outSize = (size_t)wp;
			size_t outWalk = 0;
			const size_t total = this->GetItemCount();
			for (size_t walk = 0; walk < total && outWalk < outSize; ++walk) {
				if (this->IsItemSelected(walk)) {
					out[outWalk++] = (int)walk;
				}
			}
			return outWalk;
		}
		LRESULT OnGetSelCount(UINT, WPARAM, LPARAM) {
			return (LRESULT)this->GetSelectedCount();
		}
		LRESULT OnGetCaretIndex(UINT, WPARAM, LPARAM) {
			size_t focus = this->GetFocusItem();
			if (focus == SIZE_MAX) return LB_ERR;
			return (LRESULT)focus;
		}
		LRESULT OnSetCaretIndex(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t)wp;
			if (idx < this->GetItemCount()) {
				this->SetFocusItem(idx); return LB_OKAY;
			} else {
				return LB_ERR;
			}
		}

		void OnSetFocus(HWND) {
			notifyParent(LBN_SETFOCUS);
			SetMsgHandled(FALSE);
		}
		void OnKillFocus(HWND) {
			notifyParent(LBN_KILLFOCUS);
			SetMsgHandled(FALSE);
		}

		void OnSelectionChanged(pfc::bit_array const& affected, pfc::bit_array const& status) override {
			__super::OnSelectionChanged(affected, status);
			notifyParent(LBN_SELCHANGE);
		}
		void ExecuteDefaultAction(size_t idx) override {
			notifyParent(LBN_DBLCLK);
		}

		void RequestReorder(size_t const* order, size_t count) override {}
		void RequestRemoveSelection() override {}
	};
	class CListControl_ListBox : public CListControl_ListBoxBase {
	public:
		CListControl_ListBox(DWORD style) : CListControl_ListBoxBase(style) {}

		BEGIN_MSG_MAP_EX(CListControl_ListBox)
			MESSAGE_HANDLER_EX(LB_INSERTSTRING, OnInsertString)
			MESSAGE_HANDLER_EX(LB_ADDSTRING, OnAddString)
			MESSAGE_HANDLER_EX(LB_RESETCONTENT, OnResetContent)
			MESSAGE_HANDLER_EX(LB_DELETESTRING, OnDeleteString)
			MESSAGE_HANDLER_EX(LB_SETITEMDATA, OnSetItemData)
			MESSAGE_HANDLER_EX(LB_GETITEMDATA, OnGetItemData)
			MESSAGE_HANDLER_EX(LB_GETCOUNT, OnGetCount)
			MESSAGE_HANDLER_EX(LB_FINDSTRING, OnFindString)
			MESSAGE_HANDLER_EX(LB_FINDSTRINGEXACT, OnFindStringExact)
			MESSAGE_HANDLER_EX(LB_SETCOUNT, OnSetCount)
			MESSAGE_HANDLER_EX(LB_INITSTORAGE, OnInitStorage)
			MESSAGE_HANDLER_EX(LB_GETTEXT, OnGetText)
			MESSAGE_HANDLER_EX(LB_GETTEXTLEN, OnGetTextLen)
			CHAIN_MSG_MAP(CListControl_ListBoxBase)
		END_MSG_MAP()

		struct rec_t {
			pfc::string8 text;
			LPARAM data = 0;
		};

		std::vector<rec_t> m_content;
		t_size GetItemCount() const override {
			return m_content.size();
		}

		bool isForceSorted() const {
			return (m_style & LBS_SORT) != 0;
		}
		size_t AddStringSorted(pfc::string8 && str) {
			size_t ret = 0;
			// FIX ME bsearch??
			// even with bsearch it's still O(n) so it's pointless
			while (ret < m_content.size() && pfc::stricmp_ascii(str, m_content[ret].text) > 0) ++ret;

			m_content.insert(m_content.begin() + ret, { std::move(str) });

			this->OnItemsInserted(ret, 1, false);
			return ret;
		}
		static pfc::string8 importString(LPARAM lp) {
			return pfc::utf8FromWide(reinterpret_cast<const wchar_t*>(lp));
		}
		LRESULT OnInsertString(UINT, WPARAM wp, LPARAM lp) {
			auto str = importString(lp);
			if (isForceSorted()) return AddStringSorted(std::move(str));
			size_t at = wp;
			if (at > m_content.size()) at = m_content.size();
			m_content.insert(m_content.begin() + at, { std::move(str) });
			this->OnItemsInserted(at, 1, false);
			return at;
		}
		LRESULT OnAddString(UINT, WPARAM wp, LPARAM lp) {
			auto str = importString(lp);
			if (isForceSorted()) return AddStringSorted(std::move(str));
			size_t ret = m_content.size();
			m_content.push_back({ std::move(str) });
			this->OnItemsInserted(ret, 1, false);
			return ret;
		}
		LRESULT OnResetContent(UINT, WPARAM, LPARAM) {
			size_t oldCount = m_content.size();
			if (oldCount > 0) {
				m_content.clear();
				this->OnItemsRemoved(pfc::bit_array_true(), oldCount);
			}
			return LB_OKAY;
		}
		LRESULT OnDeleteString(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t) wp;
			if (idx < m_content.size()) {
				m_content.erase(m_content.begin() + idx);
				this->OnItemRemoved(idx);
				return m_content.size();
			} else {
				return LB_ERR;
			}
		}

		LRESULT OnSetItemData(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx < m_content.size()) {
				m_content[idx].data = (size_t)lp;
				return LB_OKAY;
			} else {
				return LB_ERR;
			}
		}
		LRESULT OnGetItemData(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx < m_content.size()) {
				return (LRESULT)m_content[idx].data;
			} else {
				return LB_ERR;
			}
		}
		LRESULT OnGetCount(UINT, WPARAM, LPARAM) {
			return (LRESULT)m_content.size();
		}
		LRESULT OnFindString(UINT, WPARAM wp, LPARAM lp) {
			auto str = importString(lp);
			const auto total = m_content.size();
			size_t first = (size_t)(wp + 1) % total;
			for (size_t walk = 0; walk < total; ++walk) {
				size_t at = (first + walk) % total;
				if (pfc::string_has_prefix_i(m_content[at].text, str)) return (LRESULT)at;
			}
			return LB_ERR;
		}
		LRESULT OnFindStringExact(UINT, WPARAM wp, LPARAM lp) {
			auto str = importString(lp);
			const auto total = m_content.size();
			size_t first = (size_t)(wp + 1) % total;
			for (size_t walk = 0; walk < total; ++walk) {
				size_t at = (first + walk) % total;
				if (pfc::stringEqualsI_utf8(m_content[at].text, str)) return (LRESULT)at;
			}
			return LB_ERR;
		}

		LRESULT OnSetCount(UINT, WPARAM wp, LPARAM) {
			m_content.resize((size_t)wp);
			ReloadData();
			return LB_OKAY;
		}
		LRESULT OnInitStorage(UINT, WPARAM wp, LPARAM) {
			m_content.reserve(m_content.size() + (size_t)wp);
			return LB_OKAY;
		}

		LRESULT OnGetText(UINT, WPARAM wp, LPARAM lp) {
			size_t idx = (size_t)wp;
			if (idx < m_content.size()) {
				auto out = reinterpret_cast<wchar_t*>(lp);
				wcscpy(out, pfc::wideFromUTF8(m_content[idx].text.c_str()).c_str());
				return LB_OKAY;
			} else {
				return LB_ERR;
			}
		}
		LRESULT OnGetTextLen(UINT, WPARAM wp, LPARAM) {
			size_t idx = (size_t)wp;
			if (idx < m_content.size()) {
				return pfc::wideFromUTF8(m_content[idx].text.c_str()).length();
			} else {
				return LB_ERR;
			}
		}

		bool GetSubItemText(size_t item, size_t subItem, pfc::string_base& out) const override {
			PFC_ASSERT(subItem == 0);
			PFC_ASSERT(item < m_content.size());
			out = m_content[item].text;
			return true;
		}
	};
}

HWND CListControl_ReplaceListBox(HWND wndReplace) {
	CListBox src(wndReplace);
	const auto style = src.GetStyle();
	HWND ret = NULL;
	if ((style & LBS_NODATA) == 0) {
		PFC_ASSERT(src.GetCount() == 0); // transferring of items not yet implemented
		const auto ctrlID = src.GetDlgCtrlID();
		CWindow parent = src.GetParent();
		auto obj = PP::newWindowObj<CListControl_ListBox>(style);
		ret = obj->CreateInDialog(parent, ctrlID, src);
	}
	return ret;
}
