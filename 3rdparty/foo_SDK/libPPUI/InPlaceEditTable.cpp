#include "stdafx.h"
#include "InPlaceEditTable.h"

#include "win32_op.h"
#include "win32_utility.h"
#include "AutoComplete.h"

#include "listview_helper.h"

namespace InPlaceEdit {

	bool CTableEditHelperV2::tableEdit_cancel_task() {
		bool rv = false;
		if (m_taskKill) {
			*m_taskKill = true; m_taskKill = nullptr;
			rv = true;
		}
		return rv;
	}
	reply_t CTableEditHelperV2::tableEdit_create_task() {
		tableEdit_cancel_task();
		auto ks = std::make_shared<bool>(false);
		m_taskKill = ks;
		return [ks,this](unsigned code) {
			if ( ! * ks ) {
				this->tableEdit_on_task_completion( code );
			}
		};
	}
	t_size CTableEditHelperV2::ColumnToPosition(t_size col) const {
		PFC_ASSERT(TableEdit_IsColumnEditable(col));
		pfc::array_t<t_size> colOrder; GrabColumnOrder(colOrder);
		t_size skipped = 0;
		for (t_size walk = 0; walk < colOrder.get_size(); ++walk) {
			const t_size curCol = colOrder[walk];
			if (TableEdit_IsColumnEditable(curCol)) {
				if (curCol == col) return skipped;
				++skipped;
			}
		}
		PFC_ASSERT(!"Should not get here.");
		return ~0;
	}
	t_size CTableEditHelperV2::PositionToColumn(t_size pos) const {
		pfc::array_t<t_size> colOrder; GrabColumnOrder(colOrder);
		t_size skipped = 0;
		for (t_size walk = 0; walk < colOrder.get_size(); ++walk) {
			const t_size curCol = colOrder[walk];
			if (TableEdit_IsColumnEditable(curCol)) {
				if (skipped == pos) return curCol;
				++skipped;
			}
		}
		PFC_ASSERT(!"Should not get here.");
		return ~0;
	}
	t_size CTableEditHelperV2::EditableColumnCount() const {
		const t_size total = TableEdit_GetColumnCount();
		t_size found = 0;
		for (t_size walk = 0; walk < total; ++walk) {
			if (TableEdit_IsColumnEditable(walk)) found++;
		}
		return found;
	}

	bool CTableEditHelperV2::TableEdit_Advance(t_size & p_item, t_size & p_subItem, t_uint32 whathappened) {
		size_t guardItem = SIZE_MAX, guardSubItem = SIZE_MAX; // infinite loop guard
		size_t item = p_item, subItem = p_subItem;
		for ( ;; ) {
			unsigned _item((unsigned)item), _subItem((unsigned)ColumnToPosition(subItem));
			if (!InPlaceEdit::TableEditAdvance(_item, _subItem, (unsigned)TableEdit_GetItemCount(), (unsigned)EditableColumnCount(), whathappened)) return false;
			item = _item; subItem = PositionToColumn(_subItem);
			
			if ( guardItem == SIZE_MAX ) {
				guardItem = item; guardSubItem = subItem;
			} else {
				// infinite loop guard
				if ( item == guardItem && subItem == guardSubItem ) return false;
			}

			if (TableEdit_CanAdvanceHere(item, subItem, whathappened)) break;
		}
		p_item = item;
		p_subItem = subItem;
		return true;
	}

	void CTableEditHelperV2::TableEdit_Abort(bool forwardContent) {
		if (tableEdit_cancel_task()) {
			if (forwardContent && (m_editFlags & KFlagReadOnly) == 0) {
				if (m_editData.is_valid()) {
					pfc::string8 temp(*m_editData);
					TableEdit_SetField(m_editItem, m_editSubItem, temp);
				}
			}
			m_editData.release();
			m_editDataCombo.reset();
			::SetFocus(TableEdit_GetParentWnd());
			TableEdit_Finished();
		}

	}

	void CTableEditHelperV2::TableEdit_Start(t_size item, t_size subItem) {
		PFC_ASSERT(TableEdit_IsColumnEditable(subItem));
		m_editItem = item; m_editSubItem = subItem;
		_ReStart();
	}

