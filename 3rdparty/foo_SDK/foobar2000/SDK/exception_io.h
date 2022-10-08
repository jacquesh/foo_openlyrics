#pragma once


namespace foobar2000_io
{
	//! Generic I/O error. Root class for I/O failure exception. See relevant default message for description of each derived exception class.
	PFC_DECLARE_EXCEPTION(exception_io, pfc::exception, "I/O error");
	//! Object not found.
	PFC_DECLARE_EXCEPTION(exception_io_not_found, exception_io, "Object not found");
	//! Access denied. \n
	//! Special Windows note: this MAY be thrown instead of exception_io_sharing_violation by operations that rename/move files due to Win32 MoveFile() bugs.
	PFC_DECLARE_EXCEPTION(exception_io_denied, exception_io, "Access denied");
	//! Access denied.
	PFC_DECLARE_EXCEPTION(exception_io_denied_readonly, exception_io_denied, "File is read-only");
	//! Unsupported format or corrupted file (unexpected data encountered).
	PFC_DECLARE_EXCEPTION(exception_io_data, exception_io, "Unsupported format or corrupted file");
	//! Unsupported format or corrupted file (truncation encountered).
	PFC_DECLARE_EXCEPTION(exception_io_data_truncation, exception_io_data, "Unsupported format or corrupted file");
	//! Unsupported format (a subclass of "unsupported format or corrupted file" exception).
	PFC_DECLARE_EXCEPTION(exception_io_unsupported_format, exception_io_data, "Unsupported file format");
	//! Decode error - subsong index out of expected range
	PFC_DECLARE_EXCEPTION(exception_io_bad_subsong_index, exception_io_data, "Unexpected subsong index");
	//! Object is remote, while specific operation is supported only for local objects.
	PFC_DECLARE_EXCEPTION(exception_io_object_is_remote, exception_io, "This operation is not supported on remote objects");
	//! Sharing violation.
	PFC_DECLARE_EXCEPTION(exception_io_sharing_violation, exception_io, "File is already in use");
	//! Device full.
	PFC_DECLARE_EXCEPTION(exception_io_device_full, exception_io, "Device full");
	//! Attempt to seek outside valid range.
	PFC_DECLARE_EXCEPTION(exception_io_seek_out_of_range, exception_io, "Seek offset out of range");
	//! This operation requires a seekable object.
	PFC_DECLARE_EXCEPTION(exception_io_object_not_seekable, exception_io, "Object is not seekable");
	//! This operation requires an object with known length.
	PFC_DECLARE_EXCEPTION(exception_io_no_length, exception_io, "Length of object is unknown");
	//! Invalid path.
	PFC_DECLARE_EXCEPTION(exception_io_no_handler_for_path, exception_io, "Invalid path");
	//! Object already exists.
	PFC_DECLARE_EXCEPTION(exception_io_already_exists, exception_io, "Object already exists");
	//! Pipe error.
	PFC_DECLARE_EXCEPTION(exception_io_no_data, exception_io, "The process receiving or sending data has terminated");
	//! Network not reachable.
	PFC_DECLARE_EXCEPTION(exception_io_network_not_reachable, exception_io, "Network not reachable");
	//! Media is write protected.
	PFC_DECLARE_EXCEPTION(exception_io_write_protected, exception_io_denied, "The media is write protected");
	//! File is corrupted. This indicates filesystem call failure, not actual invalid data being read by the app.
	PFC_DECLARE_EXCEPTION(exception_io_file_corrupted, exception_io, "The file is corrupted");
	//! The disc required for requested operation is not available.
	PFC_DECLARE_EXCEPTION(exception_io_disk_change, exception_io, "Disc not available");
	//! The directory is not empty.
	PFC_DECLARE_EXCEPTION(exception_io_directory_not_empty, exception_io, "Directory not empty");
	//! A network connectivity error
	PFC_DECLARE_EXCEPTION(exception_io_net, exception_io, "Network error");
	//! A network security error
	PFC_DECLARE_EXCEPTION(exception_io_net_security, exception_io_net, "Network security error");
	//! A network connectivity error, specifically a DNS query failure
	PFC_DECLARE_EXCEPTION(exception_io_dns, exception_io_net, "DNS error");
	//! The path does not point to a directory.
	PFC_DECLARE_EXCEPTION(exception_io_not_directory, exception_io, "Not a directory");
	//! Functionality not supported by this device or file system.
	PFC_DECLARE_EXCEPTION(exception_io_unsupported_feature, exception_io, "Unsupported feature");

#ifdef _WIN32
	PFC_NORETURN void exception_io_from_win32(DWORD p_code);
#define WIN32_IO_OP(X) {SetLastError(NO_ERROR); if (!(X)) exception_io_from_win32(GetLastError());}

	// SPECIAL WORKAROUND: throw "file is read-only" rather than "access denied" where appropriate
	PFC_NORETURN void win32_file_write_failure(DWORD p_code, const char* path);
#else

    PFC_NORETURN void exception_io_from_nix(int code);
    PFC_NORETURN void nix_io_op_fail();
    void nix_pre_io_op();
#define NIX_IO_OP(X) { if (!(X)) nix_io_op_fail();}

#endif

}
