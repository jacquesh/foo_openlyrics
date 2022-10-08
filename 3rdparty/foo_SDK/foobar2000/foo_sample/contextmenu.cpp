#include "stdafx.h"


// Identifier of our context menu group. Substitute with your own when reusing code.
static const GUID guid_mygroup = { 0x572de7f4, 0xcbdf, 0x479a, { 0x97, 0x26, 0xa, 0xb0, 0x97, 0x47, 0x69, 0xe3 } };


// Switch to contextmenu_group_factory to embed your commands in the root menu but separated from other commands.

//static contextmenu_group_factory g_mygroup(guid_mygroup, contextmenu_groups::root, 0);
static contextmenu_group_popup_factory g_mygroup(guid_mygroup, contextmenu_groups::root, "Sample component", 0);

static void RunTestCommand(metadb_handle_list_cref data);
void RunCalculatePeak(metadb_handle_list_cref data); //decode.cpp

void RunCopyFiles(metadb_handle_list_cref data); // IO.cpp
void RunAlterTagsLL(metadb_handle_list_cref data); // IO.cpp

void RunUIAndThreads( metadb_handle_list_cref data ); // ui_and_threads.cpp

namespace { // anon namespace local classes for good measure
	class myFilter : public file_info_filter {
	public:
		bool apply_filter(metadb_handle_ptr p_location, t_filestats p_stats, file_info & p_info) {
			p_info.meta_set("comment", "foo_sample was here");
			// return true to write changes tags to the file, false to suppress the update
			return true;
		}
	};
}

static void RunAlterTags(metadb_handle_list_cref data) {
	// Simple alter-file-tags functionality

	const HWND wndParent = core_api::get_main_window();

	// Filter object that applies our edits to the file tags
	auto filter = fb2k::service_new<myFilter>();

	auto notify = fb2k::makeCompletionNotify( [] (unsigned code) {
		// Code values are metadb_io::t_update_info_state enum
		FB2K_console_formatter() << "Tag update finished, code: " << code;
	} );

	// Flags
	// Indicate that we're aware of fb2k 1.3+ partial info semantics
	const uint32_t flags = metadb_io_v2::op_flag_partial_info_aware;

	metadb_io_v2::get()->update_info_async(data, filter, wndParent, flags, notify);
}

// Simple context menu item class.
class myitem : public contextmenu_item_simple {
public:
	enum {
		cmd_test1 = 0,
		cmd_peak,
		cmd_copyFiles,
		cmd_alterTags,
		cmd_alterTagsLL,
		cmd_uiAndThreads,
		cmd_total
	};
	GUID get_parent() {return guid_mygroup;}
	unsigned get_num_items() {return cmd_total;}
	void get_item_name(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_test1: p_out = "Test command"; break;
			case cmd_peak: p_out = "Calculate peak"; break;
			case cmd_copyFiles: p_out = "Copy files"; break;
			case cmd_alterTags: p_out = "Alter tags"; break;
			case cmd_alterTagsLL: p_out = "Alter tags (low level)"; break;
			case cmd_uiAndThreads: p_out = "UI and threads demo"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void context_command(unsigned p_index,metadb_handle_list_cref p_data,const GUID& p_caller) {
		switch(p_index) {
			case cmd_test1:
				RunTestCommand(p_data);
				break;
			case cmd_peak:
				RunCalculatePeak(p_data);
				break;
			case cmd_copyFiles:
				RunCopyFiles(p_data);
				break;
			case cmd_alterTags:
				RunAlterTags(p_data);
				break;
			case cmd_alterTagsLL:
				RunAlterTagsLL(p_data);
				break;
			case cmd_uiAndThreads:
				RunUIAndThreads(p_data);
				break;
			default:
				uBugCheck();
		}
	}
	// Overriding this is not mandatory. We're overriding it just to demonstrate stuff that you can do such as context-sensitive menu item labels.
	bool context_get_display(unsigned p_index,metadb_handle_list_cref p_data,pfc::string_base & p_out,unsigned & p_displayflags,const GUID & p_caller) {
		switch(p_index) {
			case cmd_test1:
				if (!__super::context_get_display(p_index, p_data, p_out, p_displayflags, p_caller)) return false;
				// Example context sensitive label: append the count of selected items to the label.
				p_out << " : " << p_data.get_count() << " item";
				if (p_data.get_count() != 1) p_out << "s";
				p_out << " selected";
				return true;
			default:
				return __super::context_get_display(p_index, p_data, p_out, p_displayflags, p_caller);
		}
	}
	GUID get_item_guid(unsigned p_index) {
		// These GUIDs identify our context menu items. Substitute with your own GUIDs when reusing code.
		static const GUID guid_test1 = { 0x4021c80d, 0x9340, 0x423b, { 0xa3, 0xe2, 0x8e, 0x1e, 0xda, 0x87, 0x13, 0x7f } };
		static const GUID guid_peak = { 0xe629b5c3, 0x5af3, 0x4a1e, { 0xa0, 0xcd, 0x2d, 0x5b, 0xff, 0xa6, 0x4, 0x58 } };
		static const GUID guid_copyFiles = { 0x7f8a6569, 0xe46b, 0x4698, { 0xaa, 0x30, 0xc4, 0xc1, 0x44, 0xc9, 0xc8, 0x92 } };
		static const GUID guid_alterTags = { 0xdfb8182b, 0xf8f3, 0x4ce9, { 0xae, 0xf6, 0x8e, 0x4e, 0x51, 0x7c, 0x2d, 0x3 } };
		static const GUID guid_alterTagsLL = { 0x6b43324d, 0x6cb2, 0x42a6, { 0xbf, 0xc, 0xd9, 0x43, 0xfc, 0x83, 0x2f, 0x39 } };
		static const GUID guid_uiAndThreads = { 0x30dace2e, 0xcccf, 0x41d4, { 0x8c, 0x24, 0x57, 0xec, 0xf4, 0xa0, 0xd9, 0xc9 } };

		switch(p_index) {
			case cmd_test1: return guid_test1;
			case cmd_peak: return guid_peak;
			case cmd_copyFiles: return guid_copyFiles;
			case cmd_alterTags: return guid_alterTags;
			case cmd_alterTagsLL: return guid_alterTagsLL;
			case cmd_uiAndThreads: return guid_uiAndThreads;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}

	}
	bool get_item_description(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case cmd_test1:
				p_out = "This is a sample command.";
				return true;
			case cmd_peak:
				p_out = "This is a sample command that decodes the selected tracks and reports the peak sample value.";
				return true;
			case cmd_copyFiles:
				p_out = "This is a sample command that copies the selected tracks to another location.";
				return true;
			case cmd_alterTags:
				p_out = "This is a sample command that performs tag manipulation on the files.";
				return true;
			case cmd_alterTagsLL:
				p_out = "This is a sample command that performs low-level manipulation of tags on the files.";
				return true;
			case cmd_uiAndThreads:
				p_out = "This is a smple command that runs UI and Threads demo.";
				return true;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

static contextmenu_item_factory_t<myitem> g_myitem_factory;


static void RunTestCommand(metadb_handle_list_cref data) {
	pfc::string_formatter message;
	message << "This is a test command.\n";
	if (data.get_count() > 0) {
		message << "Parameters:\n";
		for(t_size walk = 0; walk < data.get_count(); ++walk) {
			message << data[walk] << "\n";
		}
	}	
	popup_message::g_show(message, "Blah");
}
