#include "stdafx.h"

#include <helpers/advconfig_impl.h>
#include <SDK/cfg_var.h>

#ifdef _WIN32
#include "resource.h"
#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
#endif // _WIN32

// Sample preferences interface: two meaningless configuration settings accessible through a preferences page and one accessible through advanced preferences.

// Dark Mode:
// (1) Add fb2k::CDarkModeHooks member.
// (2) Initialize it in our WM_INITDIALOG handler.
// (3) Tell foobar2000 that this prefernces page supports dark mode, by returning correct get_state() flags.
// That's all.


// These GUIDs identify the variables within our component's configuration file.
static constexpr GUID guid_cfg_bogoSetting1 = { 0xbd5c777, 0x735c, 0x440d, { 0x8c, 0x71, 0x49, 0xb6, 0xac, 0xff, 0xce, 0xb8 } };
static constexpr GUID guid_cfg_bogoSetting2 = { 0x752f1186, 0x9f61, 0x4f91, { 0xb3, 0xee, 0x2f, 0x25, 0xb1, 0x24, 0x83, 0x5d } };

// This GUID identifies our Advanced Preferences branch (replace with your own when reusing code).
static constexpr GUID guid_advconfig_branch = { 0x28564ced, 0x4abf, 0x4f0c, { 0xa4, 0x43, 0x98, 0xda, 0x88, 0xe2, 0xcd, 0x39 } };
// This GUID identifies our Advanced Preferences setting (replace with your own when reusing code) as well as this setting's storage within our component's configuration file.
static constexpr GUID guid_cfg_bogoSetting3 = { 0xf7008963, 0xed60, 0x4084, { 0xa8, 0x5d, 0xd1, 0xcd, 0xc5, 0x51, 0x22, 0xca } };
static constexpr GUID guid_cfg_bogoSetting4 = { 0x99e206f8, 0xd7be, 0x47e6, { 0xb5, 0xfe, 0xf4, 0x41, 0x5f, 0x6c, 0x16, 0xed } };
static constexpr GUID guid_cfg_bogoSetting5 = { 0x7369ad25, 0x9e81, 0x4e2f, { 0x8b, 0xe7, 0x95, 0xcb, 0x81, 0x99, 0x9b, 0x1b } };

static constexpr GUID guid_cfg_bogoRadio = { 0x4c18b9ab, 0xf823, 0x4d05, { 0xb6, 0x18, 0x14, 0xb9, 0x4c, 0xad, 0xa2, 0xaa } };
static constexpr GUID guid_cfg_bogoRadio1 = { 0xdd0a3a95, 0x1546, 0x4f25, { 0xa6, 0x8c, 0x23, 0xcf, 0x3d, 0xa5, 0x8f, 0x59 } };
static constexpr GUID guid_cfg_bogoRadio2 = { 0xc35e1689, 0xb96f, 0x4bf4, { 0x95, 0xfb, 0xb7, 0x8e, 0x83, 0xc5, 0x2c, 0x7d } };
static constexpr GUID guid_cfg_bogoRadio3 = { 0x790fe179, 0x9908, 0x47e2, { 0x9f, 0xde, 0xce, 0xe1, 0x3f, 0x9a, 0xfc, 0x5b } };

// defaults
constexpr int default_cfg_bogo1 = 1337,
	default_cfg_bogo2 = 666,
	default_cfg_bogo3 = 42;

static constexpr char default_cfg_bogo4[] = "foobar";
static constexpr bool default_cfg_bogo5 = false;

// Advanced preferences order
enum {
	order_bogo3,
	order_bogo4,
	order_bogo5,
	order_bogoRadio
};

enum {
	order_bogoRadio1,
	order_bogoRadio2,
	order_bogoRadio3,
};

namespace foo_sample { // accessed also from Mac specific code hence not static
    cfg_uint cfg_bogoSetting1(guid_cfg_bogoSetting1, default_cfg_bogo1), cfg_bogoSetting2(guid_cfg_bogoSetting2, default_cfg_bogo2);
}
using namespace foo_sample;

static advconfig_branch_factory g_advconfigBranch("Sample Component", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);
static advconfig_integer_factory cfg_bogoSetting3("Bogo setting 3","foo_sample.bogo3", guid_cfg_bogoSetting3, guid_advconfig_branch, order_bogo3, default_cfg_bogo3, 0 /*minimum value*/, 9999 /*maximum value*/);
static advconfig_string_factory cfg_bogoSetting4("Bogo setting 4", "foo_sample.bogo4", guid_cfg_bogoSetting4, guid_advconfig_branch, order_bogo4, default_cfg_bogo4);
static advconfig_checkbox_factory cfg_bogoSetting5("Bogo setting 5", "foo_sample.bogo5", guid_cfg_bogoSetting5, guid_advconfig_branch, order_bogo5, default_cfg_bogo5);
static advconfig_branch_factory cfg_bogoRadioBranch("Bogo radio", guid_cfg_bogoRadio, guid_advconfig_branch, order_bogoRadio);
static advconfig_radio_factory cfg_bogoRadio1("Radio 1", "foo_sample.bogoRaidio.1", guid_cfg_bogoRadio1, guid_cfg_bogoRadio, order_bogoRadio1, true);
static advconfig_radio_factory cfg_bogoRadio2("Radio 2", "foo_sample.bogoRaidio.2", guid_cfg_bogoRadio2, guid_cfg_bogoRadio, order_bogoRadio2, false);
static advconfig_radio_factory cfg_bogoRadio3("Radio 3", "foo_sample.bogoRaidio.3", guid_cfg_bogoRadio3, guid_cfg_bogoRadio, order_bogoRadio3, false);

#ifdef _WIN32
class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_MYPREFERENCES};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP_EX(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_BOGO1, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_BOGO2, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;

	// Dark mode hooks object, must be a member of dialog class.
	fb2k::CDarkModeHooks m_dark;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	
	// Enable dark mode
	// One call does it all, applies all relevant hacks automatically
	m_dark.AddDialogWithControls(*this);

	SetDlgItemInt(IDC_BOGO1, (UINT) cfg_bogoSetting1, FALSE);
	SetDlgItemInt(IDC_BOGO2, (UINT) cfg_bogoSetting2, FALSE);
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	// IMPORTANT: Always return dark_mode_supported - tell foobar2000 that this preferences page is dark mode compliant.
	t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SetDlgItemInt(IDC_BOGO1, default_cfg_bogo1, FALSE);
	SetDlgItemInt(IDC_BOGO2, default_cfg_bogo2, FALSE);
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_bogoSetting1 = GetDlgItemInt(IDC_BOGO1, NULL, FALSE);
	cfg_bogoSetting2 = GetDlgItemInt(IDC_BOGO2, NULL, FALSE);
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return GetDlgItemInt(IDC_BOGO1, NULL, FALSE) != cfg_bogoSetting1 || GetDlgItemInt(IDC_BOGO2, NULL, FALSE) != cfg_bogoSetting2;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "Sample Component";}
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		return GUID { 0x7702c93e, 0x24dc, 0x48ed, { 0x8d, 0xb1, 0x3f, 0x27, 0xb3, 0x8c, 0x7c, 0xc9 } };
	}
	GUID get_parent_guid() {return guid_tools;}
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;
#endif // _WIN32

