#pragma once

// ================================================================================
// CTreeMultiSel
// Implementation of multi-selection in a tree view ctrl
// Instantiate with dialog ID of your treeview,
// plug into your dialog's message map.
// Doesn't work correctly with explorer-themed tree controls (glitches happen).
// ================================================================================

#include <set>
#include <vector>

class CTreeMultiSel : public CMessageMap {
public:
	typedef std::set<HTREEITEM> selection_t;
	typedef std::vector<HTREEITEM> selectionOrdered_t;

	CTreeMultiSel(unsigned ID) : m_ID(ID) {}
	
	BEGIN_MSG_MAP_EX(CTreeMultiSel)
		NOTIFY_HANDLER_EX(m_ID, TVN_ITEMEXPANDED, OnItemExpanded)
		NOTIFY_HANDLER_EX(m_ID, NM_CLICK, OnClick)
		NOTIFY_HANDLER_EX(m_ID, TVN_DELETEITEM, OnItemDeleted)
		NOTIFY_HANDLER_EX(m_ID, TVN_SELCHANGING, OnSelChanging)
		NOTIFY_HANDLER_EX(m_ID, TVN_SELCHANGED, OnSelChangedFilter)
		NOTIFY_HANDLER_EX(m_ID, NM_SETFOCUS, OnFocus)
		NOTIFY_HANDLER_EX(m_ID, NM_KILLFOCUS, OnFocus)
		NOTIFY_HANDLER_EX(m_ID, NM_CUSTOMDRAW, OnCustomDraw)
	END_MSG_MAP()

	const unsigned m_ID;

	// Retrieves selected items - on order of appearance in the view
	selectionOrdered_t GetSelectionOrdered(CTreeViewCtrl tree) const {
		HTREEITEM first = tree.GetRootItem();
		selectionOrdered_t ret; ret.reserve( m_selection.size() );
		for(HTREEITEM walk = first; walk != NULL; walk = tree.GetNextVisibleItem(walk)) {
			if (m_selection.count(walk) > 0) ret.push_back( walk );
		}
		return ret;
	}

	//! Undefined order! Use only when order of selected items is not relevant.
	selection_t GetSelection() const { return m_selection; }
	selection_t const & GetSelectionRef() const { return m_selection; }
	bool IsItemSelected(HTREEITEM item) const {return m_selection.count(item) > 0;}
	size_t GetSelCount() const {return m_selection.size();}
	//! Retrieves a single-selection item. Null if nothing or more than one item is selected.
	HTREEITEM GetSingleSel() const {
		if (m_selection.size() != 1) return NULL;
		return *m_selection.begin();
	}

	void OnContextMenu_FixSelection(CTreeViewCtrl tree, CPoint pt) {
		if (pt != CPoint(-1, -1)) {
			WIN32_OP_D(tree.ScreenToClient(&pt));
			UINT flags = 0;
			const HTREEITEM item = tree.HitTest(pt, &flags);
			if (item != NULL && (flags & TVHT_ONITEM) != 0) {
				if (!IsItemSelected(item)) {					
					SelectSingleItem(tree, item);
				}
				CallSelectItem(tree, item);
			}
		}
	}

