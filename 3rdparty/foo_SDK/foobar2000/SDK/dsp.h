#pragma once
#include "audio_chunk.h"
#include "filesystem.h"

#ifdef FOOBAR2000_HAVE_DSP

class dsp_preset; // forward declaration

//! Interface to a DSP chunk list. A DSP chunk list object is passed to the DSP chain each time, since DSPs are allowed to remove processed chunks or insert new ones.
class NOVTABLE dsp_chunk_list {
public:
	virtual t_size get_count() const = 0;
	virtual audio_chunk * get_item(t_size n) const = 0;
	virtual void remove_by_idx(t_size idx) = 0;
	virtual void remove_mask(const bit_array & mask) = 0;
	virtual audio_chunk * insert_item(t_size idx,t_size hint_size=0) = 0;

	audio_chunk * add_item(t_size hint_size=0);

	void remove_all();

	double get_duration();

	void add_chunk(const audio_chunk * chunk);

	void remove_bad_chunks();
protected:
	dsp_chunk_list() {}
	~dsp_chunk_list() {}
};

class dsp_chunk_list_impl : public dsp_chunk_list//implementation
{
	pfc::list_t<pfc::rcptr_t<audio_chunk> > m_data, m_recycled;
public:
	t_size get_count() const;
	audio_chunk * get_item(t_size n) const;
	void remove_by_idx(t_size idx);
	void remove_mask(const bit_array & mask);
	audio_chunk * insert_item(t_size idx,t_size hint_size=0);
};

//! Instance of a DSP.\n
//! Implementation: Derive from dsp_impl_base instead of deriving from dsp directly.\n
//! Instantiation: Use dsp_entry static helper methods to instantiate DSPs, or dsp_chain_config / dsp_manager to deal with entire DSP chains.
class NOVTABLE dsp : public service_base {
public:
	enum {
		//! Flush whatever you need to when tracks change.
		END_OF_TRACK = 1,
		//! Flush everything.
		FLUSH = 2	
	};

	//! @param p_chunk_list List of chunks to process. The implementation may alter the list in any way, inserting chunks of different sample rate / channel configuration etc.
	//! @param p_cur_file Optional, location of currently decoded file. May be null.
	//! @param p_flags Flags. Can be null, or a combination of END_OF_TRACK and FLUSH constants.
	virtual void run(dsp_chunk_list * p_chunk_list,const metadb_handle_ptr & p_cur_file,int p_flags)=0;

	//! Flushes the DSP (reinitializes / drops any buffered data). Called after seeking, etc.
	virtual void flush() = 0;

	//! Retrieves amount of data buffered by the DSP, for syncing visualisation.
	//! @returns Amount of buffered audio data, in seconds.
	virtual double get_latency() = 0;
	//! Returns true if DSP needs to know exact track change point (eg. for crossfading, removing silence).\n
	//! Signaling this will force-flush any DSPs placed before this DSP so when it gets END_OF_TRACK, relevant chunks contain last samples of the track.\n
	//! Signaling this will often break regular gapless playback so don't use it unless you have reasons to.
	virtual bool need_track_change_mark() = 0;

	void run_abortable(dsp_chunk_list * p_chunk_list,const metadb_handle_ptr & p_cur_file,int p_flags,abort_callback & p_abort);

	//! Attempts to apply preset without recreating the DSP, if supported. 
	//! @returns True on success, false if not supported (DSP needs re-creating).
	bool apply_preset_(const dsp_preset&);

	FB2K_MAKE_SERVICE_INTERFACE(dsp,service_base);
};

//! Backwards-compatible extension to dsp interface, allows abortable operation. Introduced in 0.9.2.
class NOVTABLE dsp_v2 : public dsp {
public:
	//! Abortable version of dsp::run(). See dsp::run() for descriptions of parameters.
	virtual void run_v2(dsp_chunk_list * p_chunk_list,const metadb_handle_ptr & p_cur_file,int p_flags,abort_callback & p_abort) = 0;
private:
	void run(dsp_chunk_list * p_chunk_list,const metadb_handle_ptr & p_cur_file,int p_flags) {
		run_v2(p_chunk_list,p_cur_file,p_flags,fb2k::noAbort);
	}

	FB2K_MAKE_SERVICE_INTERFACE(dsp_v2,dsp);
};

