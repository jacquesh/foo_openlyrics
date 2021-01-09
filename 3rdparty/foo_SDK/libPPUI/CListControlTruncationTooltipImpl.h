#pragma once

class CListControlTruncationTooltipImpl : public CListControlHeaderImpl {
private:
	typedef CListControlHeaderImpl TParent;
public:
	CListControlTruncationTooltipImpl();

	BEGIN_MSG_MAP_EX(CListControlTruncationTooltipImpl)
		MESSAGE_HANDLER(WM_MOUSEHOVER,OnHover);
		MESSAGE_HANDLER(WM_MOUSEMOVE,OnMouseMovePassThru);
		MESSAGE_HANDLER(WM_TIMER,OnTimer);
		MESSAGE_HANDLER(WM_DESTROY,OnDestroyPassThru);
		CHAIN_MSG_MAP(TParent)
		NOTIFY_CODE_HANDLER(TTN_GETDISPINFO,OnTTGetDispInfo);
		NOTIFY_CODE_HANDLER(TTN_POP,OnTTPop);
		NOTIFY_CODE_HANDLER(TTN_SHOW,OnTTShow);
	END_MSG_MAP()

	void OnViewOriginChange(CPoint p_delta) {TParent::OnViewOriginChange(p_delta);TooltipRemove();}
	void TooltipRemove(); 
protected:
	virtual bool GetTooltipData( CPoint ptAbs, pfc::string_base & text, CRect & rc, CFontHandle & font) const;
private:
	enum {
		KTooltipTimer = 0x51dbee9e,
		KTooltipTimerDelay = 50,
	};
	LRESULT OnHover(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnMouseMovePassThru(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnTimer(UINT,WPARAM,LPARAM,BOOL&);
	LRESULT OnTTGetDispInfo(int,LPNMHDR,BOOL&);
	LRESULT OnTTShow(int,LPNMHDR,BOOL&);
	LRESULT OnTTPop(int,LPNMHDR,BOOL&);
	LRESULT OnDestroyPassThru(UINT,WPARAM,LPARAM,BOOL&);

	void InitTooltip();
	void TooltipActivateAbs(const char * label, const CRect & rect);
	void TooltipActivate(const char * label, const CRect & rect);
	void TooltipRemoveCheck(LPARAM pos);
	void TooltipRemoveCheck();
	void TooltipUpdateFont();
	void OnSetFont(bool) {TooltipUpdateFont();}
	bool IsRectFullyVisibleAbs(CRect const & r);
	bool IsRectPartiallyObscuredAbs(CRect const & r) const;
	CRect m_tooltipRect;
	CToolTipCtrl m_tooltip;
	TOOLINFO m_toolinfo;
	pfc::stringcvt::string_os_from_utf8 m_tooltipText;
	CFontHandle m_tooltipFont;
};