	void OnLButtonDown(CTreeViewCtrl tree, WPARAM wp, LPARAM lp) {
		if (!IsKeyPressed(VK_CONTROL)) {
			UINT flags = 0;
			HTREEITEM item = tree.HitTest(CPoint(lp), &flags);
			if (item != NULL && (flags & TVHT_ONITEM) != 0) {
				if (!IsItemSelected(item)) tree.SelectItem(item);
			}
		}
	}
	static bool IsNavKey(UINT vk) {
		switch(vk) {
			case VK_UP:
			case VK_DOWN:
			case VK_RIGHT:
			case VK_LEFT:
			case VK_PRIOR:
			case VK_NEXT:
			case VK_HOME:
			case VK_END:
				return true;
			default:
				return false;
		}
	}
	BOOL OnChar(CTreeViewCtrl tree, WPARAM code) {
		switch(code) {
			case ' ':
				if (IsKeyPressed(VK_CONTROL) || !IsTypingInProgress()) {
					HTREEITEM item = tree.GetSelectedItem();
					if (item != NULL) SelectToggleItem(tree, item);
					return TRUE;
				}
				break;
		}
		m_lastTypingTime = GetTickCount(); m_lastTypingTimeValid = true;
		return FALSE;
	}
	BOOL OnKeyDown(CTreeViewCtrl tree, UINT vKey) {
		if (IsNavKey(vKey)) m_lastTypingTimeValid = false;
		switch(vKey) {
			case VK_UP:
				if (IsKeyPressed(VK_CONTROL)) {
					HTREEITEM item = tree.GetSelectedItem();
					if (item != NULL) {
						HTREEITEM prev = tree.GetPrevVisibleItem(item);
						if (prev != NULL) {
							CallSelectItem(tree, prev);
							if (IsKeyPressed(VK_SHIFT)) {
								if (m_selStart == NULL) m_selStart = item;
								SelectItemRange(tree, prev);
							}
						}
					}
					return TRUE;
				}
				break;
			case VK_DOWN:
				if (IsKeyPressed(VK_CONTROL)) {
					HTREEITEM item = tree.GetSelectedItem();
					if (item != NULL) {
						HTREEITEM next = tree.GetNextVisibleItem(item);
						if (next != NULL) {
							CallSelectItem(tree, next);
							if (IsKeyPressed(VK_SHIFT)) {
								if (m_selStart == NULL) m_selStart = item;
								SelectItemRange(tree, next);
							}
						}
					}
					return TRUE;
				}
				break;
			/*case VK_LEFT:
				if (IsKeyPressed(VK_CONTROL)) {
					tree.SendMessage(WM_HSCROLL, SB_LINEUP, 0);
				}
				break;
			case VK_RIGHT:
				if (IsKeyPressed(VK_CONTROL)) {
					tree.SendMessage(WM_HSCROLL, SB_LINEDOWN, 0);
				}
				break;*/
		}
		return FALSE;
	}
private:
	LRESULT OnFocus(LPNMHDR hdr) {
		if ( m_selection.size() > 100 ) {
			CTreeViewCtrl tree(hdr->hwndFrom);
			tree.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
		} else if (m_selection.size() > 0) {
			CTreeViewCtrl tree(hdr->hwndFrom);
			CRgn rgn; rgn.CreateRectRgn(0,0,0,0);
			for(auto walk = m_selection.begin(); walk != m_selection.end(); ++walk) {
				CRect rc;
				if (tree.GetItemRect(*walk, rc, TRUE)) {
					CRgn temp; temp.CreateRectRgnIndirect(rc);
					rgn.CombineRgn(temp, RGN_OR);
				}				
			}
			tree.RedrawWindow(NULL, rgn, RDW_INVALIDATE | RDW_ERASE);
		}
		SetMsgHandled(FALSE);
		return 0;
	}
	void CallSelectItem(CTreeViewCtrl tree, HTREEITEM item) {
		const bool was = m_ownSelChange; m_ownSelChange = true;
		tree.SelectItem(item);
		m_ownSelChange = was;
	}
	LRESULT OnSelChangedFilter(LPNMHDR) {
		if (m_ownSelChangeNotify) SetMsgHandled(FALSE);
		return 0;
	}
	LRESULT OnItemDeleted(LPNMHDR pnmh) {
		const HTREEITEM item = reinterpret_cast<NMTREEVIEW*>(pnmh)->itemOld.hItem;
		m_selection.erase( item );
		if (m_selStart == item) m_selStart = NULL;
		SetMsgHandled(FALSE);
		return 0;
	}
	LRESULT OnItemExpanded(LPNMHDR pnmh) {
		NMTREEVIEW * info = reinterpret_cast<NMTREEVIEW *>(pnmh);
		CTreeViewCtrl tree ( pnmh->hwndFrom );
		if ((info->itemNew.state & TVIS_EXPANDED) == 0) {
			if (DeselectChildren( tree, info->itemNew.hItem )) {
				SendOnSelChanged(tree);
			}
		}
		SetMsgHandled(FALSE);
		return 0;
	}

