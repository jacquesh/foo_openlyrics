#pragma once
#include <functional>
#include "event_logger.h"
#include "audio_chunk.h"

PFC_DECLARE_EXCEPTION(exception_tagging_unsupported, exception_io_data, "Tagging of this file format is not supported")

enum {
	input_flag_no_seeking					= 1 << 0,
	input_flag_no_looping					= 1 << 1,
	input_flag_playback						= 1 << 2,
	input_flag_testing_integrity			= 1 << 3,
	input_flag_allow_inaccurate_seeking		= 1 << 4,
	input_flag_no_postproc					= 1 << 5,

	input_flag_simpledecode = input_flag_no_seeking|input_flag_no_looping,
};

//! Class providing interface for retrieval of information (metadata, duration, replaygain, other tech infos) from files. Also see: file_info. \n
//! Instantiating: see input_entry.\n
//! Implementing: see input_impl.

class NOVTABLE input_info_reader : public service_base
{
public:
	//! Retrieves count of subsongs in the file. 1 for non-multisubsong-enabled inputs.
	//! Note: multi-subsong handling is disabled for remote files (see: filesystem::is_remote) for performance reasons. Remote files are always assumed to be single-subsong, with null index.
	virtual t_uint32 get_subsong_count() = 0;
	
	//! Retrieves identifier of specified subsong; this identifier is meant to be used in playable_location as well as a parameter for input_info_reader::get_info().
	//! @param p_index Index of subsong to query. Must be >=0 and < get_subsong_count().
	virtual t_uint32 get_subsong(t_uint32 p_index) = 0;
	
	//! Retrieves information about specified subsong.
	//! @param p_subsong Identifier of the subsong to query. See: input_info_reader::get_subsong(), playable_location.
	//! @param p_info file_info object to fill. Must be empty on entry.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) = 0;

	//! Retrieves file stats. Equivalent to calling get_stats() on file object.
	virtual t_filestats get_file_stats(abort_callback & p_abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_info_reader,service_base);
};

//! Class providing interface for retrieval of PCM audio data from files.\n
//! Instantiating: see input_entry.\n
//! Implementing: see input_impl.

class NOVTABLE input_decoder : public input_info_reader
{
public:
	//! Prepares to decode specified subsong; resets playback position to the beginning of specified subsong. This must be called first, before any other input_decoder methods (other than those inherited from input_info_reader). \n
	//! It is legal to set initialize() more than once, with same or different subsong, to play either the same subsong again or another subsong from same file without full reopen.\n
	//! Warning: this interface inherits from input_info_reader, it is legal to call any input_info_reader methods even during decoding! Call order is not defined, other than initialize() requirement before calling other input_decoder methods.\n
	//! @param p_subsong Subsong to decode. Should always be 0 for non-multi-subsong-enabled inputs.
	//!	@param p_flags Specifies additional hints for decoding process. It can be null, or a combination of one or more following constants: \n
	//!		input_flag_no_seeking - Indicates that seek() will never be called. Can be used to avoid building potentially expensive seektables when only sequential reading is needed.\n
	//!		input_flag_no_looping - Certain input implementations can be configured to utilize looping info from file formats they process and keep playing single file forever, or keep repeating it specified number of times. This flag indicates that such features should be disabled, for e.g. ReplayGain scan or conversion.\n
	//!		input_flag_playback	- Indicates that decoding process will be used for realtime playback rather than conversion. This can be used to reconfigure features that are relevant only for conversion and take a lot of resources, such as very slow secure CDDA reading. \n
	//!		input_flag_testing_integrity - Indicates that we're testing integrity of the file. Any recoverable problems where decoding would normally continue should cause decoder to fail with exception_io_data.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void initialize(t_uint32 p_subsong,unsigned p_flags,abort_callback & p_abort) = 0;

