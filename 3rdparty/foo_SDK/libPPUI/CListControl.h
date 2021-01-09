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
	CListControlImpl() {}

	DECLARE_WND_CLASS_EX(TEXT("{4B94B650-C2D8-40de-A0AD-E8FADF62D56C}"),CS_DBLCLKS,COLOR_WINDOW);
	
	// Wrapper around CWindowImpl::Create().
	// Creates CListControl replacing another dialog control with the specified ID.
	// Note that m_dlgWantEnter is set to false by this method, as it's typically unwanted in dialogs.
	void CreateInDialog( CWindow wndDialog, UINT replaceControlID );

	enum {
		MSG_SIZE_ASYNC = WM_USER + 13,
		MSG_EXEC_DEFERRED,
		UserMsgBase
	};

	BEGIN_MSG_MAP_EX(CListControlImpl)
		MESSAGE_RANGE_HANDLER_EX(WM_MOUSEFIRST, WM_MOUSELAST, MousePassThru);
		MESSAGE_HANDLER_EX(MSG_EXEC_DEFERRED, OnExecDeferred);
		MESSAGE_HANDLER(WM_PAINT,OnPaint);
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
		MESSAGE_HANDLER(WM_ERASEBKGND,OnEraseBkgnd);
		MESSAGE_HANDLER(MSG_SIZE_ASYNC,OnSizeAsync);
		MESSAGE_HANDLER(WM_GESTURE, OnGesture)
		MSG_WM_THEMECHANGED(OnThemeChanged)
		MSG_WM_KILLFOCUS(OnKillFocus)
		MSG_WM_WINDOWPOSCHANGED(OnWindowPosChanged)
		MESSAGE_HANDLER_EX( WM_GETDLGCODE, OnGetDlgCode )
	END_MSG_MAP()

	virtual void ReloadData() { OnViewAreaChanged(); }
	virtual void ReloadItems( pfc::bit_array const & mask ) { UpdateItems( mask); }

	//! Hookable function called in response to reordering of items. Redraws the view and updates internal data to reflect the change.
	virtual void OnItemsReordered( const size_t * order, size_t count );
	//! Hookable function called in response to removal of items. Redraws the view and updates internal data to reflect the change.
	virtual void OnItemsRemoved( pfc::bit_array const & mask, size_t oldCount ) { ReloadData(); }
	//! Hookable function called in response to insertion of items. Redraws the view and updates internal data to reflect the change.
	virtual void OnItemsInserted( size_t at, size_t count, bool bSelect ) { ReloadData(); }

	void ReloadItem(size_t i) { ReloadItems( pfc::bit_array_one(i) ); }
	void OnViewAreaChanged() {OnViewAreaChanged(GetViewOrigin());}
	void OnViewAreaChanged(CPoint p_originOverride);
	void UpdateGroupHeader(int p_id);
	void UpdateItems(const pfc::bit_array & p_mask);
	void UpdateItemsAndHeaders(const pfc::bit_array & p_mask);
	void UpdateItem(t_size p_item) {UpdateItems(pfc::bit_array_one(p_item));}
	void UpdateItemsAll() {Invalidate();}
	void EnsureItemVisible(t_size p_item, bool bUser = false);
	void EnsureHeaderVisible(int p_group);
	virtual void EnsureVisibleRectAbs(const CRect & p_rect);
	CRect GetItemRect(t_size p_item) const;
	bool GetGroupHeaderRect(int p_group,CRect & p_rect) const;
	CRect GetItemRectAbs(t_size p_item) const;
	bool GetGroupHeaderRectAbs(int p_group,CRect & p_rect) const;
	CPoint GetViewOrigin() const {return m_viewOrigin;}
	CPoint GetViewOffset() const {return GetViewOrigin() - GetClientOrigin();}
	int GetViewAreaWidth() const {return GetItemWidth();}
	int GetViewAreaHeight() const;
	CRect GetViewAreaRectAbs() const;
	CRect GetViewAreaRect() const;
	CRect GetValidViewOriginArea() const;
	t_size GetGroupCount() const;
	bool GetItemRangeAbs(const CRect & p_rect,t_size & p_base,t_size & p_count) const;
	bool GetItemRangeAbsInclHeaders(const CRect & p_rect,t_size & p_base,t_size & p_count) const;
	bool GetItemRange(const CRect & p_rect,t_size & p_base,t_size & p_count) const;
	void MoveViewOriginNoClip(CPoint p_target);
	void MoveViewOrigin(CPoint p_target);
	CPoint ClipViewOrigin(CPoint p_origin) const;
	void MoveViewOriginDelta(CPoint p_delta) {MoveViewOrigin( GetViewOrigin() + p_delta );}
	void MoveViewOriginDeltaNoClip(CPoint p_delta) {MoveViewOriginNoClip( GetViewOrigin() + p_delta );}
	bool ItemFromPoint(CPoint const & p_pt,t_size & p_item) const {return ItemFromPointAbs(p_pt + GetViewOffset(),p_item);}
	bool GroupHeaderFromPoint(CPoint const & p_pt,int & p_group) const {return GroupHeaderFromPointAbs(p_pt + GetViewOffset(),p_group);}
	bool ItemFromPointAbs(CPoint const & p_pt,t_size & p_item) const;
	bool GroupHeaderFromPointAbs(CPoint const & p_pt,int & p_group) const;

	bool ResolveGroupRange(int p_id,t_size & p_base,t_size & p_count) const;

	virtual int GetGroupHeaderHeight() const {return 0;}
	virtual int GetItemHeight() const {return 0;}
	virtual int GetItemWidth() const {return 0;}
	virtual t_size GetItemCount() const {return 0;}
	virtual int GetItemGroup(t_size p_item) const {return 0;}
	//override optionally
	virtual void RenderItem(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc);
	//override optionally
	virtual void RenderGroupHeader(int p_group,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc);

	//called by default RenderItem implementation
	virtual void RenderItemText(t_size p_item,const CRect & p_itemRect,const CRect & p_updateRect,CDCHandle p_dc, bool allowColors) {}
	//called by default RenderItem implementation
	virtual void RenderGroupHeaderText(int p_group,const CRect & p_headerRect,const CRect & p_updateRect,CDCHandle p_dc) {}

	virtual void RenderItemBackground(CDCHandle p_dc,const CRect & p_itemRect,size_t item, uint32_t bkColor);
	virtual void RenderGroupHeaderBackground(CDCHandle p_dc,const CRect & p_headerRect,int iGroup);

	virtual void RenderBackground( CDCHandle dc, CRect const & rc );

	virtual void OnViewOriginChange(CPoint p_delta) {}
	virtual void RenderOverlay(const CRect & p_updaterect,CDCHandle p_dc) {}
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
	virtual void OnItemClicked(t_size item, CPoint pt) {}
	//! Called by CListControlWithSelectionBase.
	virtual void OnGroupHeaderClicked(int groupId, CPoint pt) {}

	//! Return true to indicate that some area of the control has a special purpose and clicks there should not trigger changes in focus/selection.
	virtual bool OnClickedSpecialHitTest(CPoint pt) { return false; }
	virtual bool OnClickedSpecial(DWORD status, CPoint pt) {return false;}

	virtual bool AllowScrollbar(bool vertical) const {return true;}

	CPoint GetClientOrigin() const {return GetClientRectHook().TopLeft();}
	CRect GetVisibleRectAbs() const {
		CRect view = GetClientRectHook();
		view.OffsetRect( GetViewOrigin() - view.TopLeft() );
		return view;
	}

	bool IsSameItemOrHeaderAbs(const CPoint & p_point1, const CPoint & p_point2) const;

	void AddItemToUpdateRgn(HRGN p_rgn, t_size p_index) const;
	void AddGroupHeaderToUpdateRgn(HRGN p_rgn, int id) const;

	t_size InsertIndexFromPoint(const CPoint & p_pt) const;
	//! Translate point to insert location for drag and drop. \n
	//! Made virtual so it can be specialized to allow only specific drop locations.
	virtual t_size InsertIndexFromPointEx(const CPoint & pt, bool & bInside) const;

	virtual void ListHandleResize();
	
	//! Can smooth-scroll *now* ? Used to suppress smooth scroll on temporary basis due to specific user operations in progress
	virtual bool CanSmoothScroll() const { return true; }
	//! Is smooth scroll enabled by user?
	virtual bool UserEnabledSmoothScroll() const;
	virtual bool ToggleSelectedItemsHook(pfc::bit_array const & mask) { return false; }

	void SetCaptureEx(CaptureProc_t proc);
	void SetCaptureMsgHandled(BOOL v) { this->SetMsgHandled(v); }

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
	};
	void SetPlaylistStyle() {SetRowStyle(rowStylePlaylist);}
	void SetRowStyle(unsigned v) { this->m_rowStyle = v; if (m_hWnd) Invalidate(); }
	void SetFlatStyle() {SetRowStyle(rowStyleFlat);}
	unsigned m_rowStyle = rowStylePlaylistDelimited;
	bool DelimitColumns() const { return m_rowStyle == rowStyleGrid || m_rowStyle == rowStylePlaylistDelimited; }

	static COLORREF BlendGridColor( COLORREF bk, COLORREF tx );
	static COLORREF BlendGridColor( COLORREF bk );
	COLORREF GridColor();