	BOOL HandleClick(CTreeViewCtrl tree, CPoint pt) {
		UINT htFlags = 0;
		HTREEITEM item = tree.HitTest(pt, &htFlags);
		if (item != NULL && (htFlags & TVHT_ONITEM) != 0) {
			if (IsKeyPressed(VK_CONTROL)) {
				SelectToggleItem(tree, item);
				return TRUE;
			} else if (item == tree.GetSelectedItem() && !IsItemSelected(item)) {
				SelectToggleItem(tree, item);
				return TRUE;
			} else {
				//tree.SelectItem(item);
				return FALSE;
			}
		} else {
			return FALSE;
		}
	}

	LRESULT OnClick(LPNMHDR pnmh) {
		CPoint pt(GetMessagePos());
		CTreeViewCtrl tree ( pnmh->hwndFrom );
		WIN32_OP_D ( tree.ScreenToClient( &pt ) );
		return HandleClick(tree, pt) ? 1 : 0;
	}

	LRESULT OnSelChanging(LPNMHDR pnmh) {
		if (!m_ownSelChange) {
			//console::formatter() << "OnSelChanging";
			NMTREEVIEW * info = reinterpret_cast<NMTREEVIEW *>(pnmh);
			CTreeViewCtrl tree ( pnmh->hwndFrom );
			const HTREEITEM item = info->itemNew.hItem;

			if (IsTypingInProgress()) {
				SelectSingleItem(tree, item);
			} else if (IsKeyPressed(VK_SHIFT)) {
				SelectItemRange(tree, item);
			} else if (IsKeyPressed(VK_CONTROL)) {
				SelectToggleItem(tree, item);
			} else {
				SelectSingleItem(tree, item);
			}
		}
		return 0;
	}

	void SelectItemRange(CTreeViewCtrl tree, HTREEITEM item) {
		if (m_selStart == NULL || m_selStart == item) {
			SelectSingleItem(tree, item);
			return;
		}

		selection_t newSel = GrabRange(tree, m_selStart, item );
		ApplySelection(tree, newSel);
	}
	static selection_t GrabRange(CTreeViewCtrl tree, HTREEITEM item1, HTREEITEM item2) {
		selection_t range1, range2;
		HTREEITEM walk1 = item1, walk2 = item2;
		for(;;) {
			if (walk1 != NULL) {
				range1.insert( walk1 );
				if (walk1 == item2) {
					return range1;
				}
				walk1 = tree.GetNextVisibleItem(walk1);
			}
			if (walk2 != NULL) {
				range2.insert( walk2 );
				if (walk2 == item1) {
					return range2;
				}
				walk2 = tree.GetNextVisibleItem(walk2);
			}
			if (walk1 == NULL && walk2 == NULL) {
				// should not get here
				return selection_t();
			}
		}		
	}
	void SelectToggleItem(CTreeViewCtrl tree, HTREEITEM item) {
		m_selStart = item;
		if ( IsItemSelected( item ) ) {
			m_selection.erase( item );
		} else {
			m_selection.insert( item );
		}
		UpdateItem(tree, item);
	}

	LRESULT OnCustomDraw(LPNMHDR hdr) {
		NMTVCUSTOMDRAW* info = (NMTVCUSTOMDRAW*)hdr;
		switch (info->nmcd.dwDrawStage) {
		case CDDS_ITEMPREPAINT:
			if (this->IsItemSelected((HTREEITEM)info->nmcd.dwItemSpec)) {
				info->nmcd.uItemState |= CDIS_SELECTED;
			} else {
				info->nmcd.uItemState &= ~(CDIS_FOCUS | CDIS_SELECTED);
			}
			return CDRF_DODEFAULT;
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
		default:
			return CDRF_DODEFAULT;
		}
	}
public:
	void SelectSingleItem(CTreeViewCtrl tree, HTREEITEM item) {
		m_selStart = item;
		if (m_selection.size() == 1 && *m_selection.begin() == item) return;
		DeselectAll(tree); SelectItem(tree, item);
	}