	//! Reads/decodes one chunk of audio data. Use false return value to signal end of file (no more data to return). Before calling run(), decoding must be initialized by initialize() call.
	//! @param p_chunk audio_chunk object receiving decoded data. Contents are valid only the method returns true.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	//! @returns true on success (new data decoded), false on EOF.
	virtual bool run(audio_chunk & p_chunk,abort_callback & p_abort) = 0;

	//! Seeks to specified time offset. Before seeking or other decoding calls, decoding must be initialized with initialize() call.
	//! @param p_seconds Time to seek to, in seconds. If p_seconds exceeds length of the object being decoded, succeed, and then return false from next run() call.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void seek(double p_seconds,abort_callback & p_abort) = 0;
	
	//! Queries whether resource being read/decoded is seekable. If p_value is set to false, all seek() calls will fail. Before calling can_seek() or other decoding calls, decoding must be initialized with initialize() call.
	virtual bool can_seek() = 0;

	//! This function is used to signal dynamic VBR bitrate, etc. Called after each run() (or not called at all if caller doesn't care about dynamic info).
	//! @param p_out Initially contains currently displayed info (either last get_dynamic_info result or current cached info), use this object to return changed info.
	//! @param p_timestamp_delta Indicates when returned info should be displayed (in seconds, relative to first sample of last decoded chunk), initially set to 0.
	//! @returns false to keep old info, or true to indicate that changes have been made to p_info and those should be displayed.
	virtual bool get_dynamic_info(file_info & p_out, double & p_timestamp_delta) = 0;

	//! This function is used to signal dynamic live stream song titles etc. Called after each run() (or not called at all if caller doesn't care about dynamic info). The difference between this and get_dynamic_info() is frequency and relevance of dynamic info changes - get_dynamic_info_track() returns new info only on track change in the stream, returning new titles etc.
	//! @param p_out Initially contains currently displayed info (either last get_dynamic_info_track result or current cached info), use this object to return changed info.
	//! @param p_timestamp_delta Indicates when returned info should be displayed (in seconds, relative to first sample of last decoded chunk), initially set to 0.
	//! @returns false to keep old info, or true to indicate that changes have been made to p_info and those should be displayed.
	virtual bool get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) = 0;

	//! Called from playback thread before sleeping.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void on_idle(abort_callback & p_abort) = 0;


	FB2K_MAKE_SERVICE_INTERFACE(input_decoder,input_info_reader);
};


class NOVTABLE input_decoder_v2 : public input_decoder {
	FB2K_MAKE_SERVICE_INTERFACE(input_decoder_v2, input_decoder)
public:

	//! OPTIONAL, throws pfc::exception_not_implemented() when not supported by this implementation.
	//! Special version of run(). Returns an audio_chunk object as well as a raw data block containing original PCM stream. This is mainly used for MD5 checks on lossless formats. \n
	//! If you set a "MD5" tech info entry in get_info(), you should make sure that run_raw() returns data stream that can be used to verify it. \n
	//! Returned raw data should be possible to cut into individual samples; size in bytes should be divisible by audio_chunk's sample count for splitting in case partial output is needed (with cuesheets etc).
	virtual bool run_raw(audio_chunk & out, mem_block_container & outRaw, abort_callback & abort) = 0;

	//! OBSOLETE since 1.5 \n
	//! Specify logger when opening to reliably get info generated during input open operation.
	virtual void set_logger(event_logger::ptr ptr) = 0;
};

class NOVTABLE input_decoder_v3 : public input_decoder_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(input_decoder_v3, input_decoder_v2);
public:
	//! OBSOLETE, functionality implemented by core.
	virtual void set_pause(bool paused) = 0;
	//! OPTIONAL, should return false in most cases; return true to force playback buffer flush on unpause. Valid only after initialize() with input_flag_playback.
	virtual bool flush_on_pause() = 0;
};

