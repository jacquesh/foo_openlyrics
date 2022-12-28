#pragma once
#ifdef _WIN32
#include "atl-misc.h"

// dsp_dialog.h
// DSP UI helper
// Simplifies implementation of DSP configuration popups, removes the need for separate blocking/nonblocking dialog routing all calls to one dialog class.
// Your dialog_t class must be WTL dialog taking dsp_preset_edit_callback_v2::ptr parameter in constructor.
// Do not call EndDialog(), DestroyWindow() etc, only use the provided callback's dsp_dialog_done() to make your dialog disappear.

template<typename dialog_t>
void dsp_dialog_v2(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback) {
	core_api::ensure_main_thread();

	class mycallback : public dsp_preset_edit_callback_v2 {
	public:
		void get_preset(dsp_preset& outPreset) override {
			outPreset = m_state;
		}
		void set_preset(const dsp_preset& inPreset) { m_state = inPreset; if (m_chain) m_chain->on_preset_changed(inPreset); }
		void dsp_dialog_done(bool bOK) override {
			if (m_dialog) m_dialog->EndDialog(bOK ? IDOK : IDCANCEL);
		}

		dsp_preset_impl m_state;
		dsp_preset_edit_callback* m_chain = nullptr;
		dialog_t* m_dialog = nullptr;
	};


	auto cb = fb2k::service_new<mycallback>();
	cb->m_state = p_data;
	auto toggle = pfc::autoToggle(cb->m_chain, &p_callback);
	ImplementModalTracking< dialog_t > dialog(cb);
	auto toggle2 = pfc::autoToggle(cb->m_dialog, &dialog);
	dialog.DoModal(p_parent);
}

template<typename dialog_t>
class _dialog_as_service : public dialog_t, public service_base {
public:
	template<typename ... arg_t> _dialog_as_service(arg_t && ... arg) : dialog_t(std::forward<arg_t>(arg) ...) {}
};

template<typename dialog_t>
service_ptr dsp_dialog_v3(HWND parent, dsp_preset_edit_callback_v2::ptr callback) {
	class mycallback : public dsp_preset_edit_callback_v2 {
	public:
		void get_preset(dsp_preset& outPreset) override { m_chain->get_preset(outPreset); }
		void set_preset(const dsp_preset& inPreset) { m_chain->set_preset(inPreset); }
		void dsp_dialog_done(bool bOK) override {
			m_chain->dsp_dialog_done(bOK);
			m_dialog->DestroyWindow();
		}
		dsp_preset_edit_callback_v2::ptr m_chain;
		dialog_t* m_dialog = nullptr;
	};
	auto cb = fb2k::service_new< mycallback >();
	cb->m_chain = callback;
	auto obj = fb2k::service_new_window<ImplementModelessTracking< _dialog_as_service< dialog_t > > >(cb);
	cb->m_dialog = &*obj;
	WIN32_OP_D(obj->Create(parent)); obj->ShowWindow(SW_SHOW);
	return obj;
}
#endif

template<typename dsp_t, typename dialog_t>
class dsp_impl_with_ui : public dsp_entry_common_t<dsp_t, dsp_entry_v3> {
public:
	void get_display_name(const dsp_preset& arg, pfc::string_base& out) override {
		dsp_t::g_get_display_name(arg, out);
	}
#ifdef _WIN32
	void show_config_popup_v2(const dsp_preset& p_data, fb2k::hwnd_t p_parent, dsp_preset_edit_callback& p_callback) override {
		dsp_dialog_v2<dialog_t>(p_data, p_parent, p_callback);
	}
	service_ptr show_config_popup_v3(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback) override {
		return dsp_dialog_v3<dialog_t>(parent, callback);
	}
#else
	service_ptr show_config_popup(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback) override {
		return dialog_t::show_config_popup(parent, callback);
	}
#endif
};
