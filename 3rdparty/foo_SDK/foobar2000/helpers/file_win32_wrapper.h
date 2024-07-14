#pragma once

#include <libPPUI/win32_op.h>
#include <pfc/string-conv-lite.h>

#ifdef _WIN32
namespace file_win32_helpers {
	t_filesize get_size(HANDLE p_handle);
	void seek(HANDLE p_handle,t_sfilesize p_position,file::t_seek_mode p_mode);
	void fillOverlapped(OVERLAPPED & ol, HANDLE myEvent, t_filesize s);
	void writeOverlappedPass(HANDLE handle, HANDLE myEvent, t_filesize position, const void * in,DWORD inBytes, abort_callback & abort);
	void writeOverlapped(HANDLE handle, HANDLE myEvent, t_filesize & position, const void * in, size_t inBytes, abort_callback & abort);
	void writeStreamOverlapped(HANDLE handle, HANDLE myEvent, const void * in, size_t inBytes, abort_callback & abort);
	DWORD readOverlappedPass(HANDLE handle, HANDLE myEvent, t_filesize position, void * out, DWORD outBytes, abort_callback & abort);
	size_t readOverlapped(HANDLE handle, HANDLE myEvent, t_filesize & position, void * out, size_t outBytes, abort_callback & abort);
	size_t readStreamOverlapped(HANDLE handle, HANDLE myEvent, void * out, size_t outBytes, abort_callback & abort);
	HANDLE createFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, abort_callback & abort);
	size_t lowLevelIO(HANDLE hFile, const GUID & guid, size_t arg1, void * arg2, size_t arg2size, bool canWrite, abort_callback & abort);
	bool querySeekPenalty(const char * fb2k_path, bool & out);
	bool querySeekPenalty(const wchar_t * nativePath, bool & out);

	static uint64_t make_uint64(t_uint32 p_low, t_uint32 p_high) {
		return ((t_uint64)p_low) + ((t_uint64)p_high << 32);
	}
	static uint64_t make_uint64(LARGE_INTEGER const& i) {
		return reinterpret_cast<uint64_t const&>(i);
	}
	static uint64_t make_uint64(FILETIME const& ft) {
		return reinterpret_cast<uint64_t const&>(ft);
//		return make_uint64(ft.dwLowDateTime, ft.dwHighDateTime);
	}

	template<typename t_info>
	static t_filestats translate_stats(const t_info& p_info) {
		t_filestats ret;
		ret.m_size = make_uint64(p_info.nFileSizeLow, p_info.nFileSizeHigh);
		ret.m_timestamp = make_uint64(p_info.ftLastWriteTime);
		return ret;
	}

	void attribs_from_win32(t_filestats2& out, DWORD in);
	template<typename t_info>
	static t_filestats2 translate_stats2(const t_info& p_info) {
		t_filestats2 ret;
		ret.m_size = make_uint64(p_info.nFileSizeLow, p_info.nFileSizeHigh);
		ret.m_timestamp = make_uint64(p_info.ftLastWriteTime);
		ret.m_timestampCreate = make_uint64(p_info.ftCreationTime);
		attribs_from_win32(ret, p_info.dwFileAttributes);
		return ret;
	}

	t_filestats2 stats2_from_handle(HANDLE, const wchar_t * fallbackPath, uint32_t flags, abort_callback &a);
};


template<bool p_seekable,bool p_writeable>
class file_win32_wrapper_t : public service_multi_inherit<file_v2, file_lowLevelIO> {
	typedef file_win32_wrapper_t<p_seekable, p_writeable> self_t;
public:
	file_win32_wrapper_t(HANDLE handle, pfc::wstringLite && path) : m_handle(handle), m_path(std::move(path)) {}

	static file::ptr g_CreateFile(const char * p_path,DWORD p_access,DWORD p_sharemode,LPSECURITY_ATTRIBUTES p_security_attributes,DWORD p_createmode,DWORD p_flags,HANDLE p_template) {
		auto pathW = pfc::wideFromUTF8(p_path);
		SetLastError(NO_ERROR);
		HANDLE handle = CreateFile(pathW,p_access,p_sharemode,p_security_attributes,p_createmode,p_flags,p_template);
		if (handle == INVALID_HANDLE_VALUE) {
			const DWORD code = GetLastError();
			if (p_access & GENERIC_WRITE) win32_file_write_failure(code, p_path);
			else exception_io_from_win32(code);
		}
		try {
			return g_create_from_handle(handle, std::move(pathW));
		} catch(...) {CloseHandle(handle); throw;}
	}

	static service_ptr_t<file> g_create_from_handle(HANDLE handle, pfc::wstringLite && path) {
		return new service_impl_t<self_t>(handle, std::move(path));
	}
	static service_ptr_t<file> g_create_from_handle(HANDLE handle) {
		pfc::wstringLite blank;
		g_create_from_handle(handle, std::move(blank));
	}