//! Extended version allowing live changes in configuration without reinitialization.
class NOVTABLE dsp_v3 : public dsp_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(dsp_v3, dsp_v2);
public:
	//! Live change of DSP settings. Return true if accepted, false if not (DSP will be destroyed and recreated).
	virtual bool apply_preset(const dsp_preset&) = 0;
};

//! Helper class for implementing dsps. You should derive from dsp_impl_base instead of from dsp directly.\n
//! The dsp_impl_base_t template allows you to use a custom interface class as a base class for your implementation, in case you provide extended functionality.\n
//! Use dsp_factory_t<> template to register your dsp implementation.
//! The implementation - as required by dsp_factory_t<> template - must also provide following methods:\n
//! A constructor taking const dsp_preset&, initializing the DSP with specified preset data.\n
//! static void g_get_name(pfc::string_base &); - retrieving human-readable name of the DSP to display.\n
//! static bool g_get_default_preset(dsp_preset &); - retrieving default preset for this DSP. Return value is reserved for future use and should always be true.\n
//! static GUID g_get_guid(); - retrieving GUID of your DSP implementation, to be used to identify it when storing DSP chain configuration.\n
//! static bool g_have_config_popup(); - retrieving whether your DSP implementation supplies a popup dialog for configuring it.\n
//! static void g_show_config_popup(const dsp_preset & p_data,HWND p_parent, dsp_preset_edit_callback & p_callback); - displaying your DSP's settings dialog; called only when g_have_config_popup() returns true; call p_callback.on_preset_changed() whenever user has made adjustments to the preset data.\n
template<class t_baseclass>
class dsp_impl_base_t : public t_baseclass {
private:
	typedef dsp_impl_base_t<t_baseclass> t_self;
	dsp_chunk_list * m_list = nullptr;
	t_size m_chunk_ptr = 0;
	metadb_handle* m_cur_file = nullptr;
	void run_v2(dsp_chunk_list * p_list,const metadb_handle_ptr & p_cur_file,int p_flags,abort_callback & p_abort) override;
protected:
	//! Call only from on_chunk / on_endoftrack (on_endoftrack will give info on track being finished).\n
	//! May return false when there's no known track and the metadb_handle ptr will be empty/null.
	bool get_cur_file(metadb_handle_ptr & p_out) const {p_out = m_cur_file; return p_out.is_valid();}
	metadb_handle_ptr get_cur_file() const { return m_cur_file; }
	
	dsp_impl_base_t() {}
	
	//! Inserts a new chunk of audio data. \n
	//! You can call this only from on_chunk(), on_endofplayback() and on_endoftrack(). You're NOT allowed to call this from flush() which should just drop any queued data.
	//! @param hint_size Optional, amount of buffer space that you require (in audio_samples). This is just a hint for memory allocation logic and will not cause the framework to allocate the chunk for you.
	//! @returns A pointer to the newly allocated chunk. Pass the audio data you want to insert to this chunk object. The chunk is owned by the framework, you can't delete it etc.
	audio_chunk * insert_chunk(t_size p_hint_size = 0) {
		PFC_ASSERT(m_list != NULL);
		return m_list->insert_item(m_chunk_ptr++,p_hint_size);
	}
	audio_chunk * insert_chunk( const audio_chunk & sourceCopy ) {
		audio_chunk * c = insert_chunk( sourceCopy.get_used_size() );
		c->copy( sourceCopy );
		return c;
	}


