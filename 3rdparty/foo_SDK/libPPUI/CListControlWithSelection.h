#pragma once

#include "CListControl.h"

//! Implementation of focus/selection handling. Leaves maintaining focus/selection info to the derived class. \n
//! Most classes should derive from CListControlWithSelectionImpl instead.
class CListControlWithSelectionBase : public CListControl {
public:
	typedef CListControl TParent;
	CListControlWithSelectionBase() : m_selectDragMode(), m_prepareDragDropMode(), m_prepareDragDropModeRightClick(), m_ownDDActive(), m_noEnsureVisible(), m_drawThemeText(), m_typeFindTS() {}
	BEGIN_MSG_MAP_EX(CListControlWithSelectionBase)
		MSG_WM_CREATE(OnCreatePassThru);
		MSG_WM_DESTROY(OnDestroyPassThru);
		CHAIN_MSG_MAP(TParent)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK,OnLButtonDblClk)
		MESSAGE_HANDLER(WM_LBUTTONDOWN,OnButtonDown)
		MESSAGE_HANDLER(WM_RBUTTONDOWN,OnButtonDown)
		MESSAGE_HANDLER(WM_RBUTTONDBLCLK,OnButtonDown)
		MESSAGE_HANDLER(WM_RBUTTONUP,OnRButtonUp)
		MESSAGE_HANDLER(WM_MOUSEMOVE,OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONUP,OnLButtonUp)
		MESSAGE_HANDLER(WM_KEYDOWN,OnKeyDown);
		MESSAGE_HANDLER(WM_SYSKEYDOWN,OnKeyDown);
		MESSAGE_HANDLER(WM_SETFOCUS,OnFocus);
		MESSAGE_HANDLER(WM_KILLFOCUS,OnFocus);
		MESSAGE_HANDLER(WM_TIMER,OnTimer);
		MESSAGE_HANDLER(WM_CAPTURECHANGED,OnCaptureChanged);
		MESSAGE_HANDLER(WM_GETDLGCODE,OnGetDlgCode);
		MSG_WM_CHAR(OnChar)
	END_MSG_MAP()

	virtual void SetFocusItem(t_size index) = 0;
	virtual t_size GetFocusItem() const = 0;
	virtual void SetGroupFocus(int group) = 0;
	virtual void SetGroupFocusByItem(t_size item) = 0;
	virtual int GetGroupFocus() const = 0;
	virtual bool IsItemSelected(t_size index) const = 0;
	virtual void SetSelection(pfc::bit_array const & affected,pfc::bit_array const & status) = 0;
	void SelectSingle(size_t which);
	virtual bool SelectAll();
	void SelectNone();
	virtual void RequestMoveSelection(int delta);
	bool MoveSelectionProbe(int delta);
	virtual void RequestReorder( size_t const * order, size_t count ) = 0;
	virtual void RequestRemoveSelection() = 0;
	virtual void ExecuteDefaultAction(t_size index) = 0;
	virtual void ExecuteDefaultActionGroup(t_size base, t_size count) {}
	virtual bool ExecuteCanvasDefaultAction(CPoint pt) { return false; }

	virtual t_size GetSelectionStart() const = 0;
	virtual void SetSelectionStart(t_size val) = 0;
	//! Full hook for drag-drop loop
	virtual void RunDragDrop(const CPoint & p_origin,bool p_isRightClick);

	//! Should RunDragDrop() be called at all?
	virtual bool IsDragDropSupported() {return QueryDragDropTypes() != 0;}

	//! Notification, mandatory to call by SetFocusItem() implementation. \n
	//! If overridden by subclass, must call parent.
	virtual void OnFocusChanged(size_t newFocus) {}
	virtual void OnFocusChangedGroup(int inGroup) {}
	//! Notification, mandatory to call by SetSelection() implementation. \n
	//! If overridden by subclass, must call parent.
	virtual void OnSelectionChanged(pfc::bit_array const & affected, pfc::bit_array const & status) {}

	enum {
		dragDrop_reorder = 1 << 0,
		dragDrop_external = 1 << 1,
	};

	virtual uint32_t QueryDragDropTypes() const { return 0; }
	virtual DWORD DragDropAccept(IDataObject * obj, bool & showDropMark) { return DROPEFFECT_NONE; }
	virtual pfc::com_ptr_t<IDataObject> MakeDataObject();
	virtual void OnDrop(IDataObject * obj, CPoint pt ) {}
	virtual DWORD DragDropSourceEffects() { return DROPEFFECT_MOVE | DROPEFFECT_COPY;}
	virtual void DragDropSourceSucceeded( DWORD effect ) {}

	bool GroupFocusActive() const {return GetGroupFocus() >= 0;}
	
	void RenderOverlay(const CRect & p_updaterect,CDCHandle p_dc);

	bool IsItemFocused(t_size index) const {return GetFocusItem() == index;}
	bool IsGroupHeaderFocused(int p_id) const {return GetGroupFocus() == p_id;}
	void ToggleSelection(pfc::bit_array const & mask);

	size_t GetSelectedCount(pfc::bit_array const & mask,size_t max = SIZE_MAX) const;
	size_t GetSelectedCount() const {return GetSelectedCount(pfc::bit_array_true());}
	size_t GetSingleSel() const;
	size_t GetFirstSelected() const;
	size_t GetLastSelected() const;

	//! Execute default action per focus or selection depending on what's focused/selected
	virtual void ExecuteDefaultActionByFocus();

	void FocusToUpdateRgn(HRGN rgn);

	
	//! Self-contained minimal drag and drop implementation for reordering list items only; dummy IDataObject presented. \n
	//! Call from your override of RunDragDrop(), if p_isRightClick is false / left button clicked, never with right button clicked. \n
	//! On success, use MakeDropReorderPermutation() to fetch the permutation to apply to your content.
	bool RunReorderDragDrop(CPoint ptOrigin, CPoint & ptDrop);

	bool MakeDropReorderPermutation(pfc::array_t<t_size> & out, CPoint ptDrop) const;

	size_t GetPasteTarget( const CPoint * ptPaste = nullptr ) const;

	CPoint GetContextMenuPoint(LPARAM lp);
	CPoint GetContextMenuPoint(CPoint ptGot);