class NOVTABLE input_decoder_v4 : public input_decoder_v3 {
    FB2K_MAKE_SERVICE_INTERFACE( input_decoder_v4, input_decoder_v3 );
public:
    //! OPTIONAL, return 0 if not implemented. \n
    //! Provides means for communication of context specific data with the decoder. The decoder should do nothing and return 0 if it does not recognize the passed arguments.
    virtual size_t extended_param( const GUID & type, size_t arg1, void * arg2, size_t arg2size) = 0;
};

//! Parameter GUIDs for input_decoder_v3::extended_param().
class input_params {
public:
    //! Signals whether unnecessary seeking should be avoided with this decoder for performance reasons. \n
    //! Arguments disregarded, return value 1 or 0.
    static const GUID seeking_expensive;

	//! Tells the decoder to output at this sample rate if the decoder's sample rate is adjustable. \n
	//! Sample rate signaled in arg1.
	static const GUID set_preferred_sample_rate;

	//! Retrieves logical decode position from the decoder. Implemented only in some rare cases where logical position does not match duration of returned data so far.
	//! arg2 points to double position in seconds.
	//! Return 1 if position was written to arg2, 0 if n/a.
	static const GUID query_position;
};

//! Class providing interface for writing metadata and replaygain info to files. Also see: file_info. \n
//! Instantiating: see input_entry.\n
//! Implementing: see input_impl.
class NOVTABLE input_info_writer : public input_info_reader
{
public:
	//! Tells the service to update file tags with new info for specified subsong.
	//! @param p_subsong Subsong to update. Should be always 0 for non-multisubsong-enabled inputs.
	//! @param p_info New info to write. Sometimes not all contents of p_info can be written. Caller will typically read info back after successful write, so e.g. tech infos that change with retag are properly maintained.
	//! @param p_abort abort_callback object signaling user aborting the operation. WARNING: abort_callback object is provided for consistency; if writing tags actually gets aborted, user will be likely left with corrupted file. Anything calling this should make sure that aborting is either impossible, or gives appropriate warning to the user first.
	virtual void set_info(t_uint32 p_subsong,const file_info & p_info,abort_callback & p_abort) = 0;
	
	//! Commits pending updates. In case of multisubsong inputs, set_info should queue the update and perform actual file access in commit(). Otherwise, actual writing can be done in set_info() and then commit() can just do nothing and always succeed.
	//! @param p_abort abort_callback object signaling user aborting the operation. WARNING: abort_callback object is provided for consistency; if writing tags actually gets aborted, user will be likely left with corrupted file. Anything calling this should make sure that aborting is either impossible, or gives appropriate warning to the user first.
	virtual void commit(abort_callback & p_abort) = 0;

	//! Helper for writers not implementing input_info_writer_v2::remove_tags().
	void remove_tags_fallback(abort_callback & abort);

	FB2K_MAKE_SERVICE_INTERFACE(input_info_writer,input_info_reader);
};