	void ApplySelection(CTreeViewCtrl tree, selection_t const & newSel) {
		CRgn updateRgn;
		bool changed = false;
		if (newSel.size() != m_selection.size() && newSel.size() + m_selection.size() > 100) {
			// don't bother with regions
			changed = true;
		} else {
			WIN32_OP_D(updateRgn.CreateRectRgn(0, 0, 0, 0) != NULL);
			for (auto walk = m_selection.begin(); walk != m_selection.end(); ++walk) {
				if (newSel.count(*walk) == 0) {
					changed = true;
					CRect rc;
					if (tree.GetItemRect(*walk, rc, TRUE)) {
						CRgn temp; WIN32_OP_D(temp.CreateRectRgnIndirect(rc));
						WIN32_OP_D(updateRgn.CombineRgn(temp, RGN_OR) != ERROR);
					}
				}
			}
			for (auto walk = newSel.begin(); walk != newSel.end(); ++walk) {
				if (m_selection.count(*walk) == 0) {
					changed = true;
					CRect rc;
					if (tree.GetItemRect(*walk, rc, TRUE)) {
						CRgn temp; WIN32_OP_D(temp.CreateRectRgnIndirect(rc));
						WIN32_OP_D(updateRgn.CombineRgn(temp, RGN_OR) != ERROR);
					}
				}
			}
		}
		if (changed) {
			m_selection = newSel;
			tree.RedrawWindow(NULL, updateRgn);
			SendOnSelChanged(tree);
		}
	}

	void DeselectItem(CTreeViewCtrl tree, HTREEITEM item) {
		if (IsItemSelected(item)) {
			m_selection.erase(item); UpdateItem(tree, item);
		}
	}
	void SelectItem(CTreeViewCtrl tree, HTREEITEM item) {
		if (!IsItemSelected(item)) {
			m_selection.insert(item); UpdateItem(tree, item);
		}
	}

	void DeselectAll(CTreeViewCtrl tree) {
		if (m_selection.size() == 0) return;
		CRgn updateRgn; 
		if (m_selection.size() <= 100) {
			WIN32_OP_D(updateRgn.CreateRectRgn(0, 0, 0, 0) != NULL);
			for (auto walk = m_selection.begin(); walk != m_selection.end(); ++walk) {
				CRect rc;
				if (tree.GetItemRect(*walk, rc, TRUE)) {
					CRgn temp; WIN32_OP_D(temp.CreateRectRgnIndirect(rc));
					WIN32_OP_D(updateRgn.CombineRgn(temp, RGN_OR) != ERROR);
				}
			}
		}
		m_selection.clear();
		tree.RedrawWindow(NULL, updateRgn);
	}
private:
	void UpdateItem(CTreeViewCtrl tree, HTREEITEM item) {
		CRect rc;
		if (tree.GetItemRect(item, rc, TRUE) ) {
			tree.RedrawWindow(rc);
		}
		SendOnSelChanged(tree);
	}
	void SendOnSelChanged(CTreeViewCtrl tree) {
		NMHDR hdr = {};
		hdr.code = TVN_SELCHANGED;
		hdr.hwndFrom = tree;
		hdr.idFrom = m_ID;
		const bool was = m_ownSelChangeNotify; m_ownSelChangeNotify = true;
		tree.GetParent().SendMessage(WM_NOTIFY, m_ID, (LPARAM) &hdr );
		m_ownSelChangeNotify = was;
	}

	bool DeselectChildren( CTreeViewCtrl tree, HTREEITEM item ) {
		bool state = false;
		for(HTREEITEM walk = tree.GetChildItem( item ); walk != NULL; walk = tree.GetNextSiblingItem( walk ) ) {
			if (m_selection.erase(walk) > 0) state = true;
			if (m_selStart == walk) m_selStart = NULL;
			if (tree.GetItemState( walk, TVIS_EXPANDED ) ) {
				if (DeselectChildren( tree, walk )) state = true;
			}
		}
		return state;
	}

	bool IsTypingInProgress() const {
		return m_lastTypingTimeValid && (GetTickCount() - m_lastTypingTime < 500);
	}

	selection_t m_selection;
	HTREEITEM m_selStart = NULL;
	bool m_ownSelChangeNotify = false, m_ownSelChange = false;
	DWORD m_lastTypingTime = 0; bool m_lastTypingTimeValid = false;
};