protected:
	void ToggleDDScroll(bool p_state);
	void AbortSelectDragMode() {AbortSelectDragMode(false);}
	void RenderDropMarkerByOffset(int offset,CDCHandle p_dc);
	void RenderDropMarker(CDCHandle dc, t_size item, bool bInside);
	bool RenderDropMarkerClipped(CDCHandle dc, const CRect & update, t_size item, bool bInside);
	CRect DropMarkerRect(int offset) const;
	int DropMarkerOffset(t_size marker) const;
	void AddDropMarkToUpdateRgn(HRGN p_rgn, t_size p_index, bool bInside = false) const;
	CRect DropMarkerUpdateRect(t_size index,bool bInside) const;
	bool GetFocusRect(CRect & p_rect);
	bool GetFocusRectAbs(CRect & p_rect);
	bool IsOwnDDActive() const {return m_ownDDActive;}

	SIZE DropMarkerMargin() const;
	void MakeDropMarkerPen(CPen & out) const;

	virtual void EnsureVisibleRectAbs(const CRect & p_rect);
	virtual size_t EvalTypeFind();

	virtual bool AllowRangeSelect() const { return true; }

	CRect GetWholeSelectionRectAbs() const;

	size_t GetDropMark( ) const { return m_dropMark; }
	bool IsDropMarkInside( ) const { return m_dropMarkInside; }
	void SetDropMark( size_t idx, bool bInside );
	void ClearDropMark() { SetDropMark(pfc_infinite, false); }