	//! To be overridden by a DSP implementation.\n
	//! Called on track change. You can use insert_chunk() to dump any data you have to flush. \n
	//! Note that you must implement need_track_change_mark() to return true if you need this method called.
	virtual void on_endoftrack(abort_callback & p_abort) = 0;
	//! To be overridden by a DSP implementation.\n
	//! Called at the end of played stream, typically at the end of last played track, to allow the DSP to return all data it has buffered-ahead.\n
	//! Use insert_chunk() to return any data you have buffered.\n
	//! Note that this call does not imply that the DSP will be destroyed next. \n
	//! This is also called on track changes if some DSP placed after your DSP requests track change marks.
	virtual void on_endofplayback(abort_callback & p_abort) = 0;
	//! To be overridden by a DSP implementation.\n
	//! Processes a chunk of audio data.\n
	//! You can call insert_chunk() from inside on_chunk() to insert any audio data before currently processed chunk.\n
	//! @param p_chunk Current chunk being processed. You can alter it in any way you like.
	//! @returns True to keep p_chunk (with alterations made inside on_chunk()) in the stream, false to remove it.
	virtual bool on_chunk(audio_chunk * p_chunk,abort_callback & p_abort) = 0;

public:
	//! To be overridden by a DSP implementation.\n
	//! Flushes the DSP (drops any buffered data). The implementation should reset the DSP to the same state it was in before receiving any audio data. \n
	//! Called after seeking, etc.
	virtual void flush() override = 0;
	//! To be overridden by a DSP implementation.\n
	//! Retrieves amount of data buffered by the DSP, for syncing visualisation.
	//! @returns Amount of buffered audio data, in seconds.
	virtual double get_latency() override = 0;
	//! To be overridden by a DSP implementation.\n
	//! Returns true if DSP needs to know exact track change point (eg. for crossfading, removing silence).\n
	//! Signaling this will force-flush any DSPs placed before this DSP so when it gets on_endoftrack(), relevant chunks contain last samples of the track.\n
	//! Signaling this may interfere with gapless playback in certain scenarios (forces flush of DSPs placed before you) so don't use it unless you have reasons to.
	virtual bool need_track_change_mark() override = 0;
private:
	dsp_impl_base_t(const t_self&) = delete;
	const t_self & operator=(const t_self &) = delete;
};

template<class t_baseclass>
void dsp_impl_base_t<t_baseclass>::run_v2(dsp_chunk_list * p_list,const metadb_handle_ptr & p_cur_file,int p_flags,abort_callback & p_abort) {
	pfc::vartoggle_t<dsp_chunk_list*> l_list_toggle(m_list,p_list);
	pfc::vartoggle_t<metadb_handle*> l_cur_file_toggle(m_cur_file,p_cur_file.get_ptr());
	
	for(m_chunk_ptr = 0;m_chunk_ptr<m_list->get_count();m_chunk_ptr++) {
		audio_chunk * c = m_list->get_item(m_chunk_ptr);
		if (c->is_empty() || !on_chunk(c,p_abort))
			m_list->remove_by_idx(m_chunk_ptr--);
	}

	if (p_flags & dsp::FLUSH) {
		on_endofplayback(p_abort);
	} else if (p_flags & dsp::END_OF_TRACK) {
		if (need_track_change_mark()) on_endoftrack(p_abort);
	}
}


typedef dsp_impl_base_t<dsp_v2> dsp_impl_base;

class NOVTABLE dsp_preset {
public:
	virtual GUID get_owner() const = 0;
	virtual void set_owner(const GUID & p_owner) = 0;
	virtual const void * get_data() const = 0;
	virtual t_size get_data_size() const = 0;
	virtual void set_data(const void * p_data,t_size p_data_size) = 0;
	virtual void set_data_from_stream(stream_reader * p_stream,t_size p_bytes,abort_callback & p_abort) = 0;

	const dsp_preset & operator=(const dsp_preset & p_source) {copy(p_source); return *this;}

	void copy(const dsp_preset & p_source) {set_owner(p_source.get_owner());set_data(p_source.get_data(),p_source.get_data_size());}

	void contents_to_stream(stream_writer * p_stream,abort_callback & p_abort) const;
	void contents_from_stream(stream_reader * p_stream,abort_callback & p_abort);
	static void g_contents_from_stream_skip(stream_reader * p_stream,abort_callback & p_abort);

	bool operator==(const dsp_preset & p_other) const {
		if (get_owner() != p_other.get_owner()) return false;
		if (get_data_size() != p_other.get_data_size()) return false;
		if (memcmp(get_data(),p_other.get_data(),get_data_size()) != 0) return false;
		return true;
	}
	bool operator!=(const dsp_preset & p_other) const {
		return !(*this == p_other);
	}

	pfc::string8 get_owner_name() const;
	pfc::string8 get_owner_name_debug() const;
	pfc::string8 debug(const char * knownName = nullptr) const;
protected:
	dsp_preset() {}
	~dsp_preset() {}
};

class dsp_preset_writer : public stream_writer {
public:
	void write(const void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		p_abort.check();
		m_data.append_fromptr((const t_uint8 *) p_buffer,p_bytes);
	}
	void flush(dsp_preset & p_preset) {
		p_preset.set_data(m_data.get_ptr(),m_data.get_size());
		m_data.set_size(0);
	}
private:
	pfc::array_t<t_uint8,pfc::alloc_fast_aggressive> m_data;
};

