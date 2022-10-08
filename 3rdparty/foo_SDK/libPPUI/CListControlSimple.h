#pragma once

// ================================================================================
// CListControlSimple
// Simplified CListControl interface; a ready-to-use class that can be instantiated
// without subclassing.
// Use when you don't need advanced features such as buttons or editing.
// Maintains its own data.
// ================================================================================

#include "CListControlComplete.h"

#include <functional>
#include <vector>
#include <map>
#include <string>


class CListControlSimple : public CListControlReadOnly {
public:
	// Events
	std::function<void()> onReordered; // if not set, list reordering is disabled
	std::function<void()> onRemoved; // if not set, list item removal is disabled
	std::function<void(size_t)> onItemAction;
	std::function<void()> onSelChange;

	size_t GetItemCount() const override {
		return m_lines.size();
	}
	void SetItemCount( size_t count ) {
		m_lines.resize( count );
		ReloadData();
	}
	void SetItemText(size_t item, size_t subItem, const char * text, bool bRedraw = true) {
		if ( item < m_lines.size() ) {
			m_lines[item].text[subItem] = text;
			if ( bRedraw ) ReloadItem( item );
		} else {
			PFC_ASSERT(!"CListControlSimple: item index out of range, call SetItemCount() first");
		}
	}
	bool GetSubItemText(size_t item, size_t subItem, pfc::string_base & out) const override {
		if ( item < m_lines.size() ) {
			auto & l = m_lines[item].text;
			auto iter = l.find( subItem );
			if ( iter != l.end() ) {
				out = iter->second.c_str();
				return true;
			}
		}
		return false;
	}

	uint32_t QueryDragDropTypes() const override {
		return (onReordered != nullptr) ? dragDrop_reorder : 0;
	}

	void RequestReorder( const size_t * order, size_t count) override {
		if ( onReordered == nullptr ) return;
		pfc::reorder_t( m_lines, order, count );
		this->OnItemsReordered( order, count );
		onReordered();
	}
	void RequestRemoveSelection() override {
		if (onRemoved == nullptr) return;
		auto mask = this->GetSelectionMask();
		size_t oldCount = m_lines.size();
		pfc::remove_mask_t( m_lines, mask );
		this->OnItemsRemoved( mask, oldCount );
		onRemoved();
	}
	void ExecuteDefaultAction( size_t idx ) override {
		if (onItemAction != nullptr) onItemAction(idx);
	}

	void SetItemUserData( size_t item, size_t user ) {
		if ( item < m_lines.size() ) {
			m_lines[item].user = user;
		}
	}
	size_t GetItemUserData( size_t item ) const {
		size_t ret = 0;
		if ( item < m_lines.size() ) {
			ret = m_lines[item].user;
		}
		return ret;
	}
	void RemoveAllItems() {
		RemoveItems(pfc::bit_array_true());
	}
	void RemoveItems( pfc::bit_array const & mask ) {
		const auto oldCount = m_lines.size();
		pfc::remove_mask_t( m_lines, mask );
		this->OnItemsRemoved( mask, oldCount );
	}
	void RemoveItem( size_t which ) {
		RemoveItems( pfc::bit_array_one( which ) );
	}

	size_t InsertItem( size_t insertAt, const char * textCol0 = nullptr ) {
		if ( insertAt > m_lines.size() ) {
			insertAt = m_lines.size();
		}
		{
			line_t data;
			if ( textCol0 != nullptr ) data.text[0] = textCol0;
			m_lines.insert( m_lines.begin() + insertAt, std::move(data) );
		}
		this->OnItemsInserted( insertAt, 1, false );
		return insertAt;
	}
	size_t AddItem( const char * textCol0 = nullptr ) {
		return InsertItem( SIZE_MAX, textCol0 );
	}
	size_t InsertItems( size_t insertAt, size_t count ) {
		if ( insertAt > m_lines.size() ) {
			insertAt = m_lines.size();
		}

		{
			line_t val;
			m_lines.insert( m_lines.begin() + insertAt, count, val );
		}

		this->OnItemsInserted( insertAt, count, false );
		return insertAt;
	}
protected:
	void OnSelectionChanged(pfc::bit_array const & affected, pfc::bit_array const & status) override {
		__super::OnSelectionChanged(affected, status);
		if ( onSelChange ) onSelChange();
	}
private:
	struct line_t {
		std::map<size_t, std::string> text;
		size_t user = 0;
	};
	std::vector<line_t> m_lines;
};
