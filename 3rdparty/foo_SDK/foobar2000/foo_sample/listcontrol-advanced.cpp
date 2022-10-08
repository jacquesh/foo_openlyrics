// Advanced CListControl use demo
// Subclasses a CListControl to use all its features

#include "stdafx.h"
#include "resource.h"
#include <helpers/atl-misc.h>
#include <libPPUI/CListControlComplete.h>
#include <libPPUI/CListControl-Cells.h>
#include <string>
#include <algorithm>
#include <vector>

namespace {
	struct listData_t {
		std::string m_key, m_value;
		bool m_checkState = false;
	};
	static std::vector<listData_t> makeListData() {
		std::vector<listData_t> data;
		data.resize( 10 );
		for( size_t walk = 0; walk < data.size(); ++ walk ) {
			auto & rec = data[walk];
			rec.m_key = (PFC_string_formatter() << "Item #" << (walk+1) ).c_str();
			rec.m_value = "edit me";
		}
		return data;
	}
	
	// See CListControlComplete.h for base class description
	typedef CListControlComplete CListControlDemoParent;

	class CListControlDemo : public CListControlDemoParent {
		typedef CListControlDemoParent parent_t;
	public:
		BEGIN_MSG_MAP_EX(CListControlDemo)
			CHAIN_MSG_MAP( parent_t );
			MSG_WM_CREATE(OnCreate)
			MSG_WM_CONTEXTMENU(OnContextMenu)
		END_MSG_MAP()


