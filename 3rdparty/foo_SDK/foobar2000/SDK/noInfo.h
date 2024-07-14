#pragma once

#include "file_info.h"

namespace fb2k {
	//! Helper: shared blank file_info object. See: file_info.
	class noInfo_t : public file_info {
		[[noreturn]] static void verboten() { FB2K_BugCheck(); }
	public:
		double		get_length() const override { return 0; }
		replaygain_info	get_replaygain() const override { return replaygain_info_invalid; }

		t_size		meta_get_count() const override { return 0; }
		const char* meta_enum_name(t_size p_index) const override { verboten(); }
		t_size		meta_enum_value_count(t_size p_index) const override { verboten(); }
		const char* meta_enum_value(t_size p_index, t_size p_value_number) const override { verboten(); }
		t_size		meta_find_ex(const char* p_name, t_size p_name_length) const override { return SIZE_MAX; }

		t_size		info_get_count() const override { return 0; }
		const char* info_enum_name(t_size p_index) const override { verboten(); }
		const char* info_enum_value(t_size p_index) const override { verboten(); }
		t_size		info_find_ex(const char* p_name, t_size p_name_length) const override { return SIZE_MAX; }

	private:
		void		set_length(double p_length) override { verboten(); }
		void		set_replaygain(const replaygain_info& p_info) override { verboten(); }

		t_size		info_set_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { verboten(); }
		void		info_remove_mask(const bit_array& p_mask) override { verboten(); }
		t_size		meta_set_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { verboten(); }
		void		meta_insert_value_ex(t_size p_index, t_size p_value_index, const char* p_value, t_size p_value_length) override { verboten(); }
		void		meta_remove_mask(const bit_array& p_mask) override { verboten(); }
		void		meta_reorder(const t_size* p_order) override { verboten(); }
		void		meta_remove_values(t_size p_index, const bit_array& p_mask) override { verboten(); }
		void		meta_modify_value_ex(t_size p_index, t_size p_value_index, const char* p_value, t_size p_value_length) override { verboten(); }

		void		copy(const file_info& p_source) override { verboten(); }
		void		copy_meta(const file_info& p_source) override { verboten(); }
		void		copy_info(const file_info& p_source) override { verboten(); }
		t_size	meta_set_nocheck_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { verboten(); }
		t_size	info_set_nocheck_ex(const char* p_name, t_size p_name_length, const char* p_value, t_size p_value_length) override { verboten(); }

	};

	extern noInfo_t noInfo;
}
