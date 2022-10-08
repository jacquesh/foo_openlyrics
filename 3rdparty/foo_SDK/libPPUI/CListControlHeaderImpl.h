#pragma once

class CListCell;

class CListControlHeaderImpl : public CListControlFontOps {
private:
	typedef CListControlFontOps TParent;
public:
	CListControlHeaderImpl() {}

	BEGIN_MSG_MAP_EX(CListControlHeaderImpl)
		MSG_WM_THEMECHANGED(OnThemeChangedPT)
		MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
		MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST, WM_MOUSELAST, MousePassThru);
		MSG_WM_MOUSELEAVE(OnMouseLeave)
		MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic);
		MESSAGE_HANDLER(WM_KEYDOWN,OnKeyDown);
		MESSAGE_HANDLER(WM_SYSKEYDOWN,OnKeyDown);
		MESSAGE_HANDLER_EX(WM_SIZE,OnSizePassThru);
		NOTIFY_CODE_HANDLER(HDN_ITEMCHANGED,OnHeaderItemChanged);
		NOTIFY_CODE_HANDLER(HDN_ENDDRAG,OnHeaderEndDrag);
		NOTIFY_CODE_HANDLER(HDN_ITEMCLICK,OnHeaderItemClick);
		NOTIFY_CODE_HANDLER(HDN_DIVIDERDBLCLICK,OnDividerDoubleClick);
		NOTIFY_CODE_HANDLER_EX(NM_CUSTOMDRAW, OnHeaderCustomDraw);
		MSG_WM_SETCURSOR(OnSetCursor);
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_ENABLE(OnEnable)
		MSG_WM_KILLFOCUS(OnKillFocus)
		MSG_WM_WINDOWPOSCHANGED(OnWindowPosChanged)
		CHAIN_MSG_MAP(TParent)
	END_MSG_MAP()

	typedef uint32_t cellState_t;
	typedef CListCell * cellType_t;

	int GetHeaderItemWidth( int which );
	void InitializeHeaderCtrl(DWORD flags = HDS_FULLDRAG);
	void InitializeHeaderCtrlSortable() {InitializeHeaderCtrl(HDS_FULLDRAG | HDS_BUTTONS);}
	CHeaderCtrl GetHeaderCtrl() const {return m_header;}
	void SetSortIndicator( size_t whichColumn, bool isUp );
	void ClearSortIndicator();

	bool IsHeaderEnabled() const {return m_header.m_hWnd != NULL;}
	void ResetColumns(bool update = true);

	static constexpr uint32_t columnWidthMax = (uint32_t)INT32_MAX,
		columnWidthAuto = UINT32_MAX,
		columnWidthAutoUseContent = UINT32_MAX - 1;

	void AddColumn(const char * label, uint32_t widthPixels, DWORD fmtFlags = HDF_LEFT,bool update = true);
	//! Extended AddColumn, specifies width in pixels @ 96DPI instead of screen-specific pixels
	void AddColumnEx( const char * label, uint32_t widthPixelsAt96DPI, DWORD fmtFlags = HDF_LEFT, bool update = true );
	//! Extended AddColumn, specifies width in Dialog Length Units (DLU), assumes parent of this list to be a dialog window.
	void AddColumnDLU( const char * label, uint32_t widthDLU, DWORD fmtFlags = HDF_LEFT, bool update = true );
	//! Extended AddColumn, specifies width as floating-point value of pixels at 96DPI. \n
	//! For DPI-safe storage of user's column widths.
	void AddColumnF( const char * label, float widthF, DWORD fmtFlags = HDF_LEFT, bool update = true );
	void AddColumnAutoWidth( const char * label, DWORD fmtFlags = HDF_LEFT, bool bUpdate = true) { AddColumn(label, columnWidthAuto, fmtFlags, bUpdate); }
	bool DeleteColumn(size_t index, bool updateView = true);
	void DeleteColumns( pfc::bit_array const & mask, bool updateView = true);
	void ResizeColumn(t_size index, t_uint32 widthPixels, bool updateView = true);
	void SetColumn( size_t which, const char * title, DWORD fmtFlags = HDF_LEFT, bool updateView = true);
	void GetColumnText(size_t which, pfc::string_base & out) const;

	uint32_t GetOptimalColumnWidth( size_t index ) const;
	uint32_t GetOptimalColumnWidthFixed( const char * fixedText, bool pad = true) const;
	uint32_t GetColumnsBlankWidth( size_t colExclude = SIZE_MAX ) const;
	void SizeColumnToContent( size_t which, uint32_t minWidth );
	void SizeColumnToContentFillBlank( size_t which );

	//! If creating a custom headerless multi column scheme, override these to manipulate your columns
	virtual size_t GetColumnCount() const;
	virtual uint32_t GetSubItemWidth(size_t subItem) const;
	//! Returns column width as a floating-point value of pixels at 96DPI. \n
	//! For DPI-safe storage of user's column widths.
	float GetColumnWidthF( size_t subItem ) const;
	//! Indicate how many columns a specific row/column cell spans\n
	//! This makes sense only if the columns can't be user-reordered
	virtual size_t GetSubItemSpan(size_t row, size_t column) const;

	t_size GetSubItemOrder(t_size subItem) const;
	int GetItemWidth() const override;

	void SetHeaderFont(HFONT font);
