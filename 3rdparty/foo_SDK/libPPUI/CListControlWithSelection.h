#pragma once

#include "CListControl.h"

//! Implementation of focus/selection handling. Leaves maintaining focus/selection info to the derived class. \n
//! Most classes should derive from CListControlWithSelectionImpl instead.
class CListControlWithSelectionBase : public CListControl {
public:
	typedef CListControl TParent;
	CListControlWithSelectionBase() {}
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
	virtual void SetGroupFocusByItem(t_size item) = 0;
	virtual size_t GetGroupFocus2() const = 0;
	virtual bool IsItemSelected(t_size index) const = 0;
	virtual void SetSelection(pfc::bit_array const & affected,pfc::bit_array const & status) = 0;
	void SelectSingle(size_t which);
	void SetSelectionAt(size_t idx, bool bSel);
	virtual bool SelectAll();
	void SelectNone();
	virtual void RequestMoveSelection(int delta);
	bool MoveSelectionProbe(int delta);
	virtual void RequestReorder( size_t const * order, size_t count ) = 0;
	virtual void RequestRemoveSelection() = 0;
	virtual void ExecuteDefaultAction(t_size index) = 0;
	virtual void ExecuteDefaultActionGroup(t_size base, t_size count) { (void)base; (void)count; }
	virtual bool ExecuteCanvasDefaultAction(CPoint pt) { (void)pt; return false; }

	virtual t_size GetSelectionStart() const = 0;
	virtual void SetSelectionStart(t_size val) = 0;
	//! Full hook for drag-drop loop
	virtual void RunDragDrop(const CPoint & p_origin,bool p_isRightClick);

	//! Should RunDragDrop() be called at all?
	virtual bool IsDragDropSupported() {return QueryDragDropTypes() != 0;}

	//! Notification, mandatory to call by SetFocusItem() implementation. \n
	//! If overridden by subclass, must call parent.
	virtual void OnFocusChanged(size_t oldFocus, size_t newFocus) { (void)oldFocus; (void)newFocus; }
	virtual void OnFocusChangedGroup2(size_t baseItem) { (void)baseItem; }
	//! Notification, mandatory to call by SetSelection() implementation. \n
	//! If overridden by subclass, must call parent. \n
	//! Affected: Mask indicating what items ACTUALLY CHANGED, old state to be assumed opposite of new. \n
	//! During this call, IsSelected() already returns new state.
	virtual void OnSelectionChanged(pfc::bit_array const& affected, pfc::bit_array const& status) { (void)affected; (void)status; }

	enum {
		dragDrop_reorder = 1 << 0,
		dragDrop_external = 1 << 1,
	};

	virtual uint32_t QueryDragDropTypes() const { return 0; }
	struct dragDropAccept_t {
		DWORD dwEFfect = DROPEFFECT_NONE;
		//! Show drop mark or not?
		bool showDropMark = false;
		//! Drop on item or insert into list?
		bool dropOnItem = false;
	};
	//! Deprecated, use DragDropAccept2()
	virtual DWORD DragDropAccept(IDataObject* obj, bool& showDropMark);
	//! Return info on what you can do with this IDataObject.
	virtual dragDropAccept_t DragDropAccept2(IDataObject*);
	virtual pfc::com_ptr_t<IDataObject> MakeDataObject();
	//! Called upon drop
	//! @param pt Drop point in screen coordinates.
	virtual void OnDrop(IDataObject* obj, CPoint pt) { (void)obj; (void)pt; }
	virtual DWORD DragDropSourceEffects() { return DROPEFFECT_MOVE | DROPEFFECT_COPY;}
	virtual void DragDropSourceSucceeded(DWORD effect) { (void)effect; }

	virtual void AdjustSelectionRect(size_t item, CRect& rc) { (void)item; (void)rc; }

	bool GroupFocusActive() const {return GetGroupFocus2() != SIZE_MAX;}
	
	void RenderOverlay2(const CRect & p_updaterect,CDCHandle p_dc) override;

	bool IsItemFocused(t_size index) const {return GetFocusItem() == index;}
	bool IsGroupHeaderFocused2(size_t atItem) const {return GetGroupFocus2() == atItem;}
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

	//! Fix coordinates of context menu point handed by WM_CONTEXTMENU, turn (-1,-1) into something that makes sense. \n
	//! Input & output in screen coordinates, per WM_CONTEXTMENU conventions.
	CPoint GetContextMenuPoint(LPARAM lp);
	CPoint GetContextMenuPoint(CPoint ptGot);

protected:
	void ToggleDDScroll(bool p_state);
	void AbortSelectDragMode() {AbortSelectDragMode(false);}
	void RenderDropMarkerByOffset2(int offset,CDCHandle p_dc);
	void RenderDropMarker2(CDCHandle dc, t_size item, bool bInside);
	bool RenderDropMarkerClipped2(CDCHandle dc, const CRect & update, t_size item, bool bInside);
	CRect DropMarkerRect(int offset) const;
	int DropMarkerOffset(t_size marker) const;
	void AddDropMarkToUpdateRgn(HRGN p_rgn, t_size p_index, bool bInside = false) const;
	CRect DropMarkerUpdateRect(t_size index,bool bInside) const;
	bool GetFocusRect(CRect & p_rect);
	bool GetFocusRectAbs(CRect & p_rect);
	bool IsOwnDDActive() const {return m_ownDDActive;}