//! Extended input_info_writer. Not every input implements it. \n
//! Provides an explicit remove_tags(), which erases all supported tags from the file.
class NOVTABLE input_info_writer_v2 : public input_info_writer {
public:
	//! Removes all tags from this file. Cancels any set_info() requests on this object. Does not require a commit() afterwards.
	//! If no input_info_writer_v2 is provided, similar affect can be achieved by set_info()+commit() with blank file_info, but may not be as thorough; will typically result in blank tags rather than total removal fo tags.
	virtual void remove_tags(abort_callback & abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(input_info_writer_v2, input_info_writer);
};

class NOVTABLE input_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_entry);
public:
	//! Determines whether specified content type can be handled by this input.
	//! @param p_type Content type string to test.
	virtual bool is_our_content_type(const char * p_type)=0;

	//! Determines whether specified file type can be handled by this input. This must not use any kind of file access; the result should be only based on file path / extension.
	//! @param p_full_path Full URL of file being tested.
	//! @param p_extension Extension of file being tested, provided by caller for performance reasons.
	virtual bool is_our_path(const char * p_full_path,const char * p_extension)=0;
	
	//! Opens specified resource for decoding.
	//! @param p_instance Receives new input_decoder instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Opens specified file for reading info.
	//! @param p_instance Receives new input_info_reader instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Opens specified file for writing info.
	//! @param p_instance Receives new input_info_writer instance if successful.
	//! @param p_filehint Optional; passes file object to use for the operation; if set to null, the service will handle opening file by itself. Note that not all inputs operate on physical files that can be reached through filesystem API, some of them require this parameter to be set to null (tone and silence generators for an example).
	//! @param p_path URL of resource being opened.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort) = 0;

	//! Reserved for future use. Do nothing and return until specifications are finalized.
	virtual void get_extended_data(service_ptr_t<file> p_filehint,const playable_location & p_location,const GUID & p_guid,mem_block_container & p_out,abort_callback & p_abort) = 0;

	enum {
		//! Indicates that this service implements some kind of redirector that opens another input for decoding, used to avoid circular call possibility.
		flag_redirect = 1,
		//! Indicates that multi-CPU optimizations should not be used.
		flag_parallel_reads_slow = 2,
	};
	//! See flag_* enums.
	virtual unsigned get_flags() = 0;

	inline bool is_redirect() {return (get_flags() & flag_redirect) != 0;}
	inline bool are_parallel_reads_slow() {return (get_flags() & flag_parallel_reads_slow) != 0;}

	static bool g_find_service_by_path(service_ptr_t<input_entry> & p_out,const char * p_path);
    static bool g_find_service_by_path(service_ptr_t<input_entry> & p_out,const char * p_path, const char * p_ext);
	static bool g_find_service_by_content_type(service_ptr_t<input_entry> & p_out,const char * p_content_type);
	static void g_open_for_decoding(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_open_for_info_read(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_open_for_info_write(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,bool p_from_redirect = false);
	static void g_open_for_info_write_timeout(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> p_filehint,const char * p_path,abort_callback & p_abort,double p_timeout,bool p_from_redirect = false);
	static bool g_is_supported_path(const char * p_path);
	typedef std::function<bool ( input_entry::ptr ) > input_filter_t;
	static bool g_find_inputs_by_content_type(pfc::list_base_t<service_ptr_t<input_entry> > & p_out, const char * p_content_type, bool p_from_redirect);
	static bool g_find_inputs_by_path(pfc::list_base_t<service_ptr_t<input_entry> > & p_out, const char * p_path, bool p_from_redirect );
	static bool g_find_inputs_by_content_type_ex(pfc::list_base_t<service_ptr_t<input_entry> > & p_out, const char * p_content_type, input_filter_t filter );
	static bool g_find_inputs_by_path_ex(pfc::list_base_t<service_ptr_t<input_entry> > & p_out, const char * p_path, input_filter_t filter );
	static service_ptr g_open(const GUID & whatFor, file::ptr hint, const char * path, event_logger::ptr logger, abort_callback & aborter, bool fromRedirect = false);

	void open(service_ptr_t<input_decoder> & p_instance,service_ptr_t<file> const & p_filehint,const char * p_path,abort_callback & p_abort) {open_for_decoding(p_instance,p_filehint,p_path,p_abort);}
	void open(service_ptr_t<input_info_reader> & p_instance,service_ptr_t<file> const & p_filehint,const char * p_path,abort_callback & p_abort) {open_for_info_read(p_instance,p_filehint,p_path,p_abort);}
	void open(service_ptr_t<input_info_writer> & p_instance,service_ptr_t<file> const & p_filehint,const char * p_path,abort_callback & p_abort) {open_for_info_write(p_instance,p_filehint,p_path,p_abort);}
	service_ptr open(const GUID & whatFor, file::ptr hint, const char * path, event_logger::ptr logger, abort_callback & aborter);

	typedef pfc::list_base_const_t< input_entry::ptr > input_entry_list_t;

	static service_ptr g_open_from_list(input_entry_list_t const & list, const GUID & whatFor, file::ptr hint, const char * path, event_logger::ptr logger, abort_callback & aborter, GUID * outGUID = nullptr);
	static bool g_are_parallel_reads_slow( const char * path );
	
	static uint32_t g_flags_for_path( const char * pathFor, uint32_t mask = UINT32_MAX );
	static uint32_t g_flags_for_content_type( const char * ct, uint32_t mask = UINT32_MAX );
};

//! \since 1.4
//! Extended input_entry methods provided by decoders. \n
//! Can be implemented by 1.3-compatible components but will not be called in fb2k versions prior to 1.4.
class input_entry_v2 : public input_entry {
	FB2K_MAKE_SERVICE_INTERFACE(input_entry_v2, input_entry);
public:
	//! @returns GUID used to identify us among other decoders in the decoder priority table.
	virtual GUID get_guid() = 0;
	//! @returns Name to present to the user in the decoder priority table.
	virtual const char * get_name() = 0;
	//! @returns GUID of this decoder's preferences page (optional), null guid if there's no page to present
	virtual GUID get_preferences_guid() = 0;
	//! @returns true if the decoder should be put at the end of the list when it's first sighted, false otherwise (will be put at the beginning of the list).
	virtual bool is_low_merit() = 0;
};

//! \since 1.5
class input_entry_v3 : public input_entry_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(input_entry_v3, input_entry_v2);
public:
	//! New unified open() function for all supported interfaces. Supports any future interfaces via alternate GUIDs, as well as allows the event logger to be set prior to the open() call.
	//! @param whatFor The class GUID of the service we want. \n
	//!  Currently allowed are: input_decoder::class_guid, input_info_reader::class_guid, input_info_writer::class_guid. \n
	//!  This method must throw pfc::exception_not_implemented for any GUIDs it does not recognize.
	virtual service_ptr open_v3( const GUID & whatFor, file::ptr hint, const char * path, event_logger::ptr logger, abort_callback & aborter ) = 0;

	
	void open_for_decoding(service_ptr_t<input_decoder> & p_instance, service_ptr_t<file> p_filehint, const char * p_path, abort_callback & p_abort) ;
	void open_for_info_read(service_ptr_t<input_info_reader> & p_instance, service_ptr_t<file> p_filehint, const char * p_path, abort_callback & p_abort);
	void open_for_info_write(service_ptr_t<input_info_writer> & p_instance, service_ptr_t<file> p_filehint, const char * p_path, abort_callback & p_abort);
};

