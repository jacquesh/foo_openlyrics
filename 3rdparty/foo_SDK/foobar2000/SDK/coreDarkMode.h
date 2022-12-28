#pragma once

#ifdef _WIN32

/*
 ==== fb2k::coreDarkMode API ====
For many components, main issue with implementing of Dark Mode is that libPPUI Dark Mode implementation is *huge* and adds a massive amount of code to otherwise small component DLL.
Using fb2k::coreDarkMode addresses this - provides interface to foo_ui_std implementation of Dark Mode hooks.
This allows implementing Dark Mode without size bloat - however, it also means talking to possibly outdated libPPUI that comes with user's foobar2000 install.
If you use this, you should make sure that your UI works properly with the oldest supported foobar2000 release with Dark Mode, such as early v2.0 builds.
*/


namespace fb2k {
	class coreDarkModeObj : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE(coreDarkModeObj, service_base);
	public:
		virtual void addCtrlAuto(HWND) = 0;
		virtual void addDialog(HWND) = 0;
		virtual void addControls(HWND) = 0;
		virtual void setDarkMode(bool) = 0; // valid ONLY if create(bool) was used
		virtual bool isDark() = 0;
	};

	class coreDarkMode : public service_base {
		FB2K_MAKE_SERVICE_COREAPI(coreDarkMode);
	public:
		virtual coreDarkModeObj::ptr create(bool) = 0;
		virtual coreDarkModeObj::ptr createAuto() = 0; // auto updates with fb2k config, disregards setDarkMode()
	};

	FOOGUIDDECL const GUID coreDarkMode::class_guid = { 0xe34839b0, 0xd899, 0x43c5, { 0x94, 0x71, 0x14, 0xff, 0x48, 0x1d, 0x5e, 0xe8 } };
	FOOGUIDDECL const GUID coreDarkModeObj::class_guid =  { 0xdf2e7682, 0x4b52, 0x42d8, { 0x80, 0xc8, 0x2e, 0x46, 0xf9, 0x41, 0x55, 0xd2 } };

	//! Intended as drop-in replacement of fb2k::CDarkModeHooks (see helpers/DarkMode.h), only using fb2k::coreDarkMode instead of locally-linked libPPUI. \n
	//! Does nothing under pre-v2.0 foobar2000.
	class CCoreDarkModeHooks {
	public:
		void AddCtrlAuto(HWND wnd) {
			if (is_valid()) m_obj->addCtrlAuto(wnd);
		}
		void AddDialog(HWND wnd) {
			if (is_valid()) m_obj->addDialog(wnd);
		}
		void AddControls(HWND wnd) {
			if (is_valid()) m_obj->addControls(wnd);
		}
		void AddDialogWithControls(HWND wnd) {
			AddDialog(wnd); AddControls(wnd);
		}

		void SetDark(bool v) {
			if (is_valid()) m_obj->setDarkMode(v);
		}
		operator bool() const {
			return is_valid() && m_obj->isDark();
		}

		
		CCoreDarkModeHooks() {
			fb2k::coreDarkMode::ptr api;
			if (core_api::are_services_available()) api = fb2k::coreDarkMode::tryGet();
			if (!api.is_valid()) return;
			m_obj = api->createAuto();
		}
		CCoreDarkModeHooks(bool v) {
			fb2k::coreDarkMode::ptr api;
			if (core_api::are_services_available()) api = fb2k::coreDarkMode::tryGet();
			if (!api.is_valid()) return;
			m_obj = api->create(v);
		}
		CCoreDarkModeHooks(const CCoreDarkModeHooks&) = delete;
		void operator=(const CCoreDarkModeHooks&) = delete;
	private:
		bool is_valid() const {
			return m_obj.is_valid();
		}
		fb2k::coreDarkModeObj::ptr m_obj;
	};
}

#endif // _WIN32

