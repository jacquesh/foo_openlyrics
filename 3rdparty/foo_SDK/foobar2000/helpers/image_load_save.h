#pragma once

namespace fb2k {
	bool imageLoadDialog( pfc::string_base & outFN, HWND wndParent, const char * initDir );

	album_art_data::ptr imageLoadDialog( HWND wndParent, const char * initDir );

	//! bAllowAsync: run file writing offthread. In such case the caller will not be made aware if writing failed. \n
	//! Error popup is shown if actual file writing fails.
	bool imageSaveDialog(album_art_data_ptr content, HWND wndParent, const char * initDir , bool bAllowAsync = true );

	album_art_data::ptr readPictureFile( const char * path, abort_callback & a);
}