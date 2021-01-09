#pragma once

#include <memory>
#include "InPlaceEdit.h"

namespace InPlaceEdit {
	class NOVTABLE CTableEditHelperV2 {
	public:
		virtual RECT TableEdit_GetItemRect(t_size item, t_size subItem) const = 0;
		virtual void TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount) = 0;
		virtual void TableEdit_SetField(t_size item, t_size subItem, const char * value) = 0;
		virtual HWND TableEdit_GetParentWnd() const = 0;
		virtual bool TableEdit_Advance(t_size & item, t_size & subItem, t_uint32 whathappened);
		virtual bool TableEdit_CanAdvanceHere( size_t item, size_t subItem, uint32_t whatHappened ) const { return true; }
		virtual void TableEdit_Finished() {}
		virtual t_size TableEdit_GetItemCount() const = 0;
		virtual t_size TableEdit_GetColumnCount() const = 0;
		virtual void TableEdit_SetItemFocus(t_size item, t_size subItem) = 0;
		virtual bool TableEdit_IsColumnEditable(t_size subItem) const { return true; }
		virtual void TableEdit_GetColumnOrder(t_size * out, t_size count) const { order_helper::g_fill(out, count); }
		virtual t_uint32 TableEdit_GetEditFlags(t_size item, t_size subItem) const { return 0; }
		virtual bool TableEdit_GetAutoComplete(t_size item, t_size subItem, pfc::com_ptr_t<IUnknown> & out) { return false; }

		struct autoComplete_t {
			pfc::com_ptr_t<IUnknown> data;
			enum { // ACO_* equivalents
				optsNone = 0,
				optsDefault = 1,
				optsAutoSuggest = 1,
				optsAutoAppend = 3,
			};
			DWORD options = optsNone;
		};
		virtual autoComplete_t TableEdit_GetAutoCompleteEx( size_t item, size_t sub );

		struct combo_t {
			unsigned iDefault = 0;
			pfc::string_list_impl strings;
		};

		virtual combo_t TableEdit_GetCombo(size_t item, size_t sub);

		void TableEdit_Start(t_size item, t_size subItem);
		void TableEdit_Abort(bool forwardContent);
		bool TableEdit_IsActive() const { return !!m_taskKill; }
	protected:
		~CTableEditHelperV2() { tableEdit_cancel_task(); }
		CTableEditHelperV2() {}
	private:
		void tableEdit_on_task_completion(unsigned p_status);
		reply_t tableEdit_create_task();
		bool tableEdit_cancel_task();

		t_size ColumnToPosition(t_size col) const;
		t_size PositionToColumn(t_size pos) const;
		t_size EditableColumnCount() const;
		void GrabColumnOrder(pfc::array_t<t_size> & buffer) const { buffer.set_size(TableEdit_GetColumnCount()); TableEdit_GetColumnOrder(buffer.get_ptr(), buffer.get_size()); }
		void _ReStart();

		t_size m_editItem, m_editSubItem;
		t_uint32 m_editFlags;
		pfc::rcptr_t<pfc::string8> m_editData;
		std::shared_ptr< combo_t > m_editDataCombo;

		std::shared_ptr<bool> m_taskKill;

		CTableEditHelperV2( const CTableEditHelperV2 & ) = delete;
		void operator=( const CTableEditHelperV2 & ) = delete;
	};




	class NOVTABLE CTableEditHelperV2_ListView : public CTableEditHelperV2 {
	public:
		RECT TableEdit_GetItemRect(t_size item, t_size subItem) const override;
		void TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount) override;
		void TableEdit_SetField(t_size item, t_size subItem, const char * value) override;

		t_size TableEdit_GetColumnCount() const override;

		t_size TableEdit_GetItemCount() const override;
		void TableEdit_SetItemFocus(t_size item, t_size subItem) override;

		void TableEdit_GetColumnOrder(t_size * out, t_size count) const override;
	};
}
