#pragma once

#include "file_info.h"

namespace fb2k {
	class noInfo_t : public file_info {
	public:
		double		get_length() const override { return 0; }
		replaygain_info	get_replaygain() const override { return replaygain_info_invalid; }

		t_size		meta_get_count() const override { return 0; }
		const char* meta_enum_name(t_size p_index) const override { uBugCheck(); }
		t_size		meta_enum_value_count(t_size p_index) const override { uBugCheck(); }
		const char* meta_enum_value(t_size p_index, t_size p_value_number) const override { uBugCheck(); }
		t_size		meta_find_ex(const char* p_name, t_size p_name_length) const override { return SIZE_MAX; }

		t_size		info_get_count() const override { return 0; }
		const char* info_enum_name(t_size p_index) const override { uBugCheck(); }
		const char* info_enum_value(t_size p_index) const override { uBugCheck(); }
		t_size		info_find_ex(const char* p_name, t_size p_name_length) const override { return SIZE_MAX; }

	private:
		void		set_length(double p_length) override { uBugCheck(); }
		void		set_replaygain(const replaygain_info& p_info) { uBugCheck(); }

		t_size		info_set_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { uBugCheck(); }
		void		info_remove_mask(const bit_array& p_mask) override { uBugCheck(); }
		t_size		meta_set_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { uBugCheck(); }
		void		meta_insert_value_ex(t_size p_index, t_size p_value_index, const char* p_value, t_size p_value_length) override { uBugCheck(); }
		void		meta_remove_mask(const bit_array& p_mask) override { uBugCheck(); }
		void		meta_reorder(const t_size* p_order) override { uBugCheck(); }
		void		meta_remove_values(t_size p_index, const bit_array& p_mask) override { uBugCheck(); }
		void		meta_modify_value_ex(t_size p_index, t_size p_value_index, const char* p_value, t_size p_value_length) override { uBugCheck(); }

		void		copy(const file_info& p_source) override { uBugCheck(); }
		void		copy_meta(const file_info& p_source) override { uBugCheck(); }
		void		copy_info(const file_info& p_source) override { uBugCheck(); }
		t_size	meta_set_nocheck_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { uBugCheck(); }
		t_size	info_set_nocheck_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { uBugCheck(); }

	};

	extern noInfo_t noInfo;
}
