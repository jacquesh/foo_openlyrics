#pragma once

#include "exception_io.h"

class file_info;
class mem_block_container;

//! Contains various I/O related structures and interfaces.
namespace foobar2000_io
{
	//! Type used for file size related variables.
	typedef t_uint64 t_filesize;
	//! Type used for file size related variables when a signed value is needed.
	typedef t_int64 t_sfilesize;
	//! Type used for file timestamp related variables. 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601; 0 for invalid/unknown time.
	typedef t_uint64 t_filetimestamp;
	//! Invalid/unknown file timestamp constant. Also see: t_filetimestamp.
	const t_filetimestamp filetimestamp_invalid = 0;
	//! Invalid/unknown file size constant. Also see: t_filesize.
	static constexpr t_filesize filesize_invalid = (t_filesize)(UINT64_MAX);

	static constexpr t_filetimestamp filetimestamp_1second_increment = 10000000;

	//! Stores file stats (size and timestamp).
	struct t_filestats {
		//! Size of the file.
		t_filesize m_size = filesize_invalid;
		//! Time of last file modification.
		t_filetimestamp m_timestamp = filetimestamp_invalid;

		inline bool operator==(const t_filestats& param) const { return m_size == param.m_size && m_timestamp == param.m_timestamp; }
		inline bool operator!=(const t_filestats& param) const { return m_size != param.m_size || m_timestamp != param.m_timestamp; }
	};

	struct t_filestats2 {
		//! Size of the file.
		t_filesize m_size = filesize_invalid;
		//! Time of last file modification.
		t_filetimestamp m_timestamp = filetimestamp_invalid;
		t_filetimestamp m_timestampCreate = filetimestamp_invalid;
		uint32_t m_attribs = 0;
		uint32_t m_attribsValid = 0;

		t_filestats const& as_legacy() const { return *reinterpret_cast<const t_filestats*>(this); }
		t_filestats to_legacy() const { return { m_size, m_timestamp }; }
		static t_filestats2 from_legacy(t_filestats const& in) { return { in.m_size, in.m_timestamp }; }

		enum {
			attr_readonly = 1 << 0,
			attr_folder = 1 << 1,
			attr_hidden = 1 << 2,
			attr_system = 1 << 3,
			attr_remote = 1 << 4,
		};
		bool is_set(uint32_t attr) const {
			PFC_ASSERT(m_attribsValid & attr);
			return (m_attribs & attr) != 0;
		}
		bool is_readonly() const { return is_set(attr_readonly); }
		bool is_folder() const { return is_set(attr_folder); }
		bool is_file() const { return !is_folder(); }
		bool is_hidden() const { return is_set(attr_hidden); }
		bool can_write() const { return !is_readonly(); }
		bool is_system() const { return is_set(attr_system); }
		bool is_remote() const { return is_set(attr_remote); }

		void set_attrib(uint32_t f, bool v) {
			if (v) m_attribs |= f;
			else m_attribs &= ~f;
			m_attribsValid |= f;
		}
		void set_file(bool v = true) { set_folder(!v); }
		void set_folder(bool v = true) { set_attrib(attr_folder, v); }
		void set_readonly(bool v) { set_attrib(attr_readonly, v); }
		void set_hidden(bool v) { set_attrib(attr_hidden, v); }
		void set_system(bool v) { set_attrib(attr_system, v); }
		void set_remote(bool v = true) { set_attrib(attr_remote, v); }

		static pfc::string8 format_attribs(uint32_t flags, const char* delim = ", ");
		pfc::string8 format_attribs(const char* delim = ", ") const { return format_attribs(m_attribs, delim); }

		static bool equals(t_filestats2 const& v1, t_filestats2 const& v2) {
			return v1.m_size == v2.m_size && v1.m_timestamp == v2.m_timestamp && v1.m_timestampCreate == v2.m_timestampCreate && v1.m_attribsValid == v2.m_attribsValid && v1.m_attribs == v2.m_attribs;
		}
		bool operator==(const t_filestats2& other) const { return equals(*this, other); }
		bool operator!=(const t_filestats2& other) const { return !equals(*this, other); }

