#include "stdafx.h"
#include "CEditWithButtons.h"


void CEditWithButtons::AddMoreButton(std::function<void()> f) {
	AddButton(L"more", f, nullptr, L"\x2026");
}
void CEditWithButtons::AddClearButton(const wchar_t * clearVal, bool bHandleEsc) {
	std::wstring clearValCopy(clearVal);
	auto handler = [this, clearValCopy] {
		this->SetWindowText(clearValCopy.c_str());
	};
	auto condition = [clearValCopy](const wchar_t * txt) -> bool {
		return clearValCopy != txt;
	};
	// Present "clear" to accessibility APIs but actually draw a multiplication x sign
	AddButton(L"clear", handler, condition, L"\x00D7");

	if (bHandleEsc) {
		this->onEscKey = handler;
	}
	
}

void CEditWithButtons::AddButton(const wchar_t * str, handler_t handler, condition_t condition, const wchar_t * drawAlternateText) {
	PFC_ASSERT(GetStyle() & WS_CLIPCHILDREN);
	Button_t btn;
	btn.handler = handler;
	btn.title = str;
	btn.condition = condition;
	btn.visible = EvalCondition(btn, nullptr);

	if (drawAlternateText != nullptr) {
		btn.titleDraw = drawAlternateText;
	}

	m_buttons.push_back(std::move(btn));
	RefreshButtons();
}

CRect CEditWithButtons::RectOfButton(const wchar_t * text) {
	for (auto i = m_buttons.begin(); i != m_buttons.end(); ++i) {
		if (i->title == text && i->wnd != NULL) {
			CRect rc;
			if (i->wnd.GetWindowRect(rc)) return rc;
		}
	}
	return CRect();
}

void CEditWithButtons::TabCycleButtons(HWND wnd) {
	for (auto i = m_buttons.begin(); i != m_buttons.end(); ++i) {
		if (i->wnd == wnd) {
			if (IsShiftPressed()) {
				// back
				for (;; ) {
					if (i == m_buttons.begin()) {
						TabFocusThis(m_hWnd); break;
					} else {
						--i;
						if (i->visible) {
							TabFocusThis(i->wnd);
							break;
						}
					}
				}
			} else {
				// forward
				for (;; ) {
					++i;
					if (i == m_buttons.end()) {
						TabFocusThis(m_hWnd);
						TabFocusPrevNext(false);
						break;
					} else {
						if (i->visible) {
							TabFocusThis(i->wnd);
							break;
						}
					}
				}
			}

			return;
		}
	}
}


bool CEditWithButtons::ButtonWantTab(HWND wnd) {
	if (IsShiftPressed()) return true;
	if (m_buttons.size() == 0) return false; // should not be possible
	auto last = m_buttons.rbegin();
	if (wnd == last->wnd) return false; // not for last button
	return true;
}
bool CEditWithButtons::EvalCondition(Button_t & btn, const wchar_t * newText) {
	if (!btn.condition) return true;
	if (newText != nullptr) return btn.condition(newText);
	TCHAR text[256] = {};
	GetWindowText(text, 256);
	text[255] = 0;
	return btn.condition(text);
}
void CEditWithButtons::RefreshConditions(const wchar_t * newText) {
	bool changed = false;
	for (auto i = m_buttons.begin(); i != m_buttons.end(); ++i) {
		bool status = EvalCondition(*i, newText);
		if (status != i->visible) {
			i->visible = status; changed = true;
		}
	}
	if (changed) {
		Layout();
	}
}

void CEditWithButtons::Layout(CSize size, CFontHandle fontSetMe) {
	if (m_buttons.size() == 0) return;

	int walk = size.cx;

	HDWP dwp = BeginDeferWindowPos((int)m_buttons.size());
	for (auto iter = m_buttons.rbegin(); iter != m_buttons.rend(); ++iter) {
		if (!iter->visible) {
			if (::GetFocus() == iter->wnd) {
				this->SetFocus();
			}
			::DeferWindowPos(dwp, iter->wnd, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOCOPYBITS);
			continue;
		}

		if (iter->wnd == NULL) {
			auto* b = &iter->wnd;
			b->Create(*this, NULL, iter->title.c_str());
			if (iter->titleDraw.length() > 0) b->DrawAlternateText(iter->titleDraw.c_str());
			CFontHandle font = fontSetMe;
			if (font == NULL) font = GetFont();
			b->SetFont(font);
			b->ClickHandler = iter->handler;
			b->CtlColorHandler = [=](CDCHandle dc) -> HBRUSH {
				return this->OnColorBtn(dc, NULL);
			};
			b->TabCycleHandler = [=](HWND wnd) {
				TabCycleButtons(wnd);
			};
			b->WantTabCheck = [=](HWND wnd) -> bool {
				return ButtonWantTab(wnd);
			};
			if (!IsWindowEnabled()) b->EnableWindow(FALSE);
		} else if (fontSetMe) {
			iter->wnd.SetFont(fontSetMe);
		}

		unsigned delta = MeasureButton(*iter);
		int left = walk - delta;

		if (iter->wnd != NULL) {
			CRect rc;
			rc.top = 0;
			rc.bottom = size.cy;
			rc.left = left;
			rc.right = walk;
			::DeferWindowPos(dwp, iter->wnd, NULL, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOCOPYBITS);
		}

		walk = left;
	}
	EndDeferWindowPos(dwp);
	this->SetMargins(0, size.cx - walk, EC_RIGHTMARGIN);
}

unsigned CEditWithButtons::MeasureButton(Button_t const & button) {
	if (m_fixedWidth != 0) return m_fixedWidth;

	return button.wnd.Measure();
}