	SIZE DropMarkerMargin() const;
	void MakeDropMarkerPen(CPen & out) const;

	void EnsureVisibleRectAbs(const CRect & p_rect) override;
	virtual size_t EvalTypeFind();

	virtual bool AllowRangeSelect() const { return true; }

	size_t GetDropMark( ) const { return m_dropMark; }
	bool IsDropMarkInside( ) const { return m_dropMarkInside; }
	void SetDropMark( size_t idx, bool bInside );
	void ClearDropMark() { SetDropMark(SIZE_MAX, false); }
private:
	int OnCreatePassThru(LPCREATESTRUCT lpCreateStruct);
	void OnDestroyPassThru();

	struct TDDScrollControl {
		bool m_timerActive = false;

		enum {KTimerID = 0x35bb25af,KTimerPeriod = 25};
	};

	static constexpr unsigned 
		KSelectionTimerID = 0xad8abd04,
		KSelectionTimerPeriod = 50;

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
	void SelectGroupHelper2(size_t p_groupBase,int p_keys);
	void HandleDragSel(const CPoint & p_pt);
	void AbortSelectDragMode(bool p_lostCapture);
	void InitSelectDragMode(const CPoint & p_pt,bool p_rightClick = false);

	void ToggleRangeSelection(pfc::bit_array const & mask);
	void ToggleGroupSelection2(size_t p_item);

	void HandleDDScroll();

	void PrepareDragDrop(const CPoint & p_point,bool p_isRightClick);
	void AbortPrepareDragDropMode(bool p_lostCapture = false);

	bool TypeFindCheck(DWORD ts = GetTickCount()) const;


protected:
	// Spacebar handler
	void ToggleSelectedItems();

	void RenderItem(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc) override;
	void RenderGroupHeader2(size_t baseItem,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc) override;
	void RenderSubItemText(t_size item, t_size subItem,const CRect & subItemRect,const CRect & updateRect,CDCHandle dc, bool allowColors) override;
private:
	bool m_selectDragMode = false;
	CPoint m_selectDragOriginAbs, m_selectDragCurrentAbs;
	bool m_selectDragChanged, m_selectDragMoved;
	TDDScrollControl m_ddScroll;

	bool m_prepareDragDropMode = false, m_prepareDragDropModeRightClick = false;
	bool m_noEnsureVisible = false;
	CPoint m_prepareDragDropOrigin;

	bool m_ownDDActive = false;
	bool m_drawThemeText = false;
	pfc::string8 m_typeFind; DWORD m_typeFindTS = 0;
	
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
	bool IsSingleSelect() const { return m_selectionSupport == selectionSupportSingle; }

	CListControlWithSelectionImpl() {}
	void SetFocusItem(t_size index);
	t_size GetFocusItem() const {return m_groupFocus ? SIZE_MAX : m_focus;}
	void SetGroupFocusByItem(t_size item) override;
	size_t GetGroupFocus2() const override;
	bool IsItemSelected(t_size index) const {return index < m_selection.get_size() ? m_selection[index] : false;}
	void SetSelection(pfc::bit_array const & affected,pfc::bit_array const & status);
	virtual bool CanSelectItem(size_t index) const { (void)index; return true; }
	t_size GetSelectionStart() const {return m_selectionStart;}
	void SetSelectionStart(t_size val) {m_selectionStart = val;}

	void SelHandleReorder(const t_size * order, t_size count);
	void SelHandleRemoval(const pfc::bit_array & mask, t_size oldCount);
	void SelHandleInsertion(t_size base, t_size count, bool select);
	void SelHandleInsertion(pfc::bit_array const & mask, size_t oldCount, size_t newCount, bool select);
	void SelHandleReset();

	void ReloadData() override;
	size_t _DebugGetItemCountSel() const { return m_selection.get_size(); }

	virtual void OnItemsReordered( const size_t* order, size_t count ) override;
	virtual void OnItemsRemoved( pfc::bit_array const & mask, size_t oldCount ) override;
	virtual void OnItemsInsertedEx(pfc::bit_array const& mask, size_t oldCount, size_t newCount, bool bSelect) override;

	pfc::bit_array_bittable GetSelectionMask() const; // returns a standalone object holding a copy of the state
	pfc::bit_array_table GetSelectionMaskRef() const; // returns a TEMPORARY object referencing this list's internal data

	bool SelectAll() override;

	const bool* GetSelectionArray() { RefreshSelectionSize(); return m_selection.get_ptr(); }

protected:
	
	bool AllowRangeSelect() const override { return m_selectionSupport == selectionSupportMulti; }
private:
	void SetSelectionImpl(pfc::bit_array const & affected,pfc::bit_array const & status);
	void RefreshSelectionSize();
	void RefreshSelectionSize(t_size size);
	pfc::array_t<bool> m_selection;
	size_t m_focus = SIZE_MAX, m_selectionStart = SIZE_MAX;
	bool m_groupFocus = false;
};
