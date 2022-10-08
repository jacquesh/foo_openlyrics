#include "stdafx.h"

#include "playback_stream_capture.h"

// I am foo_sample and these are *my* GUIDs
// Make your own when reusing code or else
static const GUID guid_mainmenu_group_id = { 0x44963e7a, 0x4b2a, 0x4588, { 0xb0, 0x17, 0xa8, 0x69, 0x18, 0xcb, 0x8a, 0xa5 } };
static const GUID guid_test = { 0x7c4726df, 0x3b2d, 0x4c7c,{ 0xad, 0xe8, 0x43, 0xd8, 0x46, 0xbe, 0xce, 0xa8 } };
static const GUID guid_playbackstate = { 0xbd880c51, 0xf0cc, 0x473f,{ 0x9d, 0x14, 0xa6, 0x6e, 0x8c, 0xed, 0x25, 0xae } };
static const GUID guid_io = { 0xd380c333, 0xa72c, 0x4e1e,{ 0x97, 0xca, 0xed, 0x14, 0xeb, 0x93, 0x76, 0x23 } };
static const GUID guid_listcontrol_advanced = { 0x27e29db0, 0x3079, 0x4ce0, { 0x8b, 0x4a, 0xa0, 0x78, 0xeb, 0x6, 0x56, 0x86 } };
static const GUID guid_listcontrol_simple = { 0x34664996, 0x54cd, 0x48eb, { 0xa8, 0x20, 0x8f, 0x45, 0x7d, 0xcc, 0xff, 0xbb } };
static const GUID guid_listcontrol_ownerdata = { 0xc6d23696, 0x4be5, 0x4daa, { 0xaf, 0xb2, 0x35, 0x14, 0xa, 0x47, 0xd2, 0xf9 } };
static const GUID guid_playback_stream_capture = { 0x3d0f0f1a, 0x6b5f, 0x42e3, { 0xa4, 0x6d, 0x49, 0x1, 0x3, 0xf0, 0x54, 0xb2 } };


static mainmenu_group_popup_factory g_mainmenu_group(guid_mainmenu_group_id, mainmenu_groups::file, mainmenu_commands::sort_priority_dontcare, "Sample component");

void RunPlaybackStateDemo(); //playback_state.cpp
void RunIOTest(); // IO.cpp
void RunListControlSimpleDemo(); // listcontrol-simple.cpp
void RunListControlOwnerDataDemo(); // listcontrol-ownerdata.cpp
void RunListControlAdvancedDemo(); // listcontrol-advanced.cpp

class mainmenu_commands_sample : public mainmenu_commands {
public:
	enum {
		cmd_test = 0,
		cmd_playbackstate,
		cmd_io,
		cmd_listcontrol_simple,
		cmd_listcontrol_ownerdata,
		cmd_listcontrol_advanced,
		cmd_playback_stream_capture,
		cmd_total
	};
	t_uint32 get_command_count() override {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) override {

		switch(p_index) {
			case cmd_test: return guid_test;
			case cmd_playbackstate: return guid_playbackstate;
			case cmd_io: return guid_io;
			case cmd_listcontrol_simple: return guid_listcontrol_simple;
			case cmd_listcontrol_ownerdata: return guid_listcontrol_ownerdata;
			case cmd_listcontrol_advanced: return guid_listcontrol_advanced;
			case cmd_playback_stream_capture: return guid_playback_stream_capture;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index,pfc::string_base & p_out) override {
		switch(p_index) {
			case cmd_test: p_out = "Test command"; break;
			case cmd_playbackstate: p_out = "Playback state demo"; break;
			case cmd_io: p_out = "I/O test"; break;
			case cmd_listcontrol_simple: p_out = "Simple CListControl demo"; break;
			case cmd_listcontrol_ownerdata: p_out = "Owner-data CListControl demo"; break;
			case cmd_listcontrol_advanced: p_out = "Advanced CListControl demo"; break;
			case cmd_playback_stream_capture: p_out = "Playback stream capture demo"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index,pfc::string_base & p_out) override {
		switch(p_index) {
			case cmd_test: p_out = "This is a sample menu command."; return true;
			case cmd_playbackstate: p_out = "Opens the playback state demo dialog."; return true;
			case cmd_io: p_out = "Runs I/O test."; return true;
			case cmd_listcontrol_simple: p_out = "Runs Simple CListControl demo."; return true;
			case cmd_listcontrol_ownerdata: p_out = "Runs Owner Data CListControl demo."; return true;
			case cmd_listcontrol_advanced: p_out = "Runs Advanced CListControl demo."; return true;
			case cmd_playback_stream_capture: p_out = "Toggles playback stream capture operation."; return true;
			default: return false;
		}
	}
	GUID get_parent() override {
		return guid_mainmenu_group_id;
	}
	void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback) override {
		switch(p_index) {
			case cmd_test:
				popup_message::g_show("This is a sample menu command.", "Blah");
				break;
			case cmd_playbackstate:
				RunPlaybackStateDemo();
				break;
			case cmd_io:
				RunIOTest();
				break;
			case cmd_listcontrol_simple:
				RunListControlSimpleDemo();
				break;
			case cmd_listcontrol_ownerdata:
				RunListControlOwnerDataDemo();
				break;
			case cmd_listcontrol_advanced:
				RunListControlAdvancedDemo();
				break;
			case cmd_playback_stream_capture:
				ToggleCapture();
				break;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_display(t_uint32 p_index,pfc::string_base & p_text,t_uint32 & p_flags) override {
		// OPTIONAL method
		bool rv = __super::get_display(p_index, p_text, p_flags);
		if (rv) switch(p_index) {
		case cmd_playback_stream_capture:
			// Add checkmark if capture is in progress
			if ( IsCaptureRunning() ) p_flags |= flag_checked;
			break;
		}
		return rv;
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_sample> g_mainmenu_commands_sample_factory;