private:
	void RenderRect(const CRect & p_rect,CDCHandle p_dc);
	int HandleWheel(int & p_accum,int p_delta, bool bHoriz);

	void OnKillFocus(CWindow);
	void OnWindowPosChanged(LPWINDOWPOS);
	void PaintContent(CRect rcPaint, HDC dc);
	void OnPrintClient(HDC dc, UINT uFlags);
	LRESULT OnPaint(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnVScroll(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnHScroll(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnSize(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnSizeAsync(UINT,WPARAM,LPARAM,BOOL&) {ListHandleResize();return 0;}
	LRESULT OnVWheel(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnHWheel(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnGesture(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT SetFocusPassThru(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnCreatePassThru(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnEraseBkgnd(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT MousePassThru(UINT, WPARAM, LPARAM);
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

protected:
	pfc::map_t<pfc::string8, CTheme, pfc::comparator_strcmp> m_themeCache;
	CTheme & themeFor( const char * what );
	CTheme & theme() { return themeFor("LISTVIEW");}

	const SIZE m_dpi = QueryScreenDPIEx();
	CGestureAPI m_gestureAPI;
	bool m_ensureVisibleUser = false;
	CaptureProc_t m_captureProc;
	void defer( std::function<void () > f );
	LRESULT OnExecDeferred(UINT, WPARAM, LPARAM);


	// Overlays our stuff on top of generic DoDragDrop call.
	// Currently catches mouse wheel messages in mid-drag&drop and handles them in our view.
	HRESULT DoDragDrop(LPDATAOBJECT pDataObj, LPDROPSOURCE pDropSource, DWORD dwOKEffects, LPDWORD pdwEffect);

private:
	bool m_defferredMsgPending = false;
	std::list<std::function<void ()> > m_deferred;
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
	virtual void OnSetFont(bool bUpdatingView) {}

	int GetGroupHeaderHeight() const {return m_groupHeaderHeight;}
	int GetItemHeight() const {return m_itemHeight;}
	
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

