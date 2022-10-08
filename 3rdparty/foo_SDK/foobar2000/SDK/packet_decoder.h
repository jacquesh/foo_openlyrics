#pragma once
//! Provides interface to decode various audio data types to PCM. Use packet_decoder_factory_t template to register.

class NOVTABLE packet_decoder : public service_base {
protected:
	//! Prototype of function that must be implemented by packet_decoder implementation but is not accessible through packet_decoder interface itself.
	//! Determines whether specific packet_decoder implementation supports specified decoder setup data.
	static bool g_is_our_setup(const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size) {return false;}

	//! Prototype of function that must be implemented by packet_decoder implementation but is not accessible through packet_decoder interface itself.
	//! Initializes packet decoder instance with specified decoder setup data. This is called only once, before any other methods.
	//! @param p_decode If set to true, decode() and reset_after_seek() calls can be expected later. If set to false, those methods will not be called on this packet_decoder instance - for an example when caller is only retrieving information about the file rather than preparing to decode it.
	void open(const GUID & p_owner,bool p_decode,t_size p_param1,const void * p_param2,t_size p_param2size,abort_callback & p_abort) {throw exception_io_data();}
public:

	//! Prototype of function that must be implemented by packet_decoder implementation but is not accessible through packet_decoder interface itself.
	//! Returns true if this is not the preferred decoder for this format, another one should be used if found.
	static bool g_is_supported_partially(const GUID& p_owner, t_size p_param1, const void* p_param2, t_size p_param2size) { return false; }


	//! Forwards additional information about stream being decoded. \n
	//! Calling: this must be called immediately after packet_decoder object is created, before any other methods are called.\n
	//! Implementation: this is called after open() (which is called by implementation framework immediately after creation), and before any other methods are called.
	virtual t_size set_stream_property(const GUID & p_type,t_size p_param1,const void * p_param2,t_size p_param2size) = 0;

	
	//! Retrieves additional user-readable tech infos that decoder can provide.
	//! @param p_info Interface receiving information about the stream being decoded. Note that it already contains partial info about the file; existing info should not be erased, decoder-provided info should be merged with it.
	virtual void get_info(file_info & p_info) = 0;

	//! Returns many frames back to start decoding when seeking.
	virtual unsigned get_max_frame_dependency()=0;
	//! Returns much time back to start decoding when seeking (for containers where going back by specified number of frames is not trivial).
	virtual double get_max_frame_dependency_time()=0;

	//! Flushes decoder after seeking.
	virtual void reset_after_seek()=0;

	//! Decodes a block of audio data.\n
	//! It may return empty chunk even when successful (caused by encoder+decoder delay for an example), caller must check for it and handle it appropriately.
    //! Called with 0 bytes at the end of stream - if the decoder introduces a delay between input/output, any buffered data should be passed back then.
	virtual void decode(const void * p_buffer,t_size p_bytes,audio_chunk & p_chunk,abort_callback & p_abort)=0;

	//! Returns whether this packet decoder supports analyze_first_frame() function.
	virtual bool analyze_first_frame_supported() = 0;  
	//! Optional. Some codecs need to analyze first frame of the stream to return additional info about the stream, such as encoding setup. This can be only called immediately after instantiation (and set_stream_property() if present), before any actual decoding or get_info(). Caller can determine whether this method is supported or not by calling analyze_first_frame_supported(), to avoid reading first frame when decoder won't utiilize the extra info for an example. If particular decoder can't utilize first frame info in any way (and analyze_first_frame_supported() returns false), this function should do nothing and succeed.
	virtual void analyze_first_frame(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) = 0;
	
