#pragma once
#include "inplace_edit.h"
#include <libPPUI/listview_helper.h>

namespace InPlaceEdit {
	class CTableEditHelper {
	public:
		void TableEdit_Start(HWND p_listview, unsigned p_item, unsigned p_column, unsigned p_itemcount, unsigned p_columncount, unsigned p_basecolumn, unsigned p_flags = 0);
		void TableEdit_Abort(bool p_forwardcontent);
		bool TableEdit_IsActive() const {return m_notify.is_valid();}

		virtual bool TableEdit_GetItemText(unsigned p_item, unsigned p_column, pfc::string_base & p_out, unsigned & p_linecount);
		virtual void TableEdit_SetItemText(unsigned p_item, unsigned p_column, const char * p_text);

		virtual void TableEdit_Finished() {}

		void on_task_completion(unsigned p_taskid, unsigned p_state);
		~CTableEditHelper();
	protected:
		HWND TableEdit_GetListView() const { return m_listview; }
		//return false to abort
		virtual bool TableEdit_OnEditCompleted(unsigned item, unsigned column, unsigned state) { return true; }
	private:
		void _Start();
		enum {
			KTaskID = 0xc0ffee
		};
		HWND m_listview;
		unsigned m_item, m_column;
		unsigned m_itemcount, m_columncount, m_basecolumn;
		unsigned m_flags;
		pfc::rcptr_t<pfc::string8> m_content;
		service_ptr_t<completion_notify_orphanable> m_notify;
	};
}