private:
	int OnCreatePassThru(LPCREATESTRUCT lpCreateStruct);
	void OnDestroyPassThru();

	struct TDDScrollControl {
		bool m_timerActive = false;

		enum {KTimerID = 0x35bb25af,KTimerPeriod = 25};
	};

	enum {
		KSelectionTimerID = 0xad8abd04,
		KSelectionTimerPeriod = 50,
	};

	LRESULT OnFocus(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnKeyDown(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnLButtonDblClk(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnButtonDown(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnRButtonUp(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnMouseMove(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnLButtonUp(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnTimer(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnCaptureChanged(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnGetDlgCode(UINT,WPARAM,LPARAM,BOOL&);
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	void RunTypeFind();

	void OnKeyDown_HomeEndHelper(bool isEnd, int p_keys);
	void OnKeyDown_SetIndexHelper(int p_index, int p_keys);
	void OnKeyDown_SetIndexDeltaHelper(int p_delta, int p_keys);
	void OnKeyDown_SetIndexDeltaLineHelper(int p_delta, int p_keys);
	void OnKeyDown_SetIndexDeltaPageHelper(int p_delta, int p_keys);
	void SelectGroupHelper(int p_group,int p_keys);
	void HandleDragSel(const CPoint & p_pt);
	void AbortSelectDragMode(bool p_lostCapture);
	void InitSelectDragMode(const CPoint & p_pt,bool p_rightClick = false);

	void ToggleRangeSelection(pfc::bit_array const & mask);
	void ToggleGroupSelection(int p_group);

	void HandleDDScroll();

	void PrepareDragDrop(const CPoint & p_point,bool p_isRightClick);
	void AbortPrepareDragDropMode(bool p_lostCapture = false);

	bool TypeFindCheck(DWORD ts = GetTickCount()) const;


protected:
	// Spacebar handler
	void ToggleSelectedItems();

	void RenderItem(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc);
	void RenderGroupHeader(int p_group,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc);
	void RenderSubItemText(t_size item, t_size subItem,const CRect & subItemRect,const CRect & updateRect,CDCHandle dc, bool allowColors);
private:
	bool m_selectDragMode;
	CPoint m_selectDragOriginAbs, m_selectDragCurrentAbs;
	bool m_selectDragChanged, m_selectDragMoved;
	TDDScrollControl m_ddScroll;

	bool m_prepareDragDropMode, m_prepareDragDropModeRightClick;
	bool m_noEnsureVisible;
	CPoint m_prepareDragDropOrigin;

	bool m_ownDDActive;
	bool m_drawThemeText;
	pfc::string8 m_typeFind; DWORD m_typeFindTS;
	
	size_t m_dropMark = SIZE_MAX; bool m_dropMarkInside = false;
};

//! CListControlWithSelectionImpl implements virtual methods of CListControlWithSelectionBase,
//! maintaining focus/selection info for you.
class CListControlWithSelectionImpl : public CListControlWithSelectionBase {
public:

	enum { selectionSupportNone = 0, selectionSupportSingle, selectionSupportMulti };
	unsigned m_selectionSupport = selectionSupportMulti;

	void SetSelectionModeNone() { m_selectionSupport = selectionSupportNone; }
	void SetSelectionModeSingle() { m_selectionSupport = selectionSupportSingle; }
	void SetSelectionModeMulti() { m_selectionSupport = selectionSupportMulti; }


	CListControlWithSelectionImpl() {}
	void SetFocusItem(t_size index);
	t_size GetFocusItem() const {return m_groupFocus ? SIZE_MAX : m_focus;}
	void SetGroupFocus(int group);
	void SetGroupFocusByItem(t_size item);
	int GetGroupFocus() const;
	bool IsItemSelected(t_size index) const {return index < m_selection.get_size() ? m_selection[index] : false;}
	void SetSelection(pfc::bit_array const & affected,pfc::bit_array const & status);
	virtual bool CanSelectItem( size_t index ) const { return true; }
	t_size GetSelectionStart() const {return m_selectionStart;}
	void SetSelectionStart(t_size val) {m_selectionStart = val;}

	void SelHandleReorder(const t_size * order, t_size count);
	void SelHandleRemoval(const pfc::bit_array & mask, t_size oldCount);
	void SelHandleInsertion(t_size base, t_size count, bool select);
	void SelHandleReset();

	void ReloadData() override;
	size_t _DebugGetItemCountSel() const { return m_selection.get_size(); }

	virtual void OnItemsReordered( const size_t* order, size_t count ) override;
	virtual void OnItemsRemoved( pfc::bit_array const & mask, size_t oldCount ) override;
	virtual void OnItemsInserted( size_t at, size_t count, bool bSelect ) override;

	pfc::bit_array_bittable GetSelectionMask(); // returns an standalone object holding a copy of the state
	
	bool SelectAll() override;

protected:
	const bool * GetSelectionArray() {RefreshSelectionSize();return m_selection.get_ptr();}
	pfc::bit_array_table GetSelectionMaskRef(); // returns a *temporary* object referencing internal data structures
	
	bool AllowRangeSelect() const override { return m_selectionSupport == selectionSupportMulti; }

private:
	void SetSelectionImpl(pfc::bit_array const & affected,pfc::bit_array const & status);
	void RefreshSelectionSize();
	void RefreshSelectionSize(t_size size);
	pfc::array_t<bool> m_selection;
	size_t m_focus = SIZE_MAX, m_selectionStart = SIZE_MAX;
	bool m_groupFocus = false;
};