		pfc::string8 describe() const;
	};
	enum {
		stats2_size = 1 << 0,
		stats2_timestamp = 1 << 1,
		stats2_timestampCreate = 1 << 2,
		stats2_fileOrFolder = 1 << 3,
		stats2_readOnly = 1 << 4,
		stats2_canWrite = stats2_readOnly,
		stats2_hidden = 1 << 5,
		stats2_remote = 1 << 6,
		stats2_flags = (stats2_fileOrFolder | stats2_readOnly | stats2_hidden | stats2_remote),
		stats2_legacy = (stats2_size | stats2_timestamp),
		stats2_all = 0xFFFFFFFF,
	};

	//! Invalid/unknown file stats constant. See: t_filestats.
	static constexpr t_filestats filestats_invalid = t_filestats();
	static constexpr t_filestats2 filestats2_invalid = t_filestats2();

	//! Struct to be used with guid_getFileTimes / guid_setFileTimes.
	struct filetimes_t {
		t_filetimestamp creation = filetimestamp_invalid;
		t_filetimestamp lastAccess = filetimestamp_invalid;
		t_filetimestamp lastWrite = filetimestamp_invalid;
	};

	//! Generic interface to read data from a nonseekable stream. Also see: stream_writer, file.	\n
	//! Error handling: all methods may throw exception_io or one of derivatives on failure; exception_aborted when abort_callback is signaled.
	class NOVTABLE stream_reader {
	public:
		//! Attempts to reads specified number of bytes from the stream.
		//! @param p_buffer Receives data being read. Must have at least p_bytes bytes of space allocated.
		//! @param p_bytes Number of bytes to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Number of bytes actually read. May be less than requested when EOF was reached.
		virtual t_size read(void* p_buffer, t_size p_bytes, abort_callback& p_abort) = 0;
		//! Reads specified number of bytes from the stream. If requested amount of bytes can't be read (e.g. EOF), throws exception_io_data_truncation.
		//! @param p_buffer Receives data being read. Must have at least p_bytes bytes of space allocated.
		//! @param p_bytes Number of bytes to read.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void read_object(void* p_buffer, t_size p_bytes, abort_callback& p_abort);
		//! Attempts to skip specified number of bytes in the stream.
		//! @param p_bytes Number of bytes to skip.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Number of bytes actually skipped, May be less than requested when EOF was reached.
		virtual t_filesize skip(t_filesize p_bytes, abort_callback& p_abort);
		//! Skips specified number of bytes in the stream. If requested amount of bytes can't be skipped (e.g. EOF), throws exception_io_data_truncation.
		//! @param p_bytes Number of bytes to skip.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void skip_object(t_filesize p_bytes, abort_callback& p_abort);

		//! Helper template built around read_object. Reads single raw object from the stream.
		//! @param p_object Receives object read from the stream on success.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void read_object_t(T& p_object, abort_callback& p_abort) { pfc::assert_raw_type<T>(); read_object(&p_object, sizeof(p_object), p_abort); }
		template<typename T> inline T read_object_t(abort_callback& a) { T val; this->read_object_t(val, a); return val; }
		//! Helper template built around read_object. Reads single raw object from the stream; corrects byte order assuming stream uses little endian order.
		//! @param p_object Receives object read from the stream on success.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void read_lendian_t(T& p_object, abort_callback& p_abort) { read_object_t(p_object, p_abort); byte_order::order_le_to_native_t(p_object); }
		template<typename T> inline T read_lendian_t(abort_callback& a) { T val; this->read_lendian_t(val, a); return val; }
		//! Helper template built around read_object. Reads single raw object from the stream; corrects byte order assuming stream uses big endian order.
		//! @param p_object Receives object read from the stream on success.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void read_bendian_t(T& p_object, abort_callback& p_abort) { read_object_t(p_object, p_abort); byte_order::order_be_to_native_t(p_object); }
		template<typename T> inline T read_bendian_t(abort_callback& a) { T val; this->read_bendian_t(val, a); return val; }

		//! Helper function; reads a string (with a 32-bit header indicating length in bytes followed by UTF-8 encoded data without a null terminator).
		void read_string(pfc::string_base& p_out, abort_callback& p_abort);
		//! Helper function; alternate way of storing strings; assumes string takes space up to end of stream.
		void read_string_raw(pfc::string_base& p_out, abort_callback& p_abort);
		//! Helper function; reads a string (with a 32-bit header indicating length in bytes followed by UTF-8 encoded data without a null terminator).
		pfc::string read_string(abort_callback& p_abort);