	void reopen(abort_callback & p_abort) {seek(0,p_abort);}

	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();

		PFC_STATIC_ASSERT(sizeof(t_size) >= sizeof(DWORD));

		t_size bytes_written_total = 0;

		if (sizeof(t_size) == sizeof(DWORD)) {
			p_abort.check_e();
			DWORD bytes_written = 0;
			SetLastError(ERROR_SUCCESS);
			if (!WriteFile(m_handle,p_buffer,(DWORD)p_bytes,&bytes_written,0)) exception_io_from_win32(GetLastError());
			if (bytes_written != p_bytes) throw exception_io("Write failure");
			bytes_written_total = bytes_written;
			m_position += bytes_written;
		} else {
			while(bytes_written_total < p_bytes) {
				p_abort.check_e();
				DWORD bytes_written = 0;
				DWORD delta = (DWORD) pfc::min_t<t_size>(p_bytes - bytes_written_total, 0x80000000u);
				SetLastError(ERROR_SUCCESS);
				if (!WriteFile(m_handle,(const t_uint8*)p_buffer + bytes_written_total,delta,&bytes_written,0)) exception_io_from_win32(GetLastError());
				if (bytes_written != delta) throw exception_io("Write failure");
				bytes_written_total += bytes_written;
				m_position += bytes_written;
			}
		}
	}
	
	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		PFC_STATIC_ASSERT(sizeof(t_size) >= sizeof(DWORD));
		
		t_size bytes_read_total = 0;
		if (sizeof(t_size) == sizeof(DWORD)) {
			p_abort.check_e();
			DWORD bytes_read = 0;
			SetLastError(ERROR_SUCCESS);
			if (!ReadFile(m_handle,p_buffer,pfc::downcast_guarded<DWORD>(p_bytes),&bytes_read,0)) exception_io_from_win32(GetLastError());
			bytes_read_total = bytes_read;
			m_position += bytes_read;
		} else {
			while(bytes_read_total < p_bytes) {
				p_abort.check_e();
				DWORD bytes_read = 0;
				DWORD delta = (DWORD) pfc::min_t<t_size>(p_bytes - bytes_read_total, 0x80000000u);
				SetLastError(ERROR_SUCCESS);
				if (!ReadFile(m_handle,(t_uint8*)p_buffer + bytes_read_total,delta,&bytes_read,0)) exception_io_from_win32(GetLastError());
				bytes_read_total += bytes_read;
				m_position += bytes_read;
				if (bytes_read != delta) break;
			}
		}
		return bytes_read_total;
	}


	t_filesize get_size(abort_callback & p_abort) {
		p_abort.check_e();
		return file_win32_helpers::get_size(m_handle);
	}

	t_filesize get_position(abort_callback & p_abort) {
		p_abort.check_e();
		return m_position;
	}
	
	void resize(t_filesize p_size,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();
		p_abort.check_e();
		if (m_position != p_size) {
			file_win32_helpers::seek(m_handle,p_size,file::seek_from_beginning);
		}
		SetLastError(ERROR_SUCCESS);
		if (!SetEndOfFile(m_handle)) {
			DWORD code = GetLastError();
			if (m_position != p_size) try {file_win32_helpers::seek(m_handle,m_position,file::seek_from_beginning);} catch(...) {}
			exception_io_from_win32(code);
		}
		if (m_position > p_size) m_position = p_size;
		if (m_position != p_size) file_win32_helpers::seek(m_handle,m_position,file::seek_from_beginning);
	}


	void seek(t_filesize p_position,abort_callback & p_abort) {
		if (!p_seekable) throw exception_io_object_not_seekable();
		p_abort.check_e();
		if (p_position > file_win32_helpers::get_size(m_handle)) throw exception_io_seek_out_of_range();
		file_win32_helpers::seek(m_handle,p_position,file::seek_from_beginning);
		m_position = p_position;
	}

	bool can_seek() {return p_seekable;}
	bool get_content_type(pfc::string_base & out) {return false;}
	bool is_in_memory() {return false;}
	void on_idle(abort_callback & p_abort) {p_abort.check_e();}
	
	t_filestats2 get_stats2(uint32_t f, abort_callback& a) {
		a.check();
		if (p_writeable) FlushFileBuffers(m_handle);
		return file_win32_helpers::stats2_from_handle(m_handle, m_path, f, a);
	}
	t_filetimestamp get_timestamp(abort_callback & p_abort) {
		p_abort.check_e();
		if (p_writeable) FlushFileBuffers(m_handle);
		SetLastError(ERROR_SUCCESS);
		FILETIME temp;
		if (!GetFileTime(m_handle,0,0,&temp)) exception_io_from_win32(GetLastError());
		return file_win32_helpers::make_uint64(temp);
	}

	bool is_remote() {return false;}
	~file_win32_wrapper_t() {CloseHandle(m_handle);}

	size_t lowLevelIO(const GUID & guid, size_t arg1, void * arg2, size_t arg2size, abort_callback & abort) override {
		return file_win32_helpers::lowLevelIO(m_handle, guid, arg1, arg2, arg2size, p_writeable, abort);
	}
