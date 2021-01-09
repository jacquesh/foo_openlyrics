#pragma once

namespace ProfileCache {
	inline file::ptr FetchFile(const char * context, const char * name, const char * webURL, t_filetimestamp acceptableAge, abort_callback & abort) {
		const double timeoutVal = 5;

		auto path = core_api::pathInProfile( context );
		auto fsLocal = filesystem::get(path);
		fsLocal->make_directory(path, abort);
		path.add_filename( name );

		bool fetch = false;
		file::ptr fLocal;
		
		try {
			fLocal = fsLocal->openWriteExisting(path, abort, timeoutVal );
			fetch = fLocal->get_timestamp(abort) < filetimestamp_from_system_timer() - acceptableAge;
		} catch(exception_io_not_found) {
			fLocal = fsLocal->openWriteNew(path, abort, timeoutVal);
			fetch = true;
		}
		if (fetch) {
			try {
				fLocal->resize(0, abort);
				file::ptr fRemote;
				filesystem::g_open(fRemote, webURL, filesystem::open_mode_read, abort);
				file::g_transfer_file(fRemote, fLocal, abort );
			} catch(exception_io) {
				fLocal.release();
				try { 
					retryOnSharingViolation(timeoutVal, abort, [&] {fsLocal->remove(path, abort);} );
				} catch(...) {}
				throw;
			}
			fLocal->seek(0, abort);
		}
		return fLocal;
	}
};