#ifdef FOOBAR2000_DESKTOP
//! \since 1.4
//! Core API to perform input open operations respecting user settings for decoder priority. \n
//! Unavailable prior to 1.4.
class input_manager : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(input_manager);
public:
	virtual service_ptr open(const GUID & whatFor, file::ptr hint, const char * path, bool fromRedirect, abort_callback & aborter, GUID * outUsedEntry = nullptr) = 0;

	//! input_manager_v2 wrapper.
	service_ptr open_v2(const GUID & whatFor, file::ptr hint, const char * path, bool fromRedirect, event_logger::ptr logger, abort_callback & aborter, GUID * outUsedEntry = nullptr);
};

//! \since 1.5
//! Extension of input_manager. \n
//! Extended open_v2() supports album_art_extractor and album_art_editor. It reliably throws pfc::exception_not_implemented() for unsupported GUIDs (old version would bugcheck). \n
//! It also allows event_logger to be specified in advance so open() implementation can already use it.
class input_manager_v2 : public input_manager {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(input_manager_v2, input_manager)
public:
	virtual service_ptr open_v2(const GUID & whatFor, file::ptr hint, const char * path, bool fromRedirect, event_logger::ptr logger, abort_callback & aborter, GUID * outUsedEntry = nullptr) = 0;
};

