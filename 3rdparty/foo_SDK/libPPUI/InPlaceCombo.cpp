#include "stdafx.h"

#include "InPlaceEdit.h"
#include "wtl-pp.h"
#include "win32_op.h"

#include "AutoComplete.h"
#include "CWindowCreateAndDelete.h"
#include "win32_utility.h"
#include "listview_helper.h" // ListView_GetColumnCount
#include "clipboard.h"

#include <forward_list>

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x20E
#endif

using namespace InPlaceEdit;


namespace {

	enum {
		MSG_COMPLETION = WM_USER,
		MSG_DISABLE_EDITING
	};


	// Rationale: more than one HWND on the list is extremely uncommon, hence forward_list
	static std::forward_list<HWND> g_editboxes;
	static HHOOK g_hook = NULL /*, g_keyHook = NULL*/;

	static void GAbortEditing(HWND edit, t_uint32 code) {
		CWindow parent = ::GetParent(edit);
		parent.SendMessage(MSG_DISABLE_EDITING);
		parent.PostMessage(MSG_COMPLETION, code, 0);
	}

	static void GAbortEditing(t_uint32 code) {
		for (auto walk = g_editboxes.begin(); walk != g_editboxes.end(); ++walk) {
			GAbortEditing(*walk, code);
		}
	}

	static bool IsSamePopup(CWindow wnd1, CWindow wnd2) {
		return pfc::findOwningPopup(wnd1) == pfc::findOwningPopup(wnd2);
	}

	static void MouseEventTest(HWND target, CPoint pt, bool isWheel) {
		for (auto walk = g_editboxes.begin(); walk != g_editboxes.end(); ++walk) {
			CWindow edit(*walk);
			bool cancel = false;
			if (target != edit && IsSamePopup(target, edit)) {
				cancel = true;
			} else if (isWheel) {
				CWindow target2 = WindowFromPoint(pt);
				if (target2 != edit && IsSamePopup(target2, edit)) {
					cancel = true;
				}
			}

			if (cancel) GAbortEditing(edit, KEditLostFocus);
		}
	}

