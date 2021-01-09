#include "stdafx.h"
#include "CTableEditHelper-Legacy.h"
#include <libPPUI/listview_helper.h>

namespace InPlaceEdit {
	void CTableEditHelper::TableEdit_Start(HWND p_listview, unsigned p_item, unsigned p_column, unsigned p_itemcount, unsigned p_columncount, unsigned p_basecolumn, unsigned p_flags) {
		if (m_notify.is_valid() || p_columncount == 0 || p_itemcount == 0 || p_item >= p_itemcount || p_column >= p_columncount) return;
		m_listview = p_listview;
		m_item = p_item;
		m_column = p_column;
		m_itemcount = p_itemcount;
		m_columncount = p_columncount;
		m_basecolumn = p_basecolumn;
		m_flags = p_flags;
		_Start();
	}

	void CTableEditHelper::TableEdit_Abort(bool p_forwardcontent) {
		if (m_notify.is_valid()) {
			m_notify->orphan();
			m_notify.release();

			if (p_forwardcontent && (m_flags & KFlagReadOnly) == 0) {
				if (m_content.is_valid()) {
					pfc::string8 temp(*m_content);
					m_content.release();
					TableEdit_SetItemText(m_item, m_column, temp);
				}
			} else {
				m_content.release();
			}
			SetFocus(NULL);
			TableEdit_Finished();
		}
	}

	bool CTableEditHelper::TableEdit_GetItemText(unsigned p_item, unsigned p_column, pfc::string_base & p_out, unsigned & p_linecount) {
		listview_helper::get_item_text(m_listview, p_item, p_column + m_basecolumn, p_out);
		p_linecount = pfc::is_multiline(p_out) ? 5 : 1;
		return true;
	}
	void CTableEditHelper::TableEdit_SetItemText(unsigned p_item, unsigned p_column, const char * p_text) {
		listview_helper::set_item_text(m_listview, p_item, p_column + m_basecolumn, p_text);
	}

	void CTableEditHelper::on_task_completion(unsigned p_taskid, unsigned p_state) {
		if (p_taskid == KTaskID) {
			m_notify.release();
			if (m_content.is_valid()) {
				if (p_state & InPlaceEdit::KEditFlagContentChanged) {
					TableEdit_SetItemText(m_item, m_column, *m_content);
				}
				m_content.release();
			}
			/*if (InPlaceEdit::TableEditAdvance(m_item,m_column,m_itemcount,m_columncount,p_state))*/
			if (TableEdit_OnEditCompleted(m_item, m_column, p_state) &&
				InPlaceEdit::TableEditAdvance_ListView(m_listview, m_basecolumn, m_item, m_column, m_itemcount, m_columncount, p_state)) {
				_Start();
			} else {
				TableEdit_Finished();
			}
		}
	}

	CTableEditHelper::~CTableEditHelper() {
		if (m_notify.is_valid()) {
			m_notify->orphan();
			m_notify.release();
		}
	}

	void CTableEditHelper::_Start() {
		listview_helper::select_single_item(m_listview, m_item);
		m_content.new_t();
		unsigned linecount = 1;
		if (!TableEdit_GetItemText(m_item, m_column, *m_content, linecount)) return;
		m_notify = completion_notify_create(this, KTaskID);
		InPlaceEdit::Start_FromListViewEx(m_listview, m_item, m_column + m_basecolumn, linecount, m_flags, m_content, m_notify);
	}
}