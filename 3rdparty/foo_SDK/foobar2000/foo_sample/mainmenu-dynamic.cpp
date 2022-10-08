#include "stdafx.h"
#include <vector>

namespace { // anon namespace everything, it's not accessible by means other than the service factory

// The command ID.
// Generate a new GUID when reusing code.
static const GUID guid_menucommand = { 0xab754b0b, 0x204, 0x4471, { 0xb5, 0x29, 0xff, 0x73, 0xae, 0x51, 0x5d, 0xe9 } };

// Shared with mainmenu.cpp
static const GUID guid_mainmenu_group_id = { 0x44963e7a, 0x4b2a, 0x4588, { 0xb0, 0x17, 0xa8, 0x69, 0x18, 0xcb, 0x8a, 0xa5 } };

class sample_command : public mainmenu_node_command {
public:
	sample_command( size_t index ) : m_index(index) {
	}
	void get_display(pfc::string_base & text, t_uint32 & flags) override {
		flags = 0;
		text = PFC_string_formatter() << "Test dynamic item #" << m_index;
	}
	void execute(service_ptr_t<service_base> callback) override {
		popup_message::g_show(PFC_string_formatter() << "Invoked test menu item #" << m_index, "Information");
	}
	GUID get_guid() override {
		// This method returns our subcommand ID.
		// Dynamic commands are identified by a pair of GUIDs:
		// - command ID ( see: mainmenu_commands interface ) 
		// - subcommand ID ( identifier of one of the dynamic items )
		// Subcommand identifiers don't have to be actually globally unique,
		// as long as they're unique among the subcommand identifiers for this command ID.

		// In our case, we'll just create a makeshift GUID from a hash of the index. 
		// This is perfectly okay for production code - as long as your command ID is a proper GUID!
		
		// Don't ever hash size_t which varies with CPU architecture
		// Integer endianness intentionally disregarded
		uint32_t hashme = (uint32_t) m_index;

		auto api = hasher_md5::get();
		hasher_md5_state state;
		api->initialize( state );
		api->process( state, &hashme, sizeof(hashme) );

		// fb2k hasher_md5 API even provides a convenient method to return MD5 hashes cast to GUIDs for this.
		return api->get_result_guid( state );
	}
	bool get_description(pfc::string_base & out) {
		out = PFC_string_formatter() << "This is a test menu item #" << m_index << ".";
		return true;
	}
private:
	const size_t m_index;
};
class sample_group : public mainmenu_node_group {
public:
	sample_group() {
		m_children.resize(11);
		for( size_t walk = 0; walk < m_children.size(); ++ walk ) {
			mainmenu_node::ptr node;
			// Insert separators for odd items, commands for even
			if ( walk % 2 ) {
				node = fb2k::service_new<mainmenu_node_separator>();
			} else {
				auto cmdIndex = walk/2 + 1;
				node = fb2k::service_new<sample_command>( cmdIndex );
			}
			m_children[walk] = std::move(node);
		}
	}
	void get_display(pfc::string_base & text, t_uint32 & flags) override {
		flags = 0;
		text = "Dynamic menu test group";
	}
	t_size get_children_count() override {
		return m_children.size();
	}
	mainmenu_node::ptr get_child(t_size index) override {
		PFC_ASSERT( index < m_children.size() );
		return m_children[index];
	}
private:
	std::vector<mainmenu_node::ptr> m_children;
	
};

class mainmenu_sample_dynamic : public mainmenu_commands_v2 {
public:
	// mainmenu_commands_v2 methods
	t_uint32 get_command_count() override { return 1; }
	GUID get_command(t_uint32 p_index) override {return guid_menucommand;}
	void get_name(t_uint32 p_index,pfc::string_base & p_out) override {p_out = "Dynamic menu test";}
	
	bool get_description(t_uint32 p_index,pfc::string_base & p_out) override {
		// Should not get here much
		p_out = "This is a dynamic menu command test."; 
		return true; 
	}
	GUID get_parent() override {return guid_mainmenu_group_id; }
	void execute(t_uint32 p_index,service_ptr_t<service_base> p_callback) override {
		// Should not get here, someone not aware of our dynamic status tried to invoke us?
	}

	bool is_command_dynamic(t_uint32 index) override { return true; }
	mainmenu_node::ptr dynamic_instantiate(t_uint32 index) override {
		return fb2k::service_new<sample_group>();
	}
	
	bool dynamic_execute(t_uint32 index, const GUID & subID, service_ptr_t<service_base> callback) override {
		// If your component provides a more efficient way to execute the command,
		// than doing full dynamic_instantiate() and walking all the dynamic items to find one with the matching identifier,
		// please implement it here.

		// ... or just skip implementing this method entirely.
		return __super::dynamic_execute( index, subID, callback );
	}
};

static service_factory_single_t<mainmenu_sample_dynamic> g_mainmenu_sample_dynamic;

} // namespace