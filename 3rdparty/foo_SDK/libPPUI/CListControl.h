#pragma once

// ================================================================================
// Main CListControl implementation
// 
// For ready-to-use CListControl specializations, 
// see CListControlSimple.h and CListControlOwnerData.h
// ================================================================================


#pragma comment(lib, "uxtheme.lib")

#include <functional>
#include <list>
#include <vector>
#include <set>
#include <string>
#include <map>
#include "CMiddleDragImpl.h"
#include "wtl-pp.h"
#include "gesture.h"
#include "gdiplus_helpers.h"

#define CListControl_ScrollWindowFix

#ifdef CListControl_ScrollWindowFix
#define WS_EX_COMPOSITED_CListControl 0
#else
#define WS_EX_COMPOSITED_CListControl WS_EX_COMPOSITED
#endif


typedef std::function< bool ( UINT, DWORD, CPoint ) > CaptureProc_t;

typedef CWinTraits<WS_VSCROLL | WS_HSCROLL | WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_COMPOSITED_CListControl> CListControlTraits;

class CListControlImpl : public CWindowImpl<CListControlImpl,CWindow,CListControlTraits> {
public:
	typedef uint64_t groupID_t;

	CListControlImpl() {}

	static const wchar_t* GetWndClassName() { return L"libPPUI:CListControl"; }
	DECLARE_WND_CLASS_EX(GetWndClassName(), CS_DBLCLKS, (-1));
	
	// Wrapper around CWindowImpl::Create().
	// Creates CListControl replacing another dialog control with the specified ID.
	// Note that m_dlgWantEnter is set to false by this method, as it's typically unwanted in dialogs.
	HWND CreateInDialog( CWindow wndDialog, UINT replaceControlID );
	HWND CreateInDialog(CWindow wndDialog, UINT replaceControlID, CWindow wndReplace);

	enum {
		MSG_SIZE_ASYNC = WM_USER + 13,
		MSG_EXEC_DEFERRED,
		UserMsgBase
	};
	static UINT msgSetDarkMode();
	const UINT MSG_SET_DARK = msgSetDarkMode();

	static void wndSetDarkMode(CWindow wndListControl, bool bDark);