	//! Static helper, creates a packet_decoder instance and initializes it with specific decoder setup data.
	static void g_open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size,abort_callback & p_abort);

    static const GUID owner_MP4,owner_matroska,owner_MP3,owner_MP2,owner_MP1,owner_MP4_ALAC,owner_ADTS,owner_ADIF, owner_Ogg, owner_MP4_AMR, owner_MP4_AMR_WB, owner_MP4_AC3, owner_MP4_EAC3, owner_MP4_FLAC;

	struct matroska_setup
	{
		const char * codec_id;
		uint32_t sample_rate,sample_rate_output;
		uint32_t channels;
		size_t codec_private_size;
		const void * codec_private;
	};
	//owner_MP4: param1 - codec ID (MP4 audio type), param2 - MP4 codec initialization data
	//owner_MP3: raw MP3/MP2 file, parameters ignored
	//owner_matroska: param2 = matroska_setup struct, param2size size must be equal to sizeof(matroska_setup)


	//these are used to initialize PCM decoder
	static const GUID property_samplerate,property_bitspersample,property_channels,property_byteorder,property_signed,property_channelmask, property_bufferpadding, property_eventlogger, property_checkingintegrity, property_samples_per_frame;
	//property_samplerate : param1 == sample rate in hz
	//property_bitspersample : param1 == bits per sample
	//property_channels : param1 == channel count
	//property_byteorder : if (param1) little_endian; else big_endian;
	//property_signed : if (param1) signed; else unsigned;
	//propery_bufferpadding : param1 == padding of each passed buffer in bytes; retval: decoder's preferred padding
	//property_eventlogger : param2 = event logger ptr, NULL to disable, param2size 0 always
	//property_checkingintegrity : param1 = checking integrity bool flag
    //property_samples_per_frame : param1 = samples per frame
	


	//property_ogg_header : p_param1 = unused, p_param2 = ogg_packet structure, retval: 0 when more headers are wanted, 1 when done parsing headers
	//property_ogg_query_sample_rate : returns sample rate, no parameters
	//property_ogg_packet : p_param1 = unused, p_param2 = ogg_packet strucute
	//property_ogg_qury_preskip : returns preskip samples (Opus)
	static const GUID property_ogg_header, property_ogg_query_sample_rate, property_ogg_packet, property_ogg_query_preskip;

    //property_mp4_esds : p_param2 = MP4 ESDS chunk content as needed by some decoders
    static const GUID property_mp4_esds;

    // DEPRECATED
    static const GUID property_allow_delayed_output;
    
    // property_mp3_delayless : return non-zero if this codec drops MP3 delay by itself
    static const GUID property_mp3_delayless;

	// property_query_delay_samples : 
	// Return non-zero if this codec has a decoder delay that the caller should deal with.
	// Param1 signals sample rate used by input - should always match decoder's sample rate - return zero if it does not match.
	static const GUID property_query_delay_samples;
    
	// property_query_mp4_use_elst :
	// Return non-zero if MP4 elst should be used with this codec.
	static const GUID property_query_mp4_use_elst;

	size_t initPadding();
	void setEventLogger(event_logger::ptr logger);
	void setCheckingIntegrity(bool checkingIntegrity);
    void setAllowDelayed( bool bAllow = true );

	FB2K_MAKE_SERVICE_INTERFACE(packet_decoder,service_base);
};

class NOVTABLE packet_decoder_streamparse : public packet_decoder
{
public:
	virtual void decode_ex(const void * p_buffer,t_size p_bytes,t_size & p_bytes_processed,audio_chunk & p_chunk,abort_callback & p_abort) = 0;
	virtual void analyze_first_frame_ex(const void * p_buffer,t_size p_bytes,t_size & p_bytes_processed,abort_callback & p_abort) = 0;

	FB2K_MAKE_SERVICE_INTERFACE(packet_decoder_streamparse,packet_decoder);
};

class NOVTABLE packet_decoder_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(packet_decoder_entry);
public:
	virtual bool is_our_setup(const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size) = 0;
	virtual void open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size,abort_callback & p_abort) = 0;

	//! Returns true if this is not the preferred decoder for this format, another one should be used if found.
	bool is_supported_partially_(const GUID& p_owner, t_size p_param1, const void* p_param2, t_size p_param2size);
};

class NOVTABLE packet_decoder_entry_v2 : public packet_decoder_entry {
	FB2K_MAKE_SERVICE_INTERFACE(packet_decoder_entry_v2, packet_decoder_entry);
public:
	//! Returns true if this is not the preferred decoder for this format, another one should be used if found.
	virtual bool is_supported_partially(const GUID& p_owner, t_size p_param1, const void* p_param2, t_size p_param2size) = 0;
};


template<class T>
class packet_decoder_entry_impl_t : public packet_decoder_entry_v2
{
public:
	bool is_our_setup(const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size) override {
		return T::g_is_our_setup(p_owner,p_param1,p_param2,p_param2size);
	}
	void open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,t_size p_param1,const void * p_param2,t_size p_param2size,abort_callback & p_abort) override {
		PFC_ASSERT(is_our_setup(p_owner,p_param1,p_param2,p_param2size));
		service_ptr_t<T> instance = new service_impl_t<T>();
		instance->open(p_owner,p_decode,p_param1,p_param2,p_param2size,p_abort);
		p_out = instance.get_ptr();
	}
	bool is_supported_partially(const GUID& p_owner, t_size p_param1, const void* p_param2, t_size p_param2size) override {
		return T::g_is_supported_partially(p_owner, p_param1, p_param2, p_param2size);
	}
};

template<typename T>
class packet_decoder_factory_t : public service_factory_single_t<packet_decoder_entry_impl_t<T> > {};