		//! Helper function; reads a string of specified length from the stream.
		void read_string_ex(pfc::string_base& p_out, t_size p_bytes, abort_callback& p_abort);
		//! Helper function; reads a string of specified length from the stream.
		pfc::string read_string_ex(t_size p_len, abort_callback& p_abort);

		void read_string_nullterm(pfc::string_base& out, abort_callback& abort);

		t_filesize skip_till_eof(abort_callback& abort);

		template<typename t_outArray>
		void read_till_eof(t_outArray& out, abort_callback& abort) {
			pfc::assert_raw_type<typename t_outArray::t_item>();
			const t_size itemWidth = sizeof(typename t_outArray::t_item);
			out.set_size(pfc::max_t<t_size>(1, 256 / itemWidth)); t_size done = 0;
			for (;;) {
				t_size delta = out.get_size() - done;
				t_size delta2 = read(out.get_ptr() + done, delta * itemWidth, abort) / itemWidth;
				done += delta2;
				if (delta2 != delta) break;
				out.set_size(out.get_size() << 1);
			}
			out.set_size(done);
		}

		uint8_t read_byte(abort_callback& abort);
	protected:
		stream_reader() {}
		~stream_reader() {}
	};


	//! Generic interface to write data to a nonseekable stream. Also see: stream_reader, file.	\n
	//! Error handling: all methods may throw exception_io or one of derivatives on failure; exception_aborted when abort_callback is signaled.
	class NOVTABLE stream_writer {
	public:
		//! Writes specified number of bytes from specified buffer to the stream.
		//! @param p_buffer Buffer with data to write. Must contain at least p_bytes bytes.
		//! @param p_bytes Number of bytes to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void write(const void* p_buffer, t_size p_bytes, abort_callback& p_abort) = 0;

		//! Helper. Same as write(), provided for consistency.
		inline void write_object(const void* p_buffer, t_size p_bytes, abort_callback& p_abort) { write(p_buffer, p_bytes, p_abort); }

		//! Helper template. Writes single raw object to the stream.
		//! @param p_object Object to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void write_object_t(const T& p_object, abort_callback& p_abort) { pfc::assert_raw_type<T>(); write_object(&p_object, sizeof(p_object), p_abort); }
		//! Helper template. Writes single raw object to the stream; corrects byte order assuming stream uses little endian order.
		//! @param p_object Object to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void write_lendian_t(const T& p_object, abort_callback& p_abort) { T temp = p_object; byte_order::order_native_to_le_t(temp); write_object_t(temp, p_abort); }
		//! Helper template. Writes single raw object to the stream; corrects byte order assuming stream uses big endian order.
		//! @param p_object Object to write.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		template<typename T> inline void write_bendian_t(const T& p_object, abort_callback& p_abort) { T temp = p_object; byte_order::order_native_to_be_t(temp); write_object_t(temp, p_abort); }

		//! Helper function; writes string (with 32-bit header indicating length in bytes followed by UTF-8 encoded data without null terminator).
		void write_string(const char* p_string, abort_callback& p_abort);
		void write_string(const char* p_string, t_size p_len, abort_callback& p_abort);

		template<typename T>
		void write_string(const T& val, abort_callback& p_abort) { write_string(pfc::stringToPtr(val), p_abort); }

		//! Helper function; writes raw string to the stream, with no length info or null terminators.
		void write_string_raw(const char* p_string, abort_callback& p_abort);

		void write_string_nullterm(const char* p_string, abort_callback& p_abort) { this->write(p_string, strlen(p_string) + 1, p_abort); }
	protected:
		stream_writer() {}
		~stream_writer() {}
	};

	//! A class providing abstraction for an open file object, with reading/writing/seeking methods. See also: stream_reader, stream_writer (which it inherits read/write methods from). \n
	//! Error handling: all methods may throw exception_io or one of derivatives on failure; exception_aborted when abort_callback is signaled.
	class NOVTABLE file : public service_base, public stream_reader, public stream_writer {
	public:

		//! Seeking mode constants. Note: these are purposedly defined to same values as standard C SEEK_* constants
		enum t_seek_mode {
			//! Seek relative to beginning of file (same as seeking to absolute offset).
			seek_from_beginning = 0,
			//! Seek relative to current position.
			seek_from_current = 1,
			//! Seek relative to end of file.
			seek_from_eof = 2,
		};

		//! Retrieves size of the file.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns File size on success; filesize_invalid if unknown (nonseekable stream etc).
		virtual t_filesize get_size(abort_callback& p_abort) = 0;


		//! Retrieves read/write cursor position in the file. In case of non-seekable stream, this should return number of bytes read so far since open/reopen call.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Read/write cursor position
		virtual t_filesize get_position(abort_callback& p_abort) = 0;

		//! Resizes file to the specified size in bytes.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void resize(t_filesize p_size, abort_callback& p_abort) = 0;

		//! Sets read/write cursor position to the specified offset. Throws exception_io_seek_out_of_range if the specified offset is outside the valid range.
		//! @param p_position position to seek to.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void seek(t_filesize p_position, abort_callback& p_abort) = 0;

		//! Same as seek() but throws exception_io_data instead of exception_io_seek_out_of_range.
		void seek_probe(t_filesize p_position, abort_callback& p_abort);

		//! Sets read/write cursor position to the specified offset; extended form allowing seeking relative to current position or to end of file.
		//! @param p_position Position to seek to; interpretation of this value depends on p_mode parameter.
		//! @param p_mode Seeking mode; see t_seek_mode enum values for further description.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void seek_ex(t_sfilesize p_position, t_seek_mode p_mode, abort_callback& p_abort);

		//! Returns whether the file is seekable or not. If can_seek() returns false, all seek() or seek_ex() calls will fail; reopen() is still usable on nonseekable streams.
		virtual bool can_seek() = 0;

		//! Retrieves mime type of the file.
		//! @param p_out Receives content type string on success.
		virtual bool get_content_type(pfc::string_base& p_out) = 0;

		//! Hint, returns whether the file is already fully buffered into memory.
		virtual bool is_in_memory() { return false; }

		//! Optional, called by owner thread before sleeping.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void on_idle(abort_callback& p_abort) { (void)p_abort; }

		//! Retrieves last modification time of the file.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		//! @returns Last modification time o fthe file; filetimestamp_invalid if N/A.
		virtual t_filetimestamp get_timestamp(abort_callback& p_abort) { (void)p_abort; return filetimestamp_invalid; }

		//! Resets non-seekable stream, or seeks to zero on seekable file.
		//! @param p_abort abort_callback object signaling user aborting the operation.
		virtual void reopen(abort_callback& p_abort) = 0;

		//! Indicates whether the file is a remote resource and non-sequential access may be slowed down by lag. This is typically returns to true on non-seekable sources but may also return true on seekable sources indicating that seeking is supported but will be relatively slow.
		virtual bool is_remote() = 0;

		//! Retrieves file stats structure. Uses get_size() and get_timestamp().
		t_filestats get_stats(abort_callback& p_abort);

		//! Returns whether read/write cursor position is at the end of file.
		bool is_eof(abort_callback& p_abort);

		//! Truncates file to specified size (while preserving read/write cursor position if possible); uses set_eof().
		void truncate(t_filesize p_position, abort_callback& p_abort);

		//! Truncates the file at current read/write cursor position.
		void set_eof(abort_callback& p_abort) { resize(get_position(p_abort), p_abort); }


		//! Helper; retrieves size of the file. If size is not available (get_size() returns filesize_invalid), throws exception_io_no_length.
		t_filesize get_size_ex(abort_callback& p_abort);

		//! Helper; retrieves amount of bytes between read/write cursor position and end of file. Fails when length can't be determined.
		t_filesize get_remaining(abort_callback& p_abort);

		//! Security helper; fails early with exception_io_data_truncation if it is not possible to read this amount of bytes from this file at this position.
		void probe_remaining(t_filesize bytes, abort_callback& p_abort);

		//! Helper; throws exception_io_object_not_seekable if file is not seekable.
		void ensure_seekable();