//! \since 1.5
class input_manager_v3 : public input_manager_v2 {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(input_manager_v3, input_manager_v2);
public:
	//! Retrieves list of enabled inputs, in user-specified order. \n
	//! This is rarely needed. If you need this function, consider redesigning your code to call input_manager open methods instead.
	virtual void get_enabled_inputs( pfc::list_base_t<input_entry::ptr> & out ) = 0;
	//! Returns input_entry get_flags() values for this path, as returned by enabled inputs.
	virtual uint32_t flags_for_path( const char * pathFor, uint32_t mask = UINT32_MAX ) = 0;
	//! Returns input_entry get_flags() values for this content type, as returned by enabled inputs.
	virtual uint32_t flags_for_content_type( const char * ct, uint32_t mask = UINT32_MAX ) = 0;


	enum {
		flagFromRedirect = 1 << 0,
		flagSuppressFilters = 1 << 1,
	};

	virtual service_ptr open_v3(const GUID & whatFor, file::ptr hint, const char * path, uint32_t flags, event_logger::ptr logger, abort_callback & aborter, GUID * outUsedEntry = nullptr) = 0;
};

//! \since 1.4
//! Core API for determining which audio stream to decode, in a multi-stream enabled input. \n
//! Unavailable prior to 1.4 - decode the default stream if input_stream_selector isn't present. \n
//! In foobar2000 v1.4 and up, this API allows decoders to determine which stream the user opted to decode for a specific file. \n
//! Use input_stream_selector::tryGet() to safely instantiate.
class input_stream_selector : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(input_stream_selector);
public:
	//! Returns index of stream that should be presented for this file. \n
	//! If not set by user, 0xFFFFFFFF will be returned and the default stream should be presented. \n
	//! @param guid GUID of the input asking for the stream.
	virtual uint32_t select_stream( const GUID & guid, const char * path ) = 0;
};

//! \since 1.4
//! Interface provided by multi-stream enabled inputs to let the stream picker dialog show available streams. \n
//! Can be implemented by 1.3-compatible components but will not be called in fb2k versions prior to 1.4.
class input_stream_info_reader : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(input_stream_info_reader, service_base);
public:
	//! @returns Number of audio streams found.
	virtual uint32_t get_stream_count() = 0;
	//! Retrieves information about the specified stream; most importantly the codec name and bitrate.
	virtual void get_stream_info(uint32_t index, file_info & out, abort_callback & aborter) = 0;
	//! @returns Index of default stream to decode if there is no user preference.
	virtual uint32_t get_default_stream() = 0;
};

//! \since 1.4
//! Entrypoint interface for spawning input_stream_info_reader. \n
//! Can be implemented by 1.3-compatible components but will not be called in fb2k versions prior to 1.4.
class input_stream_info_reader_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_stream_info_reader_entry);
public:
	//! Open file for reading stream infos.
	virtual input_stream_info_reader::ptr open( const char * path, file::ptr fileHint, abort_callback & abort ) = 0;

	//! Return GUID of the matching input_entry.
	virtual GUID get_guid() = 0;

};

//! \since 1.4
//! Callback for input_stream_manipulator \n
//! Used for applying ReplayGain to encoded audio streams.
class input_stream_manipulator_callback : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(input_stream_manipulator_callback, service_base);
public:
	//! Called first before other methods. Throw an exception if the file cannot be processed. \n
	//! The arguments are the same as packet_decoder open() arguments.
	virtual void set_decode_info(const GUID & p_owner, t_size p_param1, const void * p_param2, t_size p_param2size ) = 0;

	virtual void first_frame( const void * data, size_t bytes ) = 0;
	//! Called with progress value, in 0..1 range.
	virtual void on_progress( float progress ) = 0;
	//! @returns true if the frame has been altered and should be written back, false otherwise.
	virtual bool process_frame( void * data, size_t size ) = 0;
};