class dsp_preset_reader : public stream_reader {
public:
	dsp_preset_reader() : m_walk(0) {}
	dsp_preset_reader(const dsp_preset_reader & p_source) : m_walk(0) {*this = p_source;}
	void init(const dsp_preset & p_preset) {
		m_data.set_data_fromptr( (const t_uint8*) p_preset.get_data(), p_preset.get_data_size() );
		m_walk = 0;
	}
	t_size read(void * p_buffer,t_size p_bytes,abort_callback & p_abort) {
		p_abort.check();
		t_size todo = pfc::min_t<t_size>(p_bytes,m_data.get_size()-m_walk);
		memcpy(p_buffer,m_data.get_ptr()+m_walk,todo);
		m_walk += todo;
		return todo;
	}
	bool is_finished() {return m_walk == m_data.get_size();}
private:
	t_size m_walk;
	pfc::array_t<t_uint8> m_data;
};

class dsp_preset_impl : public dsp_preset
{
public:
	dsp_preset_impl() {}
	dsp_preset_impl(const dsp_preset_impl & p_source) {copy(p_source);}
	dsp_preset_impl(const dsp_preset & p_source) {copy(p_source);}
	dsp_preset_impl(dsp_preset_impl && p_source) noexcept {move(p_source);}
	void clear() {m_owner = pfc::guid_null; m_data.clear();}
	bool is_valid() const { return m_owner != pfc::guid_null; }

	const dsp_preset_impl& operator=(const dsp_preset_impl & p_source) {copy(p_source); return *this;}
	const dsp_preset_impl& operator=(const dsp_preset & p_source) {copy(p_source); return *this;}
	const dsp_preset_impl& operator=(dsp_preset_impl&& p_source) noexcept { move(p_source); return *this; }

	GUID get_owner() const {return m_owner;}
	void set_owner(const GUID & p_owner) {m_owner = p_owner;}
	const void * get_data() const {return m_data.ptr();}
	t_size get_data_size() const {return m_data.size();}
	void set_data(const void * p_data,t_size p_data_size) {m_data.set_data_fromptr((const t_uint8*)p_data,p_data_size);}
	void set_data_from_stream(stream_reader * p_stream,t_size p_bytes,abort_callback & p_abort);

	void move(dsp_preset_impl& source) noexcept {
		m_owner = source.m_owner;
		m_data = std::move(source.m_data);
	}

	void set_data(pfc::mem_block&& data) { m_data = std::move(data); }
private:
	GUID m_owner = {};
	pfc::mem_block m_data;
};

class NOVTABLE dsp_preset_edit_callback {
public:
	virtual void on_preset_changed(const dsp_preset &) = 0;
private:
	dsp_preset_edit_callback(const dsp_preset_edit_callback&) = delete;
	const dsp_preset_edit_callback & operator=(const dsp_preset_edit_callback &) = delete;
protected:
	dsp_preset_edit_callback() {}
	~dsp_preset_edit_callback() {}
};

class NOVTABLE dsp_preset_edit_callback_v2 : public service_base {
    FB2K_MAKE_SERVICE_INTERFACE(dsp_preset_edit_callback_v2, service_base);
public:
    virtual void get_preset( dsp_preset & outPreset ) = 0;
    virtual void set_preset( const dsp_preset & inPreset ) = 0;
    virtual void dsp_dialog_done( bool bOK ) = 0;
    void reset();
};


class NOVTABLE dsp_entry : public service_base {
public:
	virtual void get_name(pfc::string_base & p_out) = 0;
	virtual bool get_default_preset(dsp_preset & p_out) = 0;
	virtual bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset) = 0;	
	virtual GUID get_guid() = 0;
	virtual bool have_config_popup() = 0;
#ifdef _WIN32
	//! Shows configuration popup. Call from main thread only! \n
	//! Blocks until done. Returns true if preset has been altered, false otherwise.
	virtual bool show_config_popup(dsp_preset & p_data,fb2k::hwnd_t p_parent) = 0;
#else
	//! Shows configuration popup. Main thread only!
	virtual service_ptr show_config_popup( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback );
#endif

	//! Obsolete method, hidden DSPs now use a different entry class.
	bool is_user_accessible() { return true; }

	static bool g_get_interface(service_ptr_t<dsp_entry> & p_out,const GUID & p_guid);
	static service_ptr_t<dsp_entry> g_get_interface(const GUID&);
	static bool g_instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset);
	static bool g_instantiate_default(service_ptr_t<dsp> & p_out,const GUID & p_guid);
	static bool g_name_from_guid(pfc::string_base & p_out,const GUID & p_guid);
	static bool g_dsp_exists(const GUID & p_guid);
	static bool g_get_default_preset(dsp_preset & p_out,const GUID & p_guid);
	static bool g_have_config_popup(const GUID & p_guid);
	static bool g_have_config_popup(const dsp_preset & p_preset);
#ifdef _WIN32
	//! Shows configuration popup. Main thread only! \n
	//! Blocks until done. Returns true if preset has been altered, false otherwise.
	static bool g_show_config_popup(dsp_preset & p_preset,fb2k::hwnd_t p_parent);
	//! Shows configuration popup. Main thread only!
	//! Blocks until done. Uses callback to notify host about preset change.
	static void g_show_config_popup_v2(const dsp_preset & p_preset,fb2k::hwnd_t p_parent,dsp_preset_edit_callback & p_callback);

	//! Shows configuration popup. Main thread only! \n
	//! Blocks until done. Uses callback to notify host about preset change. \n
	//! Implements a fallback using legacy methods if show_config_popup_v2() is not available. \n
	//! @returns OK/cancel status (true/false), if the dialog supports it; otherwise always true.
	bool show_config_popup_v2_(const dsp_preset& p_preset, fb2k::hwnd_t p_parent, dsp_preset_edit_callback& p_callback);

	//! Shows configuration popup. Main thread only! \n
	//! May either block until done and return null, or run asynchronously and return an object to release to cancel the dialog. \n
	//! Implements a fallback using legacy methods if show_config_popup_v3() is not available.
	service_ptr show_config_popup_v3_(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback);
#endif

	bool get_display_name_supported();
	void get_display_name_(const dsp_preset& arg, pfc::string_base& out);

	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(dsp_entry);
};

class NOVTABLE dsp_entry_v2 : public dsp_entry {
public:
#ifdef _WIN32
	//! Shows configuration popup. Main thread only!
	virtual void show_config_popup_v2(const dsp_preset & p_data,fb2k::hwnd_t p_parent,dsp_preset_edit_callback & p_callback) = 0;
#endif

#ifdef _WIN32
	// Obsolete method, redirected to show_config_popup_v2() by default, no need to implement.
	bool show_config_popup(dsp_preset& p_data, fb2k::hwnd_t p_parent) override;
#endif
private:
    
	FB2K_MAKE_SERVICE_INTERFACE(dsp_entry_v2,dsp_entry);
};

class NOVTABLE dsp_entry_v3 : public dsp_entry_v2 {
	FB2K_MAKE_SERVICE_INTERFACE(dsp_entry_v3, dsp_entry_v2);
public:
	//! Returns the text to show in DSP list, for this specific preset.
	virtual void get_display_name(const dsp_preset& arg, pfc::string_base& out) = 0;

#ifdef _WIN32
	//! Shows configuration popup, asynchronous version - creates dialog then returns immediately. \n
	//! Since not every DSP implements this, caller must be prepated to call legacy blocking show_config_popup methods instead. \n
	//! show_config_popup_v3() may throw pfc::exception_not_implemented() to signal host that this DSP doesn't support this method yet. \n
	//! Main thread only! \n
	//! @returns Object to retain by host, to be released to request the dialog to be closed.
	virtual service_ptr show_config_popup_v3(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback) = 0;
#endif
};

class NOVTABLE dsp_entry_hidden : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(dsp_entry_hidden);
public:
	//! Obsolete method, hidden DSPs now use a different entry class from ordinary ones.
	bool is_user_accessible() {return false; }

	static bool g_get_interface( dsp_entry_hidden::ptr & out, const GUID & guid );
	static bool g_instantiate( dsp::ptr & out, const dsp_preset & preset );
	static bool g_dsp_exists(const GUID & p_guid);

	virtual bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset) = 0;	
	virtual GUID get_guid() = 0;
};

template<class T,class t_entry = dsp_entry>
class dsp_entry_impl_nopreset_t : public t_entry {
public:
	void get_name(pfc::string_base & p_out) {T::g_get_name(p_out);}
	bool get_default_preset(dsp_preset & p_out)
	{
		p_out.set_owner(T::g_get_guid());
		p_out.set_data(0,0);
		return true;
	}
	bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset)
	{
		if (p_preset.get_owner() == T::g_get_guid() && p_preset.get_data_size() == 0)
		{
			p_out = new service_impl_t<T>();
			return p_out.is_valid();
		}
		else return false;
	}
	GUID get_guid() {return T::g_get_guid();}

	bool have_config_popup() {return false;}
	bool show_config_popup(dsp_preset & p_data,fb2k::hwnd_t p_parent) {return false;}
};