protected:
	HANDLE m_handle;
	t_filesize m_position = 0;
	pfc::wstringLite m_path;
};

template<bool p_writeable>
class file_win32_wrapper_overlapped_t : public service_multi_inherit< file_v2, file_lowLevelIO > {
	typedef file_win32_wrapper_overlapped_t<p_writeable> self_t;
public:
	file_win32_wrapper_overlapped_t(HANDLE file, pfc::wstringLite && path) : m_handle(file), m_path(std::move(path))  {
		WIN32_OP( (m_event = CreateEvent(NULL, TRUE, FALSE, NULL)) != NULL );
	}
	~file_win32_wrapper_overlapped_t() {CloseHandle(m_event); CloseHandle(m_handle);}
	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();
		return file_win32_helpers::writeOverlapped(m_handle, m_event, m_position, p_buffer, p_bytes, p_abort);
	}
	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		return file_win32_helpers::readOverlapped(m_handle, m_event, m_position, p_buffer, p_bytes, p_abort);
	}

	void reopen(abort_callback & p_abort) {seek(0,p_abort);}


	t_filesize get_size(abort_callback & p_abort) {
		p_abort.check_e();
		return file_win32_helpers::get_size(m_handle);
	}

	t_filesize get_position(abort_callback & p_abort) {
		p_abort.check_e();
		return m_position;
	}
	
	void resize(t_filesize p_size,abort_callback & p_abort) {
		if (!p_writeable) throw exception_io_denied();
		p_abort.check_e();
		file_win32_helpers::seek(m_handle,p_size,file::seek_from_beginning);
		SetLastError(ERROR_SUCCESS);
		if (!SetEndOfFile(m_handle)) {
			DWORD code = GetLastError();
			exception_io_from_win32(code);
		}
		if (m_position > p_size) m_position = p_size;
	}


	void seek(t_filesize p_position,abort_callback & p_abort) {
		p_abort.check_e();
		if (p_position > file_win32_helpers::get_size(m_handle)) throw exception_io_seek_out_of_range();
		// file_win32_helpers::seek(m_handle,p_position,file::seek_from_beginning);
		m_position = p_position;
	}

	bool can_seek() {return true;}
	bool get_content_type(pfc::string_base & out) {return false;}
	bool is_in_memory() {return false;}
	void on_idle(abort_callback & p_abort) {p_abort.check_e();}
	

	t_filestats2 get_stats2(uint32_t f, abort_callback& a) {
		a.check();
		if (p_writeable) FlushFileBuffers(m_handle);
		return file_win32_helpers::stats2_from_handle(m_handle, m_path, f, a);
	}

	t_filetimestamp get_timestamp(abort_callback & p_abort) {
		p_abort.check_e();
		if (p_writeable) FlushFileBuffers(m_handle);
		SetLastError(ERROR_SUCCESS);
		FILETIME temp;
		if (!GetFileTime(m_handle,0,0,&temp)) exception_io_from_win32(GetLastError());
		return file_win32_helpers::make_uint64(temp);
	}

	bool is_remote() {return false;}
	

	static file::ptr g_CreateFile(const char * p_path,DWORD p_access,DWORD p_sharemode,LPSECURITY_ATTRIBUTES p_security_attributes,DWORD p_createmode,DWORD p_flags,HANDLE p_template) {
		auto pathW = pfc::wideFromUTF8(p_path);
		p_flags |= FILE_FLAG_OVERLAPPED;
		SetLastError(NO_ERROR);
		HANDLE handle = CreateFile(pathW,p_access,p_sharemode,p_security_attributes,p_createmode,p_flags,p_template);
		if (handle == INVALID_HANDLE_VALUE) {
			const DWORD code = GetLastError();
			if (p_access & GENERIC_WRITE) win32_file_write_failure(code, p_path);
			else exception_io_from_win32(code);
		}
		try {
			return g_create_from_handle(handle, std::move(pathW));
		} catch(...) {CloseHandle(handle); throw;}
	}

	static file::ptr g_create_from_handle(HANDLE p_handle, pfc::wstringLite && path) {
		return new service_impl_t<self_t>(p_handle, std::move(path));
	}
	static file::ptr g_create_from_handle(HANDLE p_handle) {
		pfc::wstringLite blank;
		return g_create_from_handle(p_handle, std::move(blank));
	}

	size_t lowLevelIO(const GUID & guid, size_t arg1, void * arg2, size_t arg2size, abort_callback & abort) override {
		return file_win32_helpers::lowLevelIO(m_handle, guid, arg1, arg2, arg2size, p_writeable, abort);
	}

protected:
	HANDLE m_event, m_handle;
	t_filesize m_position = 0;
	pfc::wstringLite m_path;
};

#endif // _WIN32