	void CTableEditHelperV2::_ReStart() {
		PFC_ASSERT(m_editItem < TableEdit_GetItemCount());
		PFC_ASSERT(m_editSubItem < TableEdit_GetColumnCount());

		TableEdit_SetItemFocus(m_editItem, m_editSubItem);

		m_editFlags = TableEdit_GetEditFlags(m_editItem, m_editSubItem);

		m_editData.release();
		m_editDataCombo.reset();

		if (m_editFlags & InPlaceEdit::KFlagCombo) {
			auto combo = TableEdit_GetCombo(m_editItem, m_editSubItem);
			RECT rc = TableEdit_GetItemRect(m_editItem, m_editSubItem);

			auto data = std::make_shared< combo_t >(combo);
			m_editDataCombo = data;

			auto task = tableEdit_create_task();
			auto comboTask = [data, task](unsigned status, unsigned sel) {
				data->iDefault = sel;
				task(status);
			};

			InPlaceEdit::StartCombo(TableEdit_GetParentWnd(), rc, m_editFlags, combo.strings, combo.iDefault, comboTask );
			return;
		}

		m_editData.new_t();
		t_size lineCount = 1;
		TableEdit_GetField(m_editItem, m_editSubItem, *m_editData, lineCount);

		RECT rc = TableEdit_GetItemRect(m_editItem, m_editSubItem);
		if (lineCount > 1) {
			rc.bottom = (LONG)( rc.top + (rc.bottom - rc.top) * lineCount );
			m_editFlags |= KFlagMultiLine;
		}
		auto ac = this->TableEdit_GetAutoCompleteEx(m_editItem, m_editSubItem );
		InPlaceEdit::StartEx(TableEdit_GetParentWnd(), rc, m_editFlags, m_editData, tableEdit_create_task(), ac.data.get_ptr(), ac.options);
	}

	CTableEditHelperV2::combo_t CTableEditHelperV2::TableEdit_GetCombo(size_t item, size_t sub) {
		return combo_t();
	}
	CTableEditHelperV2::autoComplete_t CTableEditHelperV2::TableEdit_GetAutoCompleteEx( size_t item, size_t sub ) {
		autoComplete_t ret;
		if ( this->TableEdit_GetAutoComplete( item, sub, ret.data ) ) ret.options = ret.optsDefault;
		return ret;
	}

	void CTableEditHelperV2::tableEdit_on_task_completion(unsigned status) {
		tableEdit_cancel_task();
		if (m_editData.is_valid()) {
			if (status & InPlaceEdit::KEditFlagContentChanged) {
				TableEdit_SetField(m_editItem, m_editSubItem, *m_editData);
			}
			m_editData.release();
		}
		if (m_editDataCombo != nullptr) {
			unsigned idx = m_editDataCombo->iDefault;
			if ( idx < m_editDataCombo->strings.get_count()) {
				const char * text = m_editDataCombo->strings.get_item(idx);
				TableEdit_SetField(m_editItem, m_editSubItem, text);
			}

			m_editDataCombo.reset();
		}

		if (TableEdit_Advance(m_editItem, m_editSubItem, status)) {
			_ReStart();
		} else {
			TableEdit_Finished();
		}
	}





	void CTableEditHelperV2_ListView::TableEdit_GetColumnOrder(t_size * out, t_size count) const {
		pfc::array_t<int> temp; temp.set_size(count);
		WIN32_OP_D(ListView_GetColumnOrderArray(TableEdit_GetParentWnd(), count, temp.get_ptr()));
		for (t_size walk = 0; walk < count; ++walk) out[walk] = temp[walk];
	}

	RECT CTableEditHelperV2_ListView::TableEdit_GetItemRect(t_size item, t_size subItem) const {
		RECT rc;
		WIN32_OP_D(ListView_GetSubItemRect(TableEdit_GetParentWnd(), (int)item, (int)subItem, LVIR_LABEL, &rc));
		return rc;
	}

	void CTableEditHelperV2_ListView::TableEdit_GetField(t_size item, t_size subItem, pfc::string_base & out, t_size & lineCount) {
		listview_helper::get_item_text(TableEdit_GetParentWnd(), (int)item, (int)subItem, out);
		lineCount = pfc::is_multiline(out) ? 5 : 1;
	}
	void CTableEditHelperV2_ListView::TableEdit_SetField(t_size item, t_size subItem, const char * value) {
		WIN32_OP_D(listview_helper::set_item_text(TableEdit_GetParentWnd(), (int)item, (int)subItem, value));
	}
	t_size CTableEditHelperV2_ListView::TableEdit_GetItemCount() const {
		LRESULT temp;
		WIN32_OP_D((temp = ListView_GetItemCount(TableEdit_GetParentWnd())) >= 0);
		return (t_size)temp;
	}
	void CTableEditHelperV2_ListView::TableEdit_SetItemFocus(t_size item, t_size subItem) {
		WIN32_OP_D(listview_helper::select_single_item(TableEdit_GetParentWnd(), (int) item));
	}

	t_size CTableEditHelperV2_ListView::TableEdit_GetColumnCount() const { 
		return (t_size)ListView_GetColumnCount(TableEdit_GetParentWnd()); 
	}

}