		//! Helper; throws exception_io_object_is_remote if the file is remote.
		void ensure_local();

		//! Helper; transfers specified number of bytes between streams.
		//! @returns number of bytes actually transferred. May be less than requested if e.g. EOF is reached.
		static t_filesize g_transfer(stream_reader* src, stream_writer* dst, t_filesize bytes, abort_callback& p_abort);
		//! Helper; transfers specified number of bytes between streams. Throws exception if requested number of bytes could not be read (EOF).
		static void g_transfer_object(stream_reader* src, stream_writer* dst, t_filesize bytes, abort_callback& p_abort);
		//! Helper; transfers entire file content from one file to another, erasing previous content.
		static void g_transfer_file(const service_ptr_t<file>& p_from, const service_ptr_t<file>& p_to, abort_callback& p_abort);
		//! Helper; transfers file modification times from one file to another, if supported by underlying objects. Returns true on success, false if the operation doesn't appear to be supported.
		static bool g_copy_timestamps(service_ptr_t<file> from, service_ptr_t<file> to, abort_callback& abort);
		static bool g_copy_creation_time(service_ptr_t<file> from, service_ptr_t<file> to, abort_callback& abort);

		//! Helper; improved performance over g_transfer on streams (avoids disk fragmentation when transferring large blocks).
		static t_filesize g_transfer(service_ptr_t<file> p_src, service_ptr_t<file> p_dst, t_filesize p_bytes, abort_callback& p_abort);
		//! Helper; improved performance over g_transfer_file on streams (avoids disk fragmentation when transferring large blocks).
		static void g_transfer_object(service_ptr_t<file> p_src, service_ptr_t<file> p_dst, t_filesize p_bytes, abort_callback& p_abort);


		//! file_v2 wrapper;
		size_t lowLevelIO_(const GUID& guid, size_t arg1, void* arg2, size_t arg2size, abort_callback& abort);

		//! Helper
		bool flushFileBuffers(abort_callback&);
		bool flushFileBuffers_(abort_callback& a) { return flushFileBuffers(a); }
		//! Helper
		bool getFileTimes(filetimes_t& out, abort_callback&);
		//! Helper
		bool setFileTimes(filetimes_t const& in, abort_callback&);

		t_filesize skip(t_filesize p_bytes, abort_callback& p_abort);
		t_filesize skip_seek(t_filesize p_bytes, abort_callback& p_abort);

		//! file_v2 wrapper.
		service_ptr get_metadata_(abort_callback& a);

		//! file_v2 wrapper.
		t_filestats2 get_stats2_(uint32_t f, abort_callback& a);

		//! file_v2 wrapper.
		t_filetimestamp get_time_created(abort_callback& a);

		FB2K_MAKE_SERVICE_INTERFACE(file, service_base);
	};

	typedef service_ptr_t<file> file_ptr;

	//! Extension for shoutcast dynamic metadata handling.
	class file_dynamicinfo : public file {
		FB2K_MAKE_SERVICE_INTERFACE(file_dynamicinfo, file);
	public:
		//! Retrieves "static" info that doesn't change in the middle of stream, such as station names etc. Returns true on success; false when static info is not available.
		virtual bool get_static_info(class file_info& p_out) = 0;
		//! Returns whether dynamic info is available on this stream or not.
		virtual bool is_dynamic_info_enabled() = 0;
		//! Retrieves dynamic stream info (e.g. online stream track titles). Returns true on success, false when info has not changed since last call.
		virtual bool get_dynamic_info(class file_info& p_out) = 0;
	};

	//! \since 1.4.1
	//! Extended version of file_dynamicinfo
	class file_dynamicinfo_v2 : public file_dynamicinfo {
		FB2K_MAKE_SERVICE_INTERFACE(file_dynamicinfo_v2, file_dynamicinfo);
	public:
		virtual bool get_dynamic_info_v2(class file_info& out, t_filesize& outOffset) = 0;
	protected:
		// Obsolete
		bool get_dynamic_info(class file_info& p_out);
	};

	//! Extension for cached file access - allows callers to know that they're dealing with a cache layer, to prevent cache duplication.
	class file_cached : public file {
		FB2K_MAKE_SERVICE_INTERFACE(file_cached, file);
	public:
		virtual size_t get_cache_block_size() = 0;
		virtual void suggest_grow_cache(size_t suggestSize) = 0;