//! \since 1.4
//! Manipulate audio stream payload in files. \n
//! Used for applying ReplayGain to encoded audio streams.
class input_stream_manipulator : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(input_stream_manipulator);
public:
	enum op_t {
		//! Probe the file for codec information; calls set_decode_info() + first_frame() only.
		op_probe = 0,
		//! Read the entire stream - same as op_probe but then calls on_progress() + process_frame() with the entire file payload. \n
		//! No writing to the file is performed - process_frame() results are disregarded.
		op_read,
		//! Rewrite the stream. Similar to op_read, but frames altered by process_frame() are written back to the file.
		op_rewrite
	};
	//! @param path Path of file to process.
	//! @param fileHint optional file object, must be opened for read+write if bWalk is true.
	//! @param callback Callback object for this operation.
	//! @param opType Operation to perform, see op_t enum for details.
	//! @param abort abort_callback object for this operating. Aborting with bWalk set to true will leave the file partially altered, use with caution!
	virtual void process( const char * path, file::ptr fileHint, input_stream_manipulator_callback::ptr callback, op_t opType, abort_callback & abort ) = 0;
	//! Return GUID of the matching input_entry.
	virtual GUID get_guid() = 0;
};

//! \since 1.5
//! An input_info_filter lets you hook into all performed tag read & write operations. \n
//! Your tag manipulations will be transparent to all fb2k components, as if the tags were read/written by relevant inputs. \n
//! Your input_info_filter needs to be enabled in Preferences in order to become active. Newly added ones are inactive by default.
class input_info_filter : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT( input_info_filter );
public:
	//! Tags are being read from a file.
	virtual void filter_info_read( const playable_location & loc,file_info & info,abort_callback & abort ) = 0;
	//! Tags are being written to a file. \n
	//! Return true to continue, false to suppress writing of tags.
	virtual bool filter_info_write( const playable_location & loc, file_info & info, abort_callback & abort ) = 0;
	//! Tags are being removed from a file. 
	virtual void on_info_remove( const char * path, abort_callback & abort ) = 0;
	//! Return GUID of your filter.
	virtual GUID get_guid() = 0;
	//! Return preferences page or advconfig branch GUID of your filter.
	virtual GUID get_preferences_guid() = 0;
	//! Return user-friendly name of your filter to be shown in preferences.
	virtual const char * get_name() = 0;
	//! Optional backwards compatibility method. \n
	//! If you also provide input services for old foobar2000 versions which don't recognize input_info_filter, report their GUIDs here so they can be ignored. \n
	//! @param outGUIDs empty on entry, contains GUIDs of ignored inputs (if any) on return.
	virtual void get_suppressed_inputs( pfc::list_base_t<GUID> & outGUIDs ) {outGUIDs.remove_all();}
	//! write_fallback() supported or not? \n
	//! Used if your filter can store tags for untaggable files.
	virtual bool supports_fallback() = 0;
	//! Optional; called when user attempted to tag an untaggable/readonly file. \n
	//! Used if your filter can store tags for untaggable files.
	virtual bool write_fallback( const playable_location & loc, file_info const & info, abort_callback & abort ) = 0;
	//! Optional; called when user attempted to remove tags from an untaggable/readonly file.\ n
	//! Used if your filter can store tags for untaggable files.
	virtual void remove_tags_fallback( const char * path, abort_callback & abort ) = 0;
};

//! \since 1.5
class input_stream_info_filter : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE( input_stream_info_filter, service_base );
public:
	virtual void filter_dynamic_info( file_info & info ) = 0;
	virtual void filter_dynamic_info_track( file_info & info ) = 0;
};

class album_art_data;