protected:
	CRect GetClientRectHook() const override;
	void RenderBackground(CDCHandle dc, CRect const& rc) override;

	struct GetOptimalWidth_Cache {
		//! For temporary use.
		pfc::string8_fastalloc m_stringTemp, m_stringTempUnfuckAmpersands;
		//! For temporary use.
		pfc::stringcvt::string_wide_from_utf8_t<pfc::alloc_fast_aggressive> m_convertTemp;
		//! Our DC for measuring text. Correct font pre-selected.
		CDCHandle m_dc;

		t_uint32 GetStringTempWidth();
	};

	void UpdateHeaderLayout();
	void OnViewOriginChange(CPoint p_delta) override;
	void RenderItemText(t_size item,const CRect & itemRect,const CRect & updateRect,CDCHandle dc, bool allowColors) override;
	void RenderGroupHeaderText2(size_t baseItem,const CRect & headerRect,const CRect & updateRect,CDCHandle dc) override;
	void RenderGroupOverlay(size_t baseItem, const CRect& p_groupWhole, const CRect& p_updateRect, CDCHandle p_dc) override;
	void UpdateGroupOverlayColumnByID(groupID_t groupID, size_t subItem);

	//! Converts an item/subitem rect to a rect in which the text should be rendered, removing spacing to the left/right of the text.
	virtual CRect GetItemTextRectHook(size_t item, size_t subItem, CRect const & itemRect) const;
	CRect GetItemTextRect(CRect const & itemRect) const;
	//! Override for custom spacing to the left/right of the text in each column.
	virtual t_uint32 GetColumnSpacing() const {return MulDiv(4,m_dpi.cx,96);}
	//! Override for column-header-click sorting.
	virtual void OnColumnHeaderClick(t_size index) { (void)index; }
	//! Override to supply item labels.
	virtual bool GetSubItemText(t_size item, t_size subItem, pfc::string_base& out) const { (void)item; (void)subItem; (void)out; return false; }
	//! Override if you support groups.
	virtual bool GetGroupHeaderText2(size_t baseItem, pfc::string_base& out) const { (void)baseItem; (void)out; return false; }
	//! Override optionally.
	virtual void RenderSubItemText(t_size item, t_size subItem,const CRect & subItemRect,const CRect & updateRect,CDCHandle dc, bool allowColors);
	virtual void RenderGroupOverlayColumn(size_t baseItem, size_t subItem, const CRect& p_groupWhole, const CRect& p_updateRect, CDCHandle p_dc) { (void)baseItem; (void)subItem; (void)p_groupWhole; (void)p_updateRect; (void)p_dc; }

	virtual void OnColumnsChanged();

	virtual t_uint32 GetOptimalSubItemWidth(t_size item, t_size subItem, GetOptimalWidth_Cache & cache) const;

	virtual t_uint32 GetOptimalGroupHeaderWidth2(size_t baseItem) const;

	
	bool GetItemAtPointAbsEx( CPoint pt, size_t & outItem, size_t & outSubItem ) const;
	cellType_t GetCellTypeAtPointAbs( CPoint pt ) const;
	virtual cellType_t GetCellType( size_t item, size_t subItem ) const;
	virtual bool AllowTypeFindInCell( size_t item, size_t subItem ) const;
	virtual bool GetCellTypeSupported() const { return false; } // optimization hint, some expensive checks can be suppressed if cell types are not used for this view
	virtual bool GetCellCheckState(size_t item, size_t subItem) const { (void)item; (void)subItem; return false; }
	virtual void SetCellCheckState(size_t item, size_t subItem, bool value);
	virtual bool ToggleSelectedItemsHook(const pfc::bit_array & mask) override;

	virtual bool RenderCellImageTest(size_t item, size_t subItem) const { return false; }
	virtual void RenderCellImage(size_t item, size_t subItem, CDCHandle, const CRect&) const {}

	t_uint32 GetOptimalColumnWidth(t_size which, GetOptimalWidth_Cache & cache) const;
	t_uint32 GetOptimalSubItemWidthSimple(t_size item, t_size subItem) const;
	
	void AutoColumnWidths(const pfc::bit_array & mask,bool expandLast = false);
	void AutoColumnWidths() {AutoColumnWidths(pfc::bit_array_true());}
	void AutoColumnWidth(t_size which) {AutoColumnWidths(pfc::bit_array_one(which));}

	virtual bool OnColumnHeaderDrag(t_size index, t_size newOrder);

	void OnItemClicked(t_size item, CPoint pt) override;
	virtual void OnSubItemClicked(t_size item, t_size subItem,CPoint pt);
	bool OnClickedSpecialHitTest(CPoint pt) override;
	bool OnClickedSpecial(DWORD status, CPoint pt) override;
	static bool CellTypeUsesSpecialHitTests( cellType_t ct );


	CRect GetSubItemRectAbs(t_size item,t_size subItem) const;
	CRect GetSubItemRect(t_size item,t_size subItem) const;

	t_size SubItemFromPointAbs(CPoint pt) const;

	static bool CellTypeReactsToMouseOver( cellType_t ct );
	virtual CRect CellHotRect( size_t item, size_t subItem, cellType_t ct, CRect rcCell );
	CRect CellHotRect( size_t item, size_t subItem, cellType_t ct );
	virtual double CellTextScale(size_t item, size_t subItem) { (void)item; (void)subItem; return 1; }
	virtual bool IsSubItemGrayed(size_t item, size_t subItem) { (void)item; (void)subItem; return !this->IsWindowEnabled(); }

	virtual void CellTrackMouseMove(size_t item, size_t subItem, UINT msg, DWORD status, CPoint pt) { (void)item; (void)subItem; (void)msg; (void)status; (void)pt; }

	// HDF_* constants for this column, override when not using list header control. Used to control text alignment.
	virtual DWORD GetColumnFormat(t_size which) const;
	void SetColumnFormat(t_size which,DWORD format);
	void SetColumnSort(t_size which, bool isUp);

	std::vector<int> GetColumnOrderArray() const;

	bool AllowScrollbar(bool vertical) const override;

	void RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,size_t item, uint32_t bkColor) override;

	void ColumnWidthFix(); // Call RecalcItemWidth() / ProcessAutoWidth()

	void ReloadData() override;

	size_t HotItem() const { return m_hotItem; }
	size_t HotSubItem() const { return m_hotSubItem; }

	virtual void RequestEditItem(size_t item, size_t subItem);