		static file::ptr g_create(service_ptr_t<file> p_base, abort_callback& p_abort, t_size blockSize);
		static void g_create(service_ptr_t<file>& p_out, service_ptr_t<file> p_base, abort_callback& p_abort, t_size blockSize);

		static void g_decodeInitCache(file::ptr& theFile, abort_callback& abort, size_t blockSize);
	};

	//! \since 1.5
	//! Additional service implemented by standard file object providing access to low level OS specific APIs. \n
	//! Obsolete, lowLevelIO() is now a part of file_v2 API.
	class file_lowLevelIO : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE(file_lowLevelIO, service_base);
	public:
		//! @returns 0 if the command was not recognized, a command-defined non zero value otherwise.
		virtual size_t lowLevelIO(const GUID& guid, size_t arg1, void* arg2, size_t arg2size, abort_callback& abort) = 0;

		//! Win32 FlushFileBuffers() wrapper. \n
		//! Throws exception_io_denied on a file opened for reading. \n
		//! No arguments are defined. \n
		//! Returns 1 if handled, 0 if unsupported.
		static const GUID guid_flushFileBuffers;
		//! Retrieves file creation / last access / last write times. \n
		//! Parameters: arg2 points to a filetimes_t struct to receive the data; arg2size must be set to sizeof(filetimes_t). \n
		//! If the filesystem does not support a specific portion of the information, relevant struct member will be set to filetimestamp_invalid. \n
		//! Returns 1 if handled, 0 if unsupported.
		static const GUID guid_getFileTimes;
		//! Sets file creation / last access / last write times. \n
		//! Parameters: arg2 points to a filetimes_t struct holding the new data; arg2size must be set to sizeof(filetimes_t). \n
		//! Individual members of the filetimes_t struct can be set to filetimestamp_invalid, if not all of the values are to be altered on the file. \n
		//! Returns 1 if handled, 0 if unsupported.
		static const GUID guid_setFileTimes;

		typedef ::foobar2000_io::filetimes_t filetimes_t;
	};

	//! Implementation helper - contains dummy implementations of methods that modify the file
	template<typename t_base> class file_readonly_t : public t_base {
	public:
		void resize(t_filesize p_size, abort_callback& p_abort) { throw exception_io_denied(); }
		void write(const void* p_buffer, t_size p_bytes, abort_callback& p_abort) { throw exception_io_denied(); }
	};
	typedef file_readonly_t<file> file_readonly;

	class file_streamstub : public file_readonly {
	public:
		t_size read(void*, t_size, abort_callback&) { return 0; }
		t_filesize get_size(abort_callback&) { return filesize_invalid; }
		t_filesize get_position(abort_callback&) { return 0; }
		bool get_content_type(pfc::string_base&) { return false; }
		bool is_remote() { return true; }
		void reopen(abort_callback&) {}
		void seek(t_filesize, abort_callback&) { throw exception_io_object_not_seekable(); }
		bool can_seek() { return false; }
	};

	//! \since 2.0
	class file_v2 : public file {
		FB2K_MAKE_SERVICE_INTERFACE(file_v2, file);
	public:
		//! Returns an object with protocol specific metadata of the file. \n
		//! It is essential that this object is made available to the caller by any wrappers working on top if a file object. \n
		//! The returned object can be of any implementation-defined class; for http it's file_metdata_http. \n
		//! Null return is allowed if no metadata is available.
		virtual service_ptr get_metadata(abort_callback&) { return nullptr; }

		virtual t_filestats2 get_stats2(uint32_t s2flags, abort_callback&) = 0;

		virtual size_t lowLevelIO(const GUID& guid, size_t arg1, void* arg2, size_t arg2size, abort_callback& abort) { return 0; }

		// Old method wrapped to get_stats2()
		t_filetimestamp get_timestamp(abort_callback& p_abort) override;
	};

	//! \since 1.6.7
	class file_metadata_http : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE(file_metadata_http, service_base);
	public:
		virtual bool get_http_header(const char* name, pfc::string_base& out) = 0;
		virtual void get_connected_path(pfc::string_base& out) = 0;
	};

}