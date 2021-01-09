#pragma once

#include "win32_op.h"
#include "wtl-pp.h"

class CPopupTooltipMessage {
public:
	CPopupTooltipMessage(DWORD style = TTS_BALLOON | TTS_NOPREFIX) : m_style(style | WS_POPUP), m_toolinfo(), m_shutDown() {}
	void ShowFocus(const TCHAR * message, CWindow wndParent) {
		Show(message, wndParent); wndParent.SetFocus();
	}
	void Show(const TCHAR * message, CWindow wndParent) {
		if (m_shutDown || (message == NULL && m_tooltip.m_hWnd == NULL)) return;
		Initialize();
		Hide();

		if (message != NULL) {
			CRect rect;
			WIN32_OP_D(wndParent.GetWindowRect(rect));
			ShowInternal(message, wndParent, rect);
		}
	}
	void ShowEx(const TCHAR * message, CWindow wndParent, CRect rect) {
		if (m_shutDown) return;
		Initialize();
		Hide();
		ShowInternal(message, wndParent, rect);
	}
	void Hide() {
		if (m_tooltip.m_hWnd != NULL && m_tooltip.GetToolCount() > 0) {
			m_tooltip.TrackActivate(&m_toolinfo, FALSE);
			m_tooltip.DelTool(&m_toolinfo);
		}
	}

	void CleanUp() {
		if (m_tooltip.m_hWnd != NULL) {
			m_tooltip.DestroyWindow();
		}
	}
	void ShutDown() {
		m_shutDown = true; CleanUp();
	}
private:
	void ShowInternal(const TCHAR * message, CWindow wndParent, CRect rect) {

		PFC_ASSERT(!m_shutDown);
		PFC_ASSERT(message != NULL);
		PFC_ASSERT(wndParent != NULL);

		if (_tcschr(message, '\n') != nullptr) {
			m_tooltip.SetMaxTipWidth(rect.Width());
		}
		m_toolinfo.cbSize = sizeof(m_toolinfo);
		m_toolinfo.uFlags = TTF_TRACK | TTF_IDISHWND | TTF_ABSOLUTE | TTF_TRANSPARENT | TTF_CENTERTIP;
		m_toolinfo.hwnd = wndParent;
		m_toolinfo.uId = 0;
		m_toolinfo.lpszText = const_cast<TCHAR*>(message);
		m_toolinfo.hinst = NULL; //core_api::get_my_instance();
		if (m_tooltip.AddTool(&m_toolinfo)) {
			m_tooltip.TrackPosition(rect.CenterPoint().x, rect.bottom);
			m_tooltip.TrackActivate(&m_toolinfo, TRUE);
		}
	}
	void Initialize() {
		if (m_tooltip.m_hWnd == NULL) {
			WIN32_OP(m_tooltip.Create(NULL, NULL, NULL, m_style));
		}
	}
	CContainedWindowSimpleT<CToolTipCtrl> m_tooltip;
	TOOLINFO m_toolinfo;
	const DWORD m_style;
	bool m_shutDown;
};


template<typename T> class CDialogWithTooltip : public CDialogImpl<T> {
public:
	BEGIN_MSG_MAP_EX(CDialogWithTooltip)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

	void ShowTip(UINT id, const TCHAR * label) {
		m_tip.Show(label, this->GetDlgItem(id));
	}
	void ShowTip(HWND child, const TCHAR * label) {
		m_tip.Show(label, child);
	}

	void ShowTipF(UINT id, const TCHAR * label) {
		m_tip.ShowFocus(label, this->GetDlgItem(id));
	}
	void ShowTipF(HWND child, const TCHAR * label) {
		m_tip.ShowFocus(label, child);
	}
	void HideTip() { m_tip.Hide(); }
private:
	void OnDestroy() { m_tip.ShutDown(); this->SetMsgHandled(FALSE); }
	CPopupTooltipMessage m_tip;
};

