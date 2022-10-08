#pragma once

namespace fb2k {
	//! \since 1.6.2
	class imageViewer : public service_base {
		FB2K_MAKE_SERVICE_COREAPI(imageViewer);
	public:
		//! Spawns an image viewer window, showing the specified picture already loaded into application memory.
		virtual void show(HWND parent, fb2k::memBlockRef data) = 0;
		//! Spawns an image viewer window, showing album art from the specified list of items.
		//! @param aaType Type of picture to load, front cover, back cover or other.
		//! @param pageno Reserved for future use, set to 0.
		virtual void load_and_show(HWND parent, metadb_handle_list_cref items, const GUID & aaType, unsigned pageno = 0) = 0;
	};
}