	BEGIN_MSG_MAP_EX(CListControlImpl)
		MESSAGE_HANDLER_EX(MSG_EXEC_DEFERRED, OnExecDeferred);
		MSG_WM_PAINT(OnPaint)
		MSG_WM_PRINTCLIENT(OnPrintClient);
		MESSAGE_HANDLER(WM_VSCROLL,OnVScroll);
		MESSAGE_HANDLER(WM_HSCROLL,OnHScroll);
		MESSAGE_HANDLER(WM_SIZE,OnSize);
		MESSAGE_HANDLER(WM_MOUSEHWHEEL,OnHWheel);
		MESSAGE_HANDLER(WM_MOUSEWHEEL,OnVWheel);
		MESSAGE_HANDLER(WM_LBUTTONDOWN,SetFocusPassThru);
		MESSAGE_HANDLER(WM_RBUTTONDOWN,SetFocusPassThru);
		MESSAGE_HANDLER(WM_MBUTTONDOWN,SetFocusPassThru);
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK,SetFocusPassThru);
		MESSAGE_HANDLER(WM_RBUTTONDBLCLK,SetFocusPassThru);
		MESSAGE_HANDLER(WM_MBUTTONDBLCLK,SetFocusPassThru);
		MESSAGE_HANDLER(WM_CREATE,OnCreatePassThru);
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MESSAGE_HANDLER(MSG_SIZE_ASYNC,OnSizeAsync);
		MESSAGE_HANDLER(WM_GESTURE, OnGesture)
		MSG_WM_THEMECHANGED(OnThemeChanged)
		MESSAGE_HANDLER_EX( WM_GETDLGCODE, OnGetDlgCode )
		MESSAGE_HANDLER_EX( MSG_SET_DARK, OnSetDark )
		MSG_WM_KEYDOWN(OnKeyDown)
	END_MSG_MAP()

	virtual void ReloadData();
	virtual void ReloadItems(pfc::bit_array const & mask);

	//! Hookable function called in response to reordering of items. Redraws the view and updates internal data to reflect the change.
	virtual void OnItemsReordered( const size_t * order, size_t count );
	//! Hookable function called in response to removal of items. Redraws the view and updates internal data to reflect the change.
	virtual void OnItemsRemoved(pfc::bit_array const& mask, size_t oldCount) { (void)mask; (void)oldCount;  ReloadData(); }
	//! Hookable function called in response to insertion of items. Redraws the view and updates internal data to reflect the change.
	virtual void OnItemsInsertedEx(pfc::bit_array const& mask, size_t oldCount, size_t newCount, bool bSelect) { (void)mask; (void)oldCount; (void) newCount; (void)bSelect; ReloadData(); }
	void OnItemsInserted(size_t at, size_t count, bool bSelect);

	//! Helper around OnItemsRemoved()
	void OnItemRemoved(size_t which);

	void ReloadItem(size_t i) { ReloadItems( pfc::bit_array_one(i) ); }
	void OnViewAreaChanged() {OnViewAreaChanged(GetViewOrigin());}
	void OnViewAreaChanged(CPoint p_originOverride);
	void UpdateGroupHeader2(size_t atItem);
	void UpdateItems(const pfc::bit_array & p_mask);
	void UpdateItemsAndHeaders(const pfc::bit_array & p_mask);
	void UpdateItem(t_size p_item) {UpdateItems(pfc::bit_array_one(p_item));}
	void UpdateItemsAll() {Invalidate();}
	void EnsureItemVisible(t_size p_item, bool bUser = false);
	void EnsureHeaderVisible2(size_t atItem);
	virtual void EnsureVisibleRectAbs(const CRect & p_rect);
	CRect GetItemRect(t_size p_item) const;
	bool GetGroupHeaderRect2(size_t atItem,CRect & p_rect) const;
	CRect GetItemRectAbs(t_size p_item) const;
	int GetItemOffsetAbs(size_t item) const;
	int GetItemOffsetAbs2(size_t from, size_t to) const;
	int GetItemBottomOffsetAbs(size_t item) const;
	int GetItemHeightCached(size_t item) const;
	int GetItemContentHeightCached(size_t item) const;
	bool GetGroupHeaderRectAbs2(size_t atItem,CRect & p_rect) const;
	CPoint GetViewOrigin() const {return m_viewOrigin;}
	CPoint GetViewOffset() const {return GetViewOrigin() - GetClientOrigin();}
	CPoint PointAbsToClient(CPoint pt) const;
	CPoint PointClientToAbs(CPoint pt) const;
	CRect RectAbsToClient(CRect rc) const;
	CRect RectClientToAbs(CRect rc) const;
	int GetViewAreaWidth() const {return GetItemWidth();}
	int GetViewAreaHeight() const;
	CRect GetViewAreaRectAbs() const;
	CRect GetViewAreaRect() const;
	CRect GetValidViewOriginArea() const;
	bool GetItemRangeAbs(const CRect & p_rect,t_size & p_base,t_size & p_count) const;
	bool GetItemRangeAbsInclHeaders(const CRect & p_rect,t_size & p_base,t_size & p_count) const;
	bool GetItemRange(const CRect & p_rect,t_size & p_base,t_size & p_count) const;
	void MoveViewOriginNoClip(CPoint p_target);
	void MoveViewOrigin(CPoint p_target);
	CPoint ClipViewOrigin(CPoint p_origin) const;
	void MoveViewOriginDelta(CPoint p_delta) {MoveViewOrigin( GetViewOrigin() + p_delta );}
	void MoveViewOriginDeltaNoClip(CPoint p_delta) {MoveViewOriginNoClip( GetViewOrigin() + p_delta );}
	bool ItemFromPoint(CPoint const & p_pt, t_size & p_item) const;
	size_t ItemFromPoint(CPoint const& pt) const;
	bool GroupHeaderFromPoint2(CPoint const & p_pt,size_t & p_atItem) const {return GroupHeaderFromPointAbs2( PointClientToAbs(p_pt),p_atItem);}
	bool ItemFromPointAbs(CPoint const & p_pt,t_size & p_item) const;
	size_t ItemFromPointAbs(CPoint const& p_pt) const;
	bool GroupHeaderFromPointAbs2(CPoint const & p_pt,size_t & p_atItem) const;
	size_t IndexFromPointAbs(CPoint pt) const;
	size_t IndexFromPointAbs(int ptY) const;

	size_t ResolveGroupRange2(t_size p_base) const;
	bool ResolveGroupRangeCached(size_t itemInGroup, size_t & outBegin, size_t & outEnd) const;

	virtual int GetGroupHeaderHeight() const {return 0;}
	virtual int GetItemHeight() const {return 0;}
	
	// Variable height items - return -1 to use generic.
	// This is intended to be used scarcely. Layout engine assumes that majority of items use the generic height.
	// Heavy use of variable height items on large data sets will lead to performance issues.
	virtual int GetItemHeight2(size_t which) const;

	// Companion to GetItemHeight2(), returns portion of GetItemHeight2()-defined item which the item content actually occupies
	virtual int GetItemHeight2Content(size_t which, int iHeight) const { (void)which; return iHeight; }

	virtual int GetMinGroupHeight() const { return 0; }
	virtual int GetMinGroupHeight2(size_t itemInGroup) const { (void)itemInGroup; return GetMinGroupHeight(); }
	void MinGroupHeight2Changed(size_t itemInGroup, bool reloadWhole = false);
	void MinGroupHeight2ChangedForGroup(groupID_t group, bool reloadWhole = false);
	virtual int GetItemWidth() const {return 0;}
	virtual t_size GetItemCount() const {return 0;}

	bool IsItemFirstInGroupCached(size_t item) const;
	bool IsItemFirstInGroup(size_t item) const;
	bool IsItemLastInGroup(size_t item) const;

	virtual groupID_t GetItemGroup(t_size p_item) const { (void)p_item; return 0; }

	virtual void RenderRect(const CRect& p_rect, CDCHandle p_dc);
	//override optionally
	virtual void RenderItem(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc);
	//override optionally
	virtual void RenderGroupHeader2(size_t p_item,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc);

	virtual void RenderGroupOverlay(size_t baseItem, const CRect& p_groupWhole, const CRect& p_updateRect, CDCHandle p_dc) { (void)baseItem; (void)p_groupWhole; (void)p_updateRect; (void)p_dc; }
	void UpdateGroupOverlayByID(groupID_t groupID, int xFrom = 0, int xTo = -1);
	bool GetGroupOverlayRectAbs(size_t atItem, CRect & outRect);

	//called by default RenderItem implementation
	virtual void RenderItemText(t_size p_item, const CRect& p_itemRect, const CRect& p_updateRect, CDCHandle p_dc, bool allowColors) { (void)p_item; (void)p_itemRect; (void)p_updateRect; (void)p_dc; (void)allowColors; }
	//called by default RenderItem implementation
	virtual void RenderGroupHeaderText2(size_t p_item, const CRect& p_headerRect, const CRect& p_updateRect, CDCHandle p_dc) { (void)p_item; (void)p_headerRect; (void)p_updateRect; (void)p_dc; }

	virtual void RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,size_t item, uint32_t bkColor);
	virtual void RenderGroupHeaderBackground(CDCHandle p_dc,const CRect & p_headerRect,int iGroup);

	virtual void RenderBackground( CDCHandle dc, CRect const & rc );

	virtual void OnViewOriginChange(CPoint p_delta) { (void)p_delta; }

	// RenderOverlay2 takes rect in client coords, not absolute
	// p_dc operates on client coords also
	virtual void RenderOverlay2(const CRect& p_updaterect, CDCHandle p_dc) { (void)p_updaterect; (void)p_dc; }

	virtual bool FixedOverlayPresent() {return false;}

	virtual CRect GetClientRectHook() const;

	enum {
		colorText = COLOR_WINDOWTEXT,
		colorBackground = COLOR_WINDOW,
		colorHighlight = COLOR_HOTLIGHT,
		colorSelection = COLOR_HIGHLIGHT,
	};

	virtual COLORREF GetSysColorHook( int colorIndex ) const;
	
	//! Called by CListControlWithSelectionBase.
	virtual void OnItemClicked(t_size item, CPoint pt) { (void)item; (void)pt; }
	//! Called by CListControlWithSelectionBase.
	virtual void OnGroupHeaderClicked(groupID_t groupId, CPoint pt) { (void)groupId; (void)pt; }

	//! Return true to indicate that some area of the control has a special purpose and clicks there should not trigger changes in focus/selection.
	virtual bool OnClickedSpecialHitTest(CPoint pt) { (void)pt; return false; }
	virtual bool OnClickedSpecial(DWORD status, CPoint pt) { (void)status; (void)pt; return false; }

	virtual bool AllowScrollbar(bool vertical) const { (void)vertical; return true; }

	CPoint GetClientOrigin() const {return GetClientRectHook().TopLeft();}
	int GetVisibleHeight() const { return GetClientRectHook().Height(); }

	bool IsItemVisible(size_t which) const;
	//! Returns first visible item, first invisible item, begin-end style
	std::pair<size_t, size_t> GetVisibleRange() const;
	CRect GetVisibleRectAbs() const {
		return this->RectClientToAbs(GetClientRectHook());
	}

	bool IsSameItemOrHeaderAbs(const CPoint & p_point1, const CPoint & p_point2) const;

	void AddItemToUpdateRgn(HRGN p_rgn, t_size p_index) const;
	void AddGroupHeaderToUpdateRgn2(HRGN p_rgn, size_t atItem) const;

	t_size InsertIndexFromPoint(const CPoint & p_pt) const;
	//! Translate point to insert location for drag and drop. \n
	//! Made virtual so it can be specialized to allow only specific drop locations.
	virtual t_size InsertIndexFromPointEx(const CPoint & pt, bool & bInside) const;

	virtual void ListHandleResize();
	
	//! Can smooth-scroll *now* ? Used to suppress smooth scroll on temporary basis due to specific user operations in progress
	virtual bool CanSmoothScroll() const { return true; }
	//! Is smooth scroll enabled by user?
	virtual bool UserEnabledSmoothScroll() const;
	virtual bool ToggleSelectedItemsHook(pfc::bit_array const& mask) { (void)mask; return false; }


	SIZE GetDPI() const { return this->m_dpi;}

	// Should this control take enter key in dialogs or not?
	// Default to true for compatibility with existing code - but when used in dialogs, you'll want it set to false to hit [OK] with enter.
	// Note that CreateInDialog() sets this to false. Change it later if you want enter key presses to reach this control in a dialog.
	bool m_dlgWantEnter = true;

	bool WantReturn() const { return m_dlgWantEnter; }
	void SetWantReturn(bool v) { m_dlgWantEnter = v; }

	enum {
		rowStyleGrid = 0,
		rowStyleFlat,
		rowStylePlaylist,
		rowStylePlaylistDelimited,


		rowStyleDefault = rowStylePlaylistDelimited
	};
	void SetPlaylistStyle() {SetRowStyle(rowStylePlaylist);}
	void SetRowStyle(unsigned v) { if (m_rowStyle == v) return; this->m_rowStyle = v; if (m_hWnd) Invalidate(); }
	void SetFlatStyle() {SetRowStyle(rowStyleFlat);}
	unsigned m_rowStyle = rowStyleDefault;
	bool DelimitColumns() const { return m_rowStyle == rowStyleGrid || m_rowStyle == rowStylePlaylistDelimited; }

	static COLORREF BlendGridColor( COLORREF bk, COLORREF tx );
	static COLORREF BlendGridColor( COLORREF bk );
	COLORREF GridColor();

	void SetDarkMode(bool bDark);
	virtual void RefreshDarkMode();
	bool GetDarkMode() const { return m_darkMode; }
