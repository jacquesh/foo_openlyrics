#pragma once

// foobar2000 v2.0+ only
#if FOOBAR2000_TARGET_VERSION >= 81

template<typename parent_t>
class CListControlFb2kColors : public parent_t, protected ui_config_callback_impl {
public:
	template<typename ... arg_t>
	CListControlFb2kColors(arg_t && ... args) : parent_t(std::forward<arg_t>(args) ... ) {
		this->SetDarkMode(m_uiConfig->is_dark_mode());
	}
protected:
	void ui_colors_changed() override {
		this->SetDarkMode(m_uiConfig->is_dark_mode());
		this->Invalidate();
	}
	COLORREF GetSysColorHook(int colorIndex) const override {
		return m_uiConfig->getSysColor(colorIndex);
	}
	const ui_config_manager::ptr m_uiConfig = ui_config_manager::get();
};

#endif
