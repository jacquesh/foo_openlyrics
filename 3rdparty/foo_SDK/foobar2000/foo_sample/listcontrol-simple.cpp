#include "stdafx.h"

// Simple CListControl use demo
// CListControlSimple

#include "stdafx.h"
#include "resource.h"
#include <helpers/atl-misc.h>
#include <libPPUI/CListControlSimple.h>
#include <string>
#include <algorithm>
#include <vector>

namespace {
	struct listData_t {
		std::string m_key, m_value;
	};
	static std::vector<listData_t> makeListData() {
		std::vector<listData_t> data;
		data.resize( 10 );
		for( size_t walk = 0; walk < data.size(); ++ walk ) {
			auto & rec = data[walk];
			rec.m_key = (PFC_string_formatter() << "Item #" << (walk+1) ).c_str();
			rec.m_value = "sample value";
		}
		return data;
	}
	

	class CListControlSimpleDemoDialog : public CDialogImpl<CListControlSimpleDemoDialog> {
	public:

		enum { IDD = IDD_LISTCONTROL_DEMO };

		BEGIN_MSG_MAP_EX(CListControlOwnerDataDemoDialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
			MSG_WM_CONTEXTMENU(OnContextMenu)
		END_MSG_MAP()
	private:
		void OnCancel(UINT, int, CWindow) {
			DestroyWindow();
		}

		BOOL OnInitDialog(CWindow, LPARAM) {

			// Create replacing existing windows list control
			// automatically initialize position, font, etc
			m_list.CreateInDialog( *this, IDC_LIST1 );

			// never hardcode values in pixels, always use screen DPI
			auto DPI = m_list.GetDPI();
			m_list.AddColumn( "Name", MulDiv(100, DPI.cx, 96 ) );
			m_list.AddColumn( "Value", MulDiv(150, DPI.cx, 96 ) );

			{
				auto data = makeListData();

				m_list.SetItemCount( data.size( ) );
				for( size_t walk = 0; walk < data.size(); ++ walk ) {
					auto & rec = data[walk];
					m_list.SetItemText( walk, 0, rec.m_key.c_str() );
					m_list.SetItemText( walk, 1, rec.m_value.c_str() ); 
				}
			}

			ShowWindow(SW_SHOW);

			return TRUE; // system should set focus
		}

		// Context menu handler
		void OnContextMenu(CWindow wnd, CPoint point) {
			// did we get a (-1,-1) point due to context menu key rather than right click?
			// GetContextMenuPoint fixes that, returning a proper point at which the menu should be shown
			point = m_list.GetContextMenuPoint(point);

			CMenu menu; 
			// WIN32_OP_D() : debug build only return value check
			// Used to check for obscure errors in debug builds, does nothing (ignores errors) in release build
			WIN32_OP_D(menu.CreatePopupMenu());

			enum { ID_TEST1 = 1, ID_TEST2, ID_SELECTALL, ID_SELECTNONE, ID_INVERTSEL };
			menu.AppendMenu(MF_STRING, ID_TEST1, L"Test 1");
			menu.AppendMenu(MF_STRING, ID_TEST2, L"Test 2");
			menu.AppendMenu(MF_SEPARATOR);
			// Note: Ctrl+A handled automatically by CListControl, no need for us to catch it
			menu.AppendMenu(MF_STRING, ID_SELECTALL, L"Select all\tCtrl+A");
			menu.AppendMenu(MF_STRING, ID_SELECTNONE, L"Select none");
			menu.AppendMenu(MF_STRING, ID_INVERTSEL, L"Invert selection");

			int cmd;
			{
				// Callback object to show menu command descriptions in the status bar.
				// it's actually a hidden window, needs a parent HWND, where we feed our control's HWND
				CMenuDescriptionMap descriptions(m_hWnd);

				// Set descriptions of all our items
				descriptions.Set(ID_TEST1, "This is a test item #1");
				descriptions.Set(ID_TEST2, "This is a test item #2");

				descriptions.Set(ID_SELECTALL, "Selects all items");
				descriptions.Set(ID_SELECTNONE, "Deselects all items");
				descriptions.Set(ID_INVERTSEL, "Invert selection");

				cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
			}
			switch(cmd) {
			case ID_TEST1:
			{
				pfc::string_formatter msg;
				msg << "Test command #1 triggered.\r\n";
				msg << m_list.GetSelectedCount() << " items selected.";
				// popup_message : non-blocking MessageBox equivalent
				popup_message::g_show(msg, "Information");
			}
				break;
			case ID_TEST2:
			{
				pfc::string_formatter msg;
				msg << "Test command #1 triggered.\r\n";
				msg << "Selected items:\r\n";
				for( size_t walk = 0; walk < m_list.GetItemCount(); ++ walk) {
					if ( m_list.IsItemSelected( walk ) ) {
						msg << "#" << (walk+1) << "\r\n";
					}
				}
				msg << "End selected items.";
				// popup_message : non-blocking MessageBox equivalent
				popup_message::g_show(msg, "Information");
			}
				break;
			case ID_SELECTALL:
				m_list.SelectAll(); // trivial
				break;
			case ID_SELECTNONE:
				m_list.SelectNone(); // trivial
				break;
			case ID_INVERTSEL:
			{
				auto mask = m_list.GetSelectionMask();
				m_list.SetSelection( 
					// Items which we alter - all of them
					pfc::bit_array_true(),
					// Selection values - NOT'd original selection mask
					pfc::bit_array_not(mask) 
				);
				// Exclusion of footer item from selection handled via CanSelectItem()
			}
				break;
			}
		}

	private:

		CListControlSimple m_list;
	};
}

// Called from mainmenu.cpp
void RunListControlSimpleDemo() {
	fb2k::newDialog<CListControlSimpleDemoDialog>();	
}