//! \since 1.5
//! Extended input_info_filter.
class input_info_filter_v2 : public input_info_filter {
	FB2K_MAKE_SERVICE_INTERFACE( input_info_filter_v2, input_info_filter );
public:
	//! Creates an object which then can work with dynamic track titles etc of a decoded track. \n
	//! Returning null to filter the info is allowed.
	virtual input_stream_info_filter::ptr open_stream(playable_location const & loc, abort_callback & abort) = 0;


	typedef service_ptr_t<album_art_data> aaptr_t;

	//! Album art is being read from the file. \n
	//! info may be null if file had no such picture. \n
	//! Return passed info, altered info or null.
	virtual aaptr_t filter_album_art_read( const char * path, const GUID & type, aaptr_t info, abort_callback & aborter ) = 0;
	//! Album art is being written to the file. \n
	//! Return passed info, altered info or null to suppress writing.
	virtual aaptr_t filter_album_art_write( const char * path, const GUID & type, aaptr_t info, abort_callback & aborter ) = 0;
	//! Specific album art is being removed from the file. \n
	//! Return true to go on, false to suppress file update.
	virtual bool filter_album_art_remove( const char * path, const GUID & type, abort_callback & aborter ) = 0;
	//! All album art is being removed from the file. \n
	//! Return true to go on, false to suppress file update.
	virtual bool filter_album_art_remove_all( const char * path, abort_callback & aborter ) = 0;
	
	//! Valid with supports_fallback() = true \n
	//! Album art is being written to an untaggable file.
	virtual void write_album_art_fallback( const char * path, const GUID & type, aaptr_t info, abort_callback & aborter ) = 0;
	//! Valid with supports_fallback() = true \n
	//! Specific album art is being removed from an untaggable file.
	virtual void remove_album_art_fallback( const char * path, const GUID & type, abort_callback & aborter ) = 0;
	//! Valid with supports_fallback() = true \n
	//! All album art is being removed from an untaggable file.
	virtual void remove_all_album_art_fallback( const char * path, abort_callback & aborter ) = 0;
};

class dsp_preset;

//! \since 1.5
//! An input_playback_shim adds additional functionality to a DSP, allowing full control of the decoder. \n
//! Currently, input_playback_shim can only exist alongside a DSP, must have the same GUID as a DSP. \n
//! It will only be used in supported scenarios when the user has put your DSP in the chain. \n
//! Your DSP will be deactivated in such case when your input_playback_shim is active. \n
//! input_playback_shim is specifically intended to be instantiated for playback. Do not call this service from your component. \n/
//! Implement this service ONLY IF NECESSARY. Very few tasks really need it, primarily DSPs that manipulate logical playback time & seeking.
class input_playback_shim : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT( input_playback_shim );
public:
	//! Same GUID as your DSP.
	virtual GUID get_guid() = 0;
	//! Preferences page / advconfig branch GUID of your shim, pfc::guid_null if none. \n
	//! This is currently unused / reserved for future use.
	virtual GUID get_preferences_guid() = 0;
	//! Same as your DSP. \n
	//! This is currently unused / reserved for future use.
	virtual const char * get_name() = 0;
	//! Instantiates your shim on top of existing input_decoder. \n
	//! If you don't want to do anything with this specific decoder, just return the passed decoder.
	virtual input_decoder::ptr shim( input_decoder::ptr dec, const char * path, dsp_preset const & preset, abort_callback & aborter ) = 0;
	//! Optional backwards compatibility method. \n
	//! If you also provide input services for old versions of foobar2000 which don't recognize input_playback_shim, report their GUIDs here so they can be ignored. \n
	//! @param outGUIDs empty on entry, contains GUIDs of ignored inputs (if any) on return.
	virtual void get_suppressed_inputs( pfc::list_base_t<GUID> & outGUIDs ) {outGUIDs.remove_all();}
};

#endif // #ifdef FOOBAR2000_DESKTOP


typedef input_info_writer_v2 input_info_writer_vhighest;
typedef input_decoder_v4 input_decoder_vhighest;
typedef input_info_reader input_info_reader_vhighest;