private:
	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	LRESULT OnSetDark(UINT, WPARAM, LPARAM);
	int HandleWheel(int & p_accum,int p_delta, bool bHoriz);

	void PaintContent(CRect rcPaint, HDC dc);
	void OnPrintClient(HDC dc, UINT uFlags);
	void OnPaint(CDCHandle);
	LRESULT OnVScroll(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnHScroll(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnSize(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnSizeAsync(UINT,WPARAM,LPARAM,BOOL&) {ListHandleResize();return 0;}
	LRESULT OnVWheel(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnHWheel(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnGesture(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT SetFocusPassThru(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnCreatePassThru(UINT,WPARAM,LPARAM,BOOL&);
	BOOL OnEraseBkgnd(CDCHandle);
	LRESULT OnGetDlgCode(UINT, WPARAM, LPARAM);

	void OnThemeChanged();
	int GetScrollThumbPos(int which);
	void RefreshSliders();
	void RefreshSlider(bool p_vertical);

	void OnSizeAsync_Trigger();
	static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
	bool MouseWheelFromHook(UINT msg, LPARAM data);

	bool m_suppressMouseWheel = false;
	int m_wheelAccumX = 0, m_wheelAccumY = 0;
	CPoint m_viewOrigin = CPoint(0,0);
	bool m_sizeAsyncPending = false;
	CPoint m_gesturePoint;
	
	// Prepares group header & variable height item data for view with the specified origin and current client size
	// Returns true if layout cache has changed, false otherwise.
	// Updates ptOrigin if necessary
	bool PrepLayoutCache(CPoint & ptOrigin, size_t indexLo = SIZE_MAX, size_t indexHi = SIZE_MAX);
	std::map<size_t, int> m_varItemHeights;
	std::set<size_t> m_groupHeaders;
	
	size_t FindGroupBaseCached(size_t itemFor) const;
	size_t FindGroupBase(size_t itemFor) const;
	size_t FindGroupBase(size_t itemFor, groupID_t group) const;

protected:
	// Grouped layout mode toggle
	// Default mode is greedy, probes whole list layout in advance (no glitches when scrolling)
	// In special conditions when probing is expensive, greedy mode should be turned off
	bool m_greedyGroupLayout = true;

	pfc::map_t<pfc::string8, CTheme, pfc::comparator_strcmp> m_themeCache;
	CTheme & themeFor( const char * what );
	CTheme & theme() { return themeFor("LISTVIEW");}

	const SIZE m_dpi = QueryScreenDPIEx();
	CGestureAPI m_gestureAPI;
	bool m_ensureVisibleUser = false;

	void defer( std::function<void () > f );
	LRESULT OnExecDeferred(UINT, WPARAM, LPARAM);

	// Overlays our stuff on top of generic DoDragDrop call.
	// Currently catches mouse wheel messages in mid-drag&drop and handles them in our view.
	HRESULT DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource, DWORD dwOKEffects, LPDWORD pdwEffect);

	bool paintInProgress() const { return m_paintInProgress; }
private:
	bool m_defferredMsgPending = false;
	std::list<std::function<void ()> > m_deferred;
	bool m_darkMode = false;

	bool m_paintInProgress = false;
};

class CListControlFontOps : public CListControlImpl {
private:
	typedef CListControlImpl TParent;
public:
	CListControlFontOps();
	BEGIN_MSG_MAP_EX(CListControlFontOps)
		MESSAGE_HANDLER(WM_GETFONT,OnGetFont);
		MESSAGE_HANDLER(WM_SETFONT,OnSetFont);
		CHAIN_MSG_MAP(TParent);
	END_MSG_MAP()
	CFontHandle GetFont() const { return m_font; }
	void SetFont(HFONT font, bool bUpdateView = true);
protected:
	CFontHandle GetGroupHeaderFont() const {return (HFONT)m_groupHeaderFont;}
	virtual double GroupHeaderFontScale() const { return 1.25; }
	virtual int GroupHeaderFontWeight(int origVal) const {
		//return pfc::min_t<int>(FW_BLACK, origVal + 200);
		return origVal; 
	}

	//! Overridden implementations should always forward the call to the base class.
	virtual void OnSetFont(bool bUpdatingView) { (void)bUpdatingView; }

	int GetGroupHeaderHeight() const override {return m_groupHeaderHeight;}
	int GetItemHeight() const override {return m_itemHeight;}
	
private:
	LRESULT OnSetFont(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnGetFont(UINT,WPARAM,LPARAM,BOOL&);
	void UpdateGroupHeaderFont();
	void CalculateHeights();
	int m_itemHeight, m_groupHeaderHeight;
	CFontHandle m_font;
	CFont m_groupHeaderFont;
};

#include "CListControlHeaderImpl.h"
#include "CListControlTruncationTooltipImpl.h"



typedef CMiddleDragImpl<CListControlTruncationTooltipImpl> CListControl;

