#include "shared.h"

static DWORD g_main_thread = GetCurrentThreadId();

static t_modal_dialog_entry g_status = {0,false};

static bool TestMainThread()
{
	if (GetCurrentThreadId() == g_main_thread) return true;
	OutputDebugString(TEXT("This function can be called only from main thread.\n"));
	return false;
}

HWND SHARED_EXPORT FindOwningPopup(HWND p_wnd)
{
	return pfc::findOwningPopup(p_wnd);
}

void SHARED_EXPORT PokeWindow(HWND p_wnd)
{
	p_wnd = FindOwningPopup(p_wnd);
	if (IsWindowEnabled(p_wnd))
	{
//		SetForegroundWindow(p_wnd);
		SetActiveWindow(p_wnd);
		FlashWindow(p_wnd,FALSE);
	}
	else
	{
		HWND child = GetWindow(p_wnd,GW_ENABLEDPOPUP);
		if (child != 0)
		{
//			SetForegroundWindow(child);
			SetActiveWindow(child);
			FlashWindow(child,FALSE);
		}
	}
}

extern "C" {
	void SHARED_EXPORT ModalDialog_Switch(t_modal_dialog_entry & p_entry)
	{
		if (TestMainThread())
			pfc::swap_t(p_entry,g_status);
	}

	void SHARED_EXPORT ModalDialog_PokeExisting()
	{
		if (TestMainThread())
		{
			if (g_status.m_in_use && g_status.m_wnd_to_poke != 0)
			{
				PokeWindow(g_status.m_wnd_to_poke);
				MessageBeep(0);
			}
		}
	}

	bool SHARED_EXPORT ModalDialog_CanCreateNew()
	{
		if (TestMainThread())
			return !g_status.m_in_use;
		else
			return false;
	}
}