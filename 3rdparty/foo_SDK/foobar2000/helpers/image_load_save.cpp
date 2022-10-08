#include "StdAfx.h"
#include "image_load_save.h"
#include <memory>
#include "../SDK/imageLoaderLite.h"

namespace fb2k {
	bool imageSaveDialog(album_art_data_ptr content, HWND wndParent, const char* initDir, bool bAsync) {
		pfc::string8 fileTypes = "All files|*.*";
		pfc::string8 ext;

		try {
			auto info = fb2k::imageLoaderLite::get()->getInfo(content);
			if (info.formatName) {
				pfc::string8 nameCapitalized = pfc::stringToUpper( info.formatName );
				ext = pfc::stringToLower( info.formatName );
				if (nameCapitalized == "WEBP") nameCapitalized = "WebP";
				pfc::string8 extmask;
				if (ext == "jpeg") {
					ext = "jpg";
					extmask = "*.jpg;*.jpeg";
				} else {
					extmask << "*." << ext;
				}
				fileTypes.reset();
				fileTypes << nameCapitalized << " files|" << extmask;
			}
		} catch (...) {}
		pfc::string8 fn;

		if (!uGetOpenFileName(wndParent, fileTypes, 0, ext.length() > 0 ? ext.c_str() : nullptr, "Export picture file", initDir, fn, TRUE)) return false;

		auto bErrord = std::make_shared<bool>(false);
		auto work = [content, fn, bErrord] {
			try {
				auto f = fileOpenWriteNew(fn, fb2k::noAbort, 0.5);
				f->write(content->get_ptr(), content->get_size(), fb2k::noAbort);
			} catch(std::exception const & e) {
				* bErrord = true;
				pfc::string8 msg;
				msg << "Image file could not be written: " << e;
				fb2k::inMainThread([msg] {
					popup_message::g_show(msg, "Information");
				});
			}
		};
		if (bAsync) {
			fb2k::splitTask(work);
			return true;
		} else {
			work();
			return ! *bErrord;
		}
	}

	bool imageLoadDialog(pfc::string_base& outFN, HWND wndParent, const char* initDir) {
		return !!uGetOpenFileName(wndParent, FB2K_GETOPENFILENAME_PICTUREFILES_ALL, 0, nullptr, "Import picture file", initDir, outFN, FALSE);
	}
	album_art_data::ptr imageLoadDialog(HWND wndParent, const char* initDir) {
		album_art_data::ptr ret;
		pfc::string8 fn;
		if (imageLoadDialog(fn, wndParent, initDir)) {
			try {
				ret = readPictureFile(fn, fb2k::noAbort);
			} catch (std::exception const& e) {
				popup_message::g_show(PFC_string_formatter() << "Image file could not be read: " << e, "Information");
			}
		}
		return ret;
	}

	album_art_data_ptr readPictureFile(const char* p_path, abort_callback& p_abort) {
		PFC_ASSERT(p_path != nullptr);
		PFC_ASSERT(*p_path != 0);
		p_abort.check();
		
		// Pointless, not a media file, path often from openfiledialog and not canonical
		// file_lock_ptr lock = file_lock_manager::get()->acquire_read(p_path, p_abort);
		
		file_ptr l_file;
		filesystem::g_open_timeout(l_file, p_path, filesystem::open_mode_read, 0.5, p_abort);
		service_ptr_t<album_art_data_impl> instance = new service_impl_t<album_art_data_impl>();
		t_filesize size = l_file->get_size_ex(p_abort);
		if (size > 1024 * 1024 * 64) throw std::runtime_error("File too large");
		instance->from_stream(l_file.get_ptr(), (t_size)size, p_abort);
		return instance;
	}

}