template<typename T, typename interface_t>
class dsp_entry_common_t : public interface_t {
public:
	void get_name(pfc::string_base& p_out) override { T::g_get_name(p_out); }
	bool get_default_preset(dsp_preset& p_out) override { return T::g_get_default_preset(p_out); }
	bool instantiate(service_ptr_t<dsp>& p_out, const dsp_preset& p_preset) override {
		if (p_preset.get_owner() == T::g_get_guid()) {
			p_out = new service_impl_t<T>(p_preset);
			return true;
		} else return false;
	}
	GUID get_guid() override { return T::g_get_guid(); }

	bool have_config_popup() override { return T::g_have_config_popup(); }
};

template<class T, class t_entry = dsp_entry>
class dsp_entry_impl_t : public dsp_entry_common_t<T, t_entry> {
public:

#ifdef _WIN32
	bool show_config_popup(dsp_preset & p_data,fb2k::hwnd_t p_parent) override {return T::g_show_config_popup(p_data,p_parent);}
#else
    service_ptr show_config_popup( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback ) override {
        return T::g_show_config_popup(parent, callback);
    }
#endif
};

template<class T, class t_entry = dsp_entry_v2>
class dsp_entry_v2_impl_t : public dsp_entry_common_t<T, t_entry> {
public:
#ifdef _WIN32
	void show_config_popup_v2(const dsp_preset & p_data,fb2k::hwnd_t p_parent,dsp_preset_edit_callback & p_callback) override {T::g_show_config_popup(p_data,p_parent,p_callback);}
#else
    service_ptr show_config_popup( fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback ) override {
        return T::g_show_config_popup(parent, callback);
    }
#endif
};

template<class T, class t_entry = dsp_entry_v3>
class dsp_entry_v3_impl_t : public dsp_entry_v2_impl_t<T, t_entry> {
public:
	void get_display_name(const dsp_preset& arg, pfc::string_base& out) override {
		T::g_get_display_name(arg, out);
	}

#ifdef _WIN32
	service_ptr show_config_popup_v3(fb2k::hwnd_t parent, dsp_preset_edit_callback_v2::ptr callback) override {
		return T::g_show_config_popup_v3(parent, callback);
	}
#endif
};

template<typename T>
class dsp_entry_hidden_t : public dsp_entry_hidden {
public:
	bool instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset) {
		if (p_preset.get_owner() == T::g_get_guid()) {
			p_out = new service_impl_t<T>(p_preset);
			return true;
		} else return false;
	}
	GUID get_guid() {return T::g_get_guid();}
#if 0
	void get_name( pfc::string_base& out ) {out = ""; }
	bool get_default_preset(dsp_preset & p_out) { return false; }
	bool have_config_popup() { return false; }
	bool show_config_popup(dsp_preset & p_data,HWND p_parent) { uBugCheck(); }
	void show_config_popup_v2(const dsp_preset & p_data,HWND p_parent,dsp_preset_edit_callback & p_callback) { uBugCheck(); }

	bool is_user_accessible() { return false; }
#endif
};

template<class T>
class dsp_factory_nopreset_t : public service_factory_single_t<dsp_entry_impl_nopreset_t<T> > {};

template<class T>
class dsp_factory_t : public service_factory_single_t<dsp_entry_v2_impl_t<T> > {};

template<class T>
class dsp_factory_hidden_t : public service_factory_single_t< dsp_entry_hidden_t<T> > {};

class NOVTABLE dsp_chain_config
{
public:
	virtual t_size get_count() const = 0;
	virtual const dsp_preset & get_item(t_size p_index) const = 0;
	virtual void replace_item(const dsp_preset & p_data,t_size p_index) = 0;
	virtual void insert_item(const dsp_preset & p_data,t_size p_index) = 0;
	virtual void remove_mask(const bit_array & p_mask) = 0;
	
	void remove_item(t_size p_index);
	void remove_all();
	void add_item(const dsp_preset & p_data);
	void copy(const dsp_chain_config & p_source);

	const dsp_chain_config & operator=(const dsp_chain_config & p_source) {copy(p_source); return *this;}

	void contents_to_stream(stream_writer * p_stream,abort_callback & p_abort) const;
	void contents_from_stream(stream_reader * p_stream,abort_callback & p_abort);
	fb2k::memBlock::ptr to_blob() const;
	void from_blob(const void* p, size_t size);
	void from_blob(fb2k::memBlock::ptr);

	void instantiate(service_list_t<dsp> & p_out);

	pfc::string8 get_name_list() const;
	void get_name_list(pfc::string_base & p_out) const;

	static bool equals(dsp_chain_config const & v1, dsp_chain_config const & v2);
	static bool equals_debug(dsp_chain_config const& v1, dsp_chain_config const& v2);

	pfc::string8 debug() const;

	bool operator==(const dsp_chain_config & other) const {return equals(*this, other);}
	bool operator!=(const dsp_chain_config & other) const {return !equals(*this, other);}
};

FB2K_STREAM_READER_OVERLOAD(dsp_chain_config) {
	value.contents_from_stream(&stream.m_stream, stream.m_abort); return stream;
}

FB2K_STREAM_WRITER_OVERLOAD(dsp_chain_config) {
	value.contents_to_stream(&stream.m_stream, stream.m_abort); return stream;
}

class dsp_chain_config_impl : public dsp_chain_config
{
public:
	dsp_chain_config_impl() {}
	dsp_chain_config_impl(const dsp_chain_config & p_source) {copy(p_source);}
	dsp_chain_config_impl(const dsp_chain_config_impl & p_source) { copy_v2(p_source);}
	dsp_chain_config_impl(dsp_chain_config_impl&& p_source) noexcept : m_data(std::move(p_source.m_data)) {}
	t_size get_count() const override;
	const dsp_preset & get_item(t_size p_index) const override;
	void replace_item(const dsp_preset & p_data,t_size p_index) override;
	void insert_item(const dsp_preset & p_data,t_size p_index) override;
	void remove_mask(const bit_array & p_mask) override;
	
	const char* get_dsp_name(size_t idx) const;
	void insert_item_v2(const dsp_preset& data, const char* dspName, size_t index);
	void add_item_v2(const dsp_preset& data, const char* dspName);
	void copy_v2(dsp_chain_config_impl const&);
	pfc::string8 debug() const;

	const dsp_chain_config_impl & operator=(const dsp_chain_config & p_source) {copy(p_source); return *this;}
	const dsp_chain_config_impl & operator=(const dsp_chain_config_impl & p_source) {copy_v2(p_source); return *this;}
	const dsp_chain_config_impl & operator=(dsp_chain_config_impl&& p_source) noexcept { m_data = std::move(p_source.m_data); p_source.m_data.remove_all(); return *this; }

	~dsp_chain_config_impl();

	void reorder( const size_t * order, size_t count );

	void supply_name(size_t idx, pfc::string8 && name) { m_data[idx]->dspName = std::move(name); }
	const char* find_dsp_name(const GUID& guid) const;
private:

	struct entry_t {
		dsp_preset_impl data;
		pfc::string8 dspName;
	};


	pfc::ptr_list_t<entry_t> m_data;
};

//! Helper.
class dsp_preset_parser : public stream_reader_formatter<> {
public:
	dsp_preset_parser(const dsp_preset& in) : stream_reader_formatter(_m_stream, fb2k::noAbort), m_data(in), _m_stream(in.get_data(), in.get_data_size()) {}

	void reset() { _m_stream.reset(); }
	t_size get_remaining() const { return _m_stream.get_remaining(); }

	void assume_empty() const {
		if (get_remaining() != 0) throw exception_io_data();
	}

	GUID get_owner() const { return m_data.get_owner(); }
private:
	const dsp_preset& m_data;
	stream_reader_memblock_ref _m_stream;
};

//! Helper.
class dsp_preset_builder : public stream_writer_formatter<> {
public:
	dsp_preset_builder() : stream_writer_formatter(_m_stream, fb2k::noAbort) {}
	void finish(const GUID& id, dsp_preset& out) {
		out.set_owner(id);
		out.set_data(_m_stream.m_buffer.get_ptr(), _m_stream.m_buffer.get_size());
	}
	void reset() {
		_m_stream.m_buffer.set_size(0);
	}
private:
	stream_writer_buffer_simple _m_stream;
};

#endif