		// Context menu handler
		void OnContextMenu(CWindow wnd, CPoint point) {
			// did we get a (-1,-1) point due to context menu key rather than right click?
			// GetContextMenuPoint fixes that, returning a proper point at which the menu should be shown
			point = this->GetContextMenuPoint(point);

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
				msg << this->GetSelectedCount() << " items selected.";
				// popup_message : non-blocking MessageBox equivalent
				popup_message::g_show(msg, "Information");
			}
				break;
			case ID_TEST2:
			{
				pfc::string_formatter msg;
				msg << "Test command #1 triggered.\r\n";
				msg << "Selected items:\r\n";
				for( size_t walk = 0; walk < GetItemCount(); ++ walk) {
					if ( this->IsItemSelected( walk ) ) {
						msg << m_data[walk].m_key.c_str() << "\r\n";
					}
				}
				msg << "End selected items.";
				// popup_message : non-blocking MessageBox equivalent
				popup_message::g_show(msg, "Information");
			}
				break;
			case ID_SELECTALL:
				this->SelectAll(); // trivial
				break;
			case ID_SELECTNONE:
				this->SelectNone(); // trivial
				break;
			case ID_INVERTSEL:
			{
				auto mask = this->GetSelectionMask();
				this->SetSelection( 
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
		
		int OnCreate(LPCREATESTRUCT lpCreateStruct) {
			InitHeader(); // set up header control with columns
			SetWindowText(L"List Control Demo"); // screen reader will see this
			return 0;
		}
		void InitHeader() {
			InitializeHeaderCtrl(HDS_FULLDRAG);

			// never hardcode values in pixels, always use screen DPI
			auto DPI = m_dpi;
			AddColumn( "Check", MulDiv(60, DPI.cx, 96 ) );
			AddColumn( "Name", MulDiv(100, DPI.cx, 96 ) );
			AddColumn( "Value", MulDiv(100, DPI.cx, 96 ) );
		}

		bool CanSelectItem( size_t row ) const override {
			// can not select footer
			return row != footerRow();
		}
		size_t footerRow() const {
			return m_data.size();
		}
		t_size GetItemCount() const override {
			return m_data.size() + 1; // footer
		}
		void onFooterClicked() {
			SelectNone();
			listData_t obj = {};
			obj.m_key = "New item";
			size_t index = m_data.size();
			m_data.push_back( std::move(obj) );
			OnItemsInserted(index, 1, true);
		}
		void OnSubItemClicked(t_size item, t_size subItem, CPoint pt) override {
			if ( item == footerRow() ) {
				onFooterClicked(); return;
			}
			if ( subItem == 2 ) {
				TableEdit_Start(item, subItem); return;
			}
			__super::OnSubItemClicked( item, subItem, pt );
		}
		bool AllowScrollbar(bool vertical) const override {
			return true;
		}
		t_size InsertIndexFromPointEx(const CPoint & pt, bool & bInside) const override {
			// Drag&drop insertion point hook, for reordering only
			auto ret = __super::InsertIndexFromPointEx(pt, bInside);
			bInside = false; // never drop *into* an item, only between, as we only allow reorder
			if ( ret > m_data.size() ) ret = m_data.size(); // never allow drop beyond footer
			return ret;
		}
		void RequestReorder(size_t const * order, size_t count) override {
			// we've been asked to reorder the items, by either drag&drop or cursors+modifiers
			// we can either reorder as requested, reorder partially if some of the items aren't moveable, or reject the request
			
			PFC_ASSERT( count == GetItemCount() );

			// Footer row cannot be moved
			if ( order[footerRow()] != footerRow() ) return;

			pfc::reorder_t( m_data, order, count );
			this->OnItemsReordered( order, count );
		}
		void RemoveMask( pfc::bit_array const & mask ) {
			if ( mask.get(footerRow() ) ) return; // footer row cannot be removed
			auto oldCount = GetItemCount();
			pfc::remove_mask_t( m_data, mask );
			this->OnItemsRemoved( mask, oldCount );
		}
		void RequestRemoveSelection() override {
			// Delete key etc
			RemoveMask(GetSelectionMask());
		}
		void ExecuteDefaultAction(t_size index) override {
			// double click, enter key, etc
			if ( index == footerRow() ) onFooterClicked();
		}

		bool GetSubItemText(t_size item, t_size subItem, pfc::string_base & out) const override {
			if ( item == footerRow() ) {
				if ( subItem == 0 ) {
					out = "+ add new";
					return true;
				}
				return false;
			}
			auto & rec = m_data[item];
			switch(subItem) {
			case 0:
				// pass blank string or return false to create a checkbox only column
				out = "check";
				return true;
			case 1:
				out = rec.m_key.c_str();
				return true;
			case 2:
				out = rec.m_value.c_str();
				return true;
			default:
				return false;
			}
		}

		size_t GetSubItemSpan(size_t row, size_t column) const override {
			if ( row == footerRow() && column == 0 ) {
				return GetColumnCount();				
			}
			return 1;
		}
		cellType_t GetCellType(size_t item, size_t subItem) const override {
			// cellType_t is a pointer to a cell class object supplying cell behavior specification & rendering methods
			// use PFC_SINGLETON to provide static instances of used cells
			if ( item == footerRow() ) {
				if ( subItem == 0 ) {
					return & PFC_SINGLETON( CListCell_Button );
				} else {
					return nullptr;
				}
			}
			switch(subItem) {
			case 0:
				return & PFC_SINGLETON( CListCell_Checkbox );
			default:
				return & PFC_SINGLETON( CListCell_Text );
			}

		}
		bool GetCellTypeSupported() const override {
			return true;
		}
		bool GetCellCheckState(size_t item, size_t subItem) const override {
			if ( subItem == 0 ) {
				auto & rec = m_data[item];
				return rec.m_checkState;
			}
			return false;
		}
		void SetCellCheckState(size_t item, size_t subItem, bool value) override {
			if ( subItem == 0 ) {
				auto & rec = m_data[item];
				if (rec.m_checkState != value) {
					rec.m_checkState = value;
					__super::SetCellCheckState(item, subItem, value);
				}
			}
		}

		uint32_t QueryDragDropTypes() const override {return dragDrop_reorder;}

		// Inplace edit handlers
		// Overrides of CTableEditHelperV2 methods
		void TableEdit_SetField(t_size item, t_size subItem, const char * value) override {
			if ( subItem == 2 ) {
				m_data[item].m_value = value;
				ReloadItem( item );
			}
		}
		bool TableEdit_IsColumnEditable(t_size subItem) const override {
			return subItem == 2; 
		}
	private:
		std::vector< listData_t > m_data = makeListData();
	};

	// Straightforward WTL dialog code
	class CListControlAdvancedDemoDialog : public CDialogImpl<CListControlAdvancedDemoDialog> {
	public:
		enum { IDD = IDD_LISTCONTROL_DEMO };

		BEGIN_MSG_MAP_EX(CListControlAdvancedDemoDialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		END_MSG_MAP()
	private:
		void OnCancel(UINT, int, CWindow) {
			DestroyWindow();
		}

		BOOL OnInitDialog(CWindow, LPARAM) {

			// Create replacing existing windows list control
			// automatically initialize position, font, etc
			m_list.CreateInDialog( *this, IDC_LIST1 );

			ShowWindow(SW_SHOW);

			return TRUE; // system should set focus
		}

		CListControlDemo m_list;
	};
}

// Called from mainmenu.cpp
void RunListControlAdvancedDemo() {
	// automatically creates the dialog with object lifetime management and modeless dialog registration
	fb2k::newDialog<CListControlAdvancedDemoDialog>();	
}