private:
	void OnLButtonDblClk(UINT nFlags, CPoint point);
	void OnThemeChangedPT();
	void OnEnable(BOOL) { Invalidate(); }
	HBRUSH OnCtlColorStatic(CDCHandle dc, CStatic wndStatic);
	void ProcessColumnsChange() { OnColumnsChanged();}
	LRESULT OnSizePassThru(UINT,WPARAM,LPARAM);
	LRESULT OnHeaderItemClick(int,LPNMHDR,BOOL&);
	LRESULT OnHeaderCustomDraw(LPNMHDR);
	LRESULT OnDividerDoubleClick(int,LPNMHDR,BOOL&);
	LRESULT OnHeaderItemChanged(int,LPNMHDR,BOOL&);
	LRESULT OnHeaderEndDrag(int,LPNMHDR,BOOL&);
	LRESULT OnKeyDown(UINT,WPARAM,LPARAM,BOOL&);
	void OnDestroy();
	BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
	void OnMouseMove(UINT nFlags, CPoint point);

	void RecalcItemWidth(); // FIXED width math
	void ProcessAutoWidth(); // DYNAMIC width math
	
	bool m_ownColumnsChange = false;

	int m_itemWidth = 0;
	int m_clientWidth = 0;
	CHeaderCtrl m_header;
	bool m_headerDark = false;
	CStatic m_headerLine;

	bool HaveAutoWidthColumns() const;
	bool HaveAutoWidthContentColumns() const;

	struct colRuntime_t {
		bool m_autoWidth = false;
		bool m_autoWidthContent = false;
		int m_widthPixels = 0;
		uint32_t m_userWidth = 0;
		std::string m_text;

		bool autoWidth() const { return m_userWidth > columnWidthMax; }
		bool autoWidthContent() const { return m_userWidth == columnWidthAutoUseContent; }
		bool autoWidthPlain() const { return m_userWidth == columnWidthAuto; }
	};

	std::vector<colRuntime_t> m_colRuntime;
	

	//for group headers
	GdiplusScope m_gdiPlusScope;

	void SetPressedItem(size_t row, size_t column);
	void ClearPressedItem() {SetPressedItem(SIZE_MAX, SIZE_MAX);}
	void SetHotItem( size_t row, size_t column );
	void ClearHotItem() { SetHotItem(SIZE_MAX, SIZE_MAX); }

	virtual void HotItemChanged(size_t row, size_t column) { (void)row; (void)column; }
	virtual void PressedItemChanged(size_t row, size_t column) { (void)row; (void)column; }

	size_t m_pressedItem = SIZE_MAX, m_pressedSubItem = SIZE_MAX;
	size_t m_hotItem = SIZE_MAX, m_hotSubItem = SIZE_MAX;

private:
	// ==== mySetCapture stuff ====
	// SetCapture()-like functionality used for tracking of hot fields.
	// Not a part of the API, do not reuse in subclasses.
	void mySetCapture(CaptureProc_t proc);
	void myReleaseCapture();

	void TrackMouseLeave();

	void mySetCaptureMsgHandled(BOOL v) { this->SetMsgHandled(v); }
	CaptureProc_t m_captureProc;

	void OnMouseLeave();
	void OnKillFocus(CWindow);
	void OnWindowPosChanged(LPWINDOWPOS);
	LRESULT MousePassThru(UINT, WPARAM, LPARAM);
};
