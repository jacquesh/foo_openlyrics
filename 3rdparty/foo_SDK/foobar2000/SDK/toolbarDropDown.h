#pragma once

namespace fb2k {
	
	//! \since 2.0
	class toolbarDropDownNotify {
	public:
		virtual void contentChanged() = 0;
		virtual void selectionChanged() = 0;
	protected:
		~toolbarDropDownNotify() {}
	};

	//! \since 2.0
	class toolbarDropDown : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(toolbarDropDown)
	public:
		virtual GUID getGuid() = 0;
		virtual void getShortName(pfc::string_base& out) = 0; // name to appear in toolbar
		virtual void getLongName(pfc::string_base& out) = 0; // long descriptive name
		virtual size_t getNumValues() = 0;
		virtual void getValue(size_t idx, pfc::string_base& out) = 0;
		virtual void setSelectedIndex(size_t) = 0;
		virtual size_t getSelectedIndex() = 0;
		virtual void addNotify(toolbarDropDownNotify*) = 0;
		virtual void removeNotify(toolbarDropDownNotify*) = 0;
		virtual void onDropDown() = 0;
	};
}