	static LRESULT CALLBACK GMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
		if (nCode == HC_ACTION) {
			const MOUSEHOOKSTRUCT * mhs = reinterpret_cast<const MOUSEHOOKSTRUCT *>(lParam);
			switch (wParam) {
			case WM_NCLBUTTONDOWN:
			case WM_NCRBUTTONDOWN:
			case WM_NCMBUTTONDOWN:
			case WM_NCXBUTTONDOWN:
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_XBUTTONDOWN:
				MouseEventTest(mhs->hwnd, mhs->pt, false);
				break;
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
				MouseEventTest(mhs->hwnd, mhs->pt, true);
				break;
			}
		}
		return CallNextHookEx(g_hook, nCode, wParam, lParam);
	}

	static void on_editbox_creation(HWND p_editbox) {
		// PFC_ASSERT(core_api::is_main_thread());
		g_editboxes.push_front(p_editbox);
		if (g_hook == NULL) {
			g_hook = SetWindowsHookEx(WH_MOUSE, GMouseProc, NULL, GetCurrentThreadId());
		}
		/*if (g_keyHook == NULL) {
			g_keyHook = SetWindowsHookEx(WH_KEYBOARD, GKeyboardProc, NULL, GetCurrentThreadId());
		}*/
	}
	static void UnhookHelper(HHOOK & hook) {
		HHOOK v = pfc::replace_null_t(hook);
		if (v != NULL) UnhookWindowsHookEx(v);
	}
	static void on_editbox_destruction(HWND p_editbox) {
		// PFC_ASSERT(core_api::is_main_thread());
		g_editboxes.remove(p_editbox);
		if (g_editboxes.empty()) {
			UnhookHelper(g_hook); /*UnhookHelper(g_keyHook);*/
		}
	}

	class CInPlaceComboBox : public CContainedWindowSimpleT<CComboBox> {
	public:
		BEGIN_MSG_MAP_EX(CInPlaceComboBox)
			//MSG_WM_CREATE(OnCreate)
			MSG_WM_DESTROY(OnDestroy)
			MSG_WM_GETDLGCODE(OnGetDlgCode)
			// MSG_WM_KILLFOCUS(OnKillFocus)
			MSG_WM_KEYDOWN(OnKeyDown)
		END_MSG_MAP()

		void OnCreation() {
			on_editbox_creation(m_hWnd);
		}
	private:
		void OnDestroy() {
			m_selfDestruct = true;
			on_editbox_destruction(m_hWnd);
			SetMsgHandled(FALSE);
		}
		int OnCreate(LPCREATESTRUCT lpCreateStruct) {
			OnCreation();
			SetMsgHandled(FALSE);
			return 0;
		}
		UINT OnGetDlgCode(LPMSG lpMsg) {
			if (lpMsg == NULL) {
				SetMsgHandled(FALSE); return 0;
			} else {
				switch (lpMsg->message) {
				case WM_KEYDOWN:
				case WM_SYSKEYDOWN:
					switch (lpMsg->wParam) {
					case VK_TAB:
					case VK_ESCAPE:
					case VK_RETURN:
						return DLGC_WANTMESSAGE;
					default:
						SetMsgHandled(FALSE); return 0;
					}
				default:
					SetMsgHandled(FALSE); return 0;

				}
			}
		}
		void OnKillFocus(CWindow wndFocus) {
			if ( wndFocus != NULL ) ForwardCompletion(KEditLostFocus);
			SetMsgHandled(FALSE);
		}

		void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
			m_suppressChar = nFlags & 0xFF;
			switch (nChar) {
			case VK_TAB:
				ForwardCompletion(IsKeyPressed(VK_SHIFT) ? KEditShiftTab : KEditTab);
				return;
			case VK_ESCAPE:
				ForwardCompletion(KEditAborted);
				return;
			}
			m_suppressChar = 0;
			SetMsgHandled(FALSE);
		}

		void ForwardCompletion(t_uint32 code) {
			if (IsWindowEnabled()) {
				CWindow owner = GetParent();
				owner.SendMessage(MSG_DISABLE_EDITING);
				owner.PostMessage(MSG_COMPLETION, code, 0);
				EnableWindow(FALSE);
			}
		}

		bool m_selfDestruct = false;
		UINT m_suppressChar = 0;
	};

	class InPlaceComboContainer : public CWindowImpl<InPlaceComboContainer> {
	public:
		DECLARE_WND_CLASS_EX(_T("{18D85006-0CDB-49AB-A563-6A42014309A3}"), 0, -1);

		const pfc::string_list_const * m_initData;
		const unsigned m_iDefault;

		HWND Create(CWindow parent) {

			RECT rect_cropped;
			{
				RECT client;
				WIN32_OP_D(parent.GetClientRect(&client));
				IntersectRect(&rect_cropped, &client, &m_initRect);
			}
			const DWORD containerStyle = WS_BORDER | WS_CHILD;
			AdjustWindowRect(&rect_cropped, containerStyle, FALSE);



			WIN32_OP(__super::Create(parent, rect_cropped, NULL, containerStyle) != NULL);

			try {
				CRect rcClient;
				WIN32_OP_D(GetClientRect(rcClient));


				DWORD style = WS_CHILD | WS_VISIBLE;//parent is invisible now

				style |= CBS_DROPDOWNLIST;


				CComboBox edit;

				WIN32_OP(edit.Create(*this, rcClient, NULL, style, 0, ID_MYEDIT) != NULL);
				edit.SetFont(parent.GetFont());

				m_edit.SubclassWindow(edit);
				m_edit.OnCreation();


#if 0 // doesn't quite work
				if (m_flags & (KFlagAlignCenter | KFlagAlignRight)) {
					COMBOBOXINFO info = {sizeof(info)};
					if (m_edit.GetComboBoxInfo(&info)) {
						CEdit edit2 = info.hwndList;
						if (edit2) {
							if (m_flags & KFlagAlignCenter) edit2.ModifyStyle(0, ES_CENTER);
							else if (m_flags & KFlagAlignRight) edit2.ModifyStyle(0, ES_RIGHT);
						}
					}
				}
#endif


				if (m_initData != nullptr) {
					const size_t count = m_initData->get_count();
					for (size_t walk = 0; walk < count; ++walk) {
						m_edit.AddString(pfc::stringcvt::string_os_from_utf8(m_initData->get_item(walk)));
					}
					if (m_iDefault < count) m_edit.SetCurSel(m_iDefault);
				}
			} catch (...) {
				PostMessage(MSG_COMPLETION, InPlaceEdit::KEditAborted, 0);
				return m_hWnd;
			}

			ShowWindow(SW_SHOW);
			m_edit.SetFocus();

			m_initialized = true;

			m_edit.ShowDropDown();

			PFC_ASSERT(m_hWnd != NULL);

			return m_hWnd;
		}

		InPlaceComboContainer(const RECT & p_rect, unsigned p_flags, pfc::string_list_const * initData, unsigned iDefault, comboReply_t p_notify) : m_notify(p_notify), m_initData(initData), m_iDefault(iDefault), m_initRect(p_rect), m_flags(p_flags) { }

		enum { ID_MYEDIT = 666 };

		BEGIN_MSG_MAP_EX(InPlaceEditContainer)
			MESSAGE_HANDLER_EX(WM_CTLCOLOREDIT, MsgForwardToParent)
			MESSAGE_HANDLER_EX(WM_CTLCOLORSTATIC, MsgForwardToParent)
			MESSAGE_HANDLER_EX(WM_MOUSEWHEEL, MsgLostFocus)
			MESSAGE_HANDLER_EX(WM_MOUSEHWHEEL, MsgLostFocus)
			MESSAGE_HANDLER_SIMPLE(MSG_DISABLE_EDITING, OnMsgDisableEditing)
			MESSAGE_HANDLER_EX(MSG_COMPLETION, OnMsgCompletion)
			COMMAND_ID_HANDLER_EX(ID_MYEDIT, OnComboMsg)
			MSG_WM_DESTROY(OnDestroy)
		END_MSG_MAP()

		HWND GetEditBox() const { return m_edit; }

	private:
		void OnDestroy() { m_selfDestruct = true; }

		LRESULT MsgForwardToParent(UINT msg, WPARAM wParam, LPARAM lParam) {
			return GetParent().SendMessage(msg, wParam, lParam);
		}
		LRESULT MsgLostFocus(UINT, WPARAM, LPARAM) {
			PostMessage(MSG_COMPLETION, InPlaceEdit::KEditLostFocus, 0);
			return 0;
		}
		void OnMsgDisableEditing() {
			ShowWindow(SW_HIDE);
			GetParent().UpdateWindow();
		}
		LRESULT OnMsgCompletion(UINT, WPARAM wParam, LPARAM lParam) {
			PFC_ASSERT(m_initialized);
			if ((wParam & KEditMaskReason) != KEditLostFocus) {
				GetParent().SetFocus();
			}
			OnCompletion((unsigned) wParam );
			if (!m_selfDestruct) {
				m_selfDestruct = true;
				DestroyWindow();
			}
			return 0;
		}
		void OnComboMsg(UINT code, int, CWindow source) {
			if (m_initialized && (code == CBN_SELENDOK || code == CBN_SELENDCANCEL) ) {
				PostMessage(MSG_COMPLETION, InPlaceEdit::KEditLostFocus, 0);
			}
		}

	private:

		void OnCompletion(unsigned p_status) {
			if (!m_completed) {
				m_completed = true;
				if (m_notify) m_notify( p_status, m_edit.GetCurSel() );
			}
		}

		const comboReply_t m_notify;
		bool m_completed = false;
		bool m_initialized = false;
		bool m_selfDestruct = false;
		const CRect m_initRect;
		CInPlaceComboBox m_edit;
		const unsigned m_flags;
	};

}

static void fail(comboReply_t p_notify) {
	p_notify(KEditAborted, UINT_MAX);
}

HWND InPlaceEdit::StartCombo(HWND p_parentwnd, const RECT & p_rect, unsigned p_flags, pfc::string_list_const & data, unsigned iDefault, comboReply_t p_notify) {
	try {
		PFC_ASSERT((CWindow(p_parentwnd).GetWindowLong(GWL_STYLE) & WS_CLIPCHILDREN) != 0);
		return (new CWindowCreateAndDelete<InPlaceComboContainer>(p_parentwnd, p_rect, p_flags, &data, iDefault, p_notify))->GetEditBox();
	} catch (...) {
		fail(p_notify);
		return NULL;
	}
}
