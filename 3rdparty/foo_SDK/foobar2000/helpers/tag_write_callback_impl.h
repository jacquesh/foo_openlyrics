#pragma once

#ifdef PFC_WINDOWS_STORE_APP
#include <pfc/pp-winapi.h>
#endif

#if defined(_WIN32) && defined(FOOBAR2000_MOBILE)
#include <pfc/timers.h>
#include <pfc/string_conv.h>
#endif


class tag_write_callback_impl : public tag_write_callback {
public:
	tag_write_callback_impl(const char * p_origpath) : m_origpath(p_origpath) {
		m_fs = filesystem::get(p_origpath);
	}
	bool open_temp_file(service_ptr_t<file> & p_out,abort_callback & p_abort) {
		pfc::dynamic_assert(m_tempfile.is_empty());
		generate_temp_location_for_file(m_temppath,m_origpath,/*pfc::string_extension(m_origpath)*/ "tmp","retagging temporary file");
		service_ptr_t<file> l_tempfile;
		try {
			openTempFile(l_tempfile, p_abort);
		} catch(exception_io) {return false;}
		p_out = m_tempfile = l_tempfile;
		return true;
	}
	bool got_temp_file() const {return m_tempfile.is_valid();}

	// p_owner must be the only reference to open source file, it will be closed + reopened
	//WARNING: if this errors out, it may leave caller with null file pointer; take appropriate measures not to crash in such cases
	void finalize(service_ptr_t<file> & p_owner,abort_callback & p_abort) {
		if (m_tempfile.is_valid()) {
			m_tempfile->flushFileBuffers_(p_abort);

			if (p_owner.is_valid()) {
				try {
					file::g_copy_creation_time(p_owner, m_tempfile, p_abort);
				} catch (exception_io) {}
			}

			m_tempfile.release();
			p_owner.release();
			handleFileMove(m_temppath, m_origpath, p_abort);
			input_open_file_helper(p_owner,m_origpath,input_open_info_write,p_abort);
		}
	}
	// Alternate finalizer without owner file object, caller takes responsibility for closing the source file before calling
	void finalize_no_reopen( abort_callback & p_abort ) {
		if (m_tempfile.is_valid()) {
			m_tempfile->flushFileBuffers_(p_abort);
			m_tempfile.release();
			handleFileMove(m_temppath, m_origpath, p_abort);
		}
	}
	void handle_failure() throw() {
		if (m_tempfile.is_valid()) {
			m_tempfile.release();
			try {
				retryOnSharingViolation( 1, fb2k::noAbort, [&] {
					m_fs->remove(m_temppath, fb2k::noAbort);
				} );
			} catch(...) {}
		}
	}
private:
	void openTempFile(file::ptr & out, abort_callback & abort) {
		out = m_fs->openWriteNew(m_temppath, abort, 1 );
	}
	void handleFileMove(const char * from, const char * to, abort_callback & abort) {
		PFC_ASSERT(m_fs->is_our_path(from));
		PFC_ASSERT(m_fs->is_our_path(to));
		FB2K_RETRY_FILE_MOVE(m_fs->replace_file(from, to, abort), abort, 10 );
	}
	pfc::string8 m_origpath;
	pfc::string8 m_temppath;
	service_ptr_t<file> m_tempfile;
	filesystem::ptr m_fs;
};
