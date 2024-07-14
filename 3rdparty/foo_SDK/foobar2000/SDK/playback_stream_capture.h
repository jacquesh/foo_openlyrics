#pragma once

//! \since 1.0
//! Implemented by components - register with playback_stream_capture methods.
class NOVTABLE playback_stream_capture_callback {
public:
	//! Delivers a real-time chunk of audio data. \n
	//! Audio is roughly synchronized with what can currently be heard. This API is provided for utility purposes such as streaming; if you want to implement a visualisation, use the visualisation_manager API instead. \n
	//! Contrary to visualisation methods, this guarantees that all played audio data is coming thru. \n
	//! Called only from the main thread. \n
	virtual void on_chunk(const audio_chunk &) = 0;
protected:
	playback_stream_capture_callback() {}
	~playback_stream_capture_callback() {}
};

//! \since 1.0
//! Implemented by core.
class NOVTABLE playback_stream_capture : public service_base {
	FB2K_MAKE_SERVICE_COREAPI(playback_stream_capture)
public:
	//! Register a playback_stream_capture_callback. \n
	//! Possible to call only from the main thread.
	virtual void add_callback(playback_stream_capture_callback * ) = 0;
	//! Un-register a playback_stream_capture_callback. \n
	//! Possible to call only from the main thread.
	virtual void remove_callback(playback_stream_capture_callback * ) = 0;
};

//! \since 2.0.
//! Implemented by core.
class NOVTABLE playback_stream_capture_v2 : public playback_stream_capture {
	FB2K_MAKE_SERVICE_COREAPI_EXTENSION(playback_stream_capture_v2, playback_stream_capture);
public:
	//! @param requestInterval Interval, in seconds, in which the callback expects to be called. \n
	//! Set to -1 to use defaults. \n
	//! Note that if many callbacks are registered, they all get called at once; one callback requesting lower interval lowers the interval for all.
	virtual void add_callback_v2(playback_stream_capture_callback* cb, double requestInterval = -1) = 0;
};

class playback_stream_capture_callback_impl : public playback_stream_capture_callback {
public:
	void on_chunk(const audio_chunk&) override {}

	playback_stream_capture_callback_impl(double interval = -1) {
		PFC_ASSERT(core_api::is_main_thread());
#if FOOBAR2020
		playback_stream_capture_v2::get()->add_callback_v2(this, interval);
#else
		auto api = playback_stream_capture::get();
		playback_stream_capture_v2::ptr v2;
		if (v2 &= api) v2->add_callback_v2(this, interval);
		else api->add_callback(this);
#endif
	}
	~playback_stream_capture_callback_impl() {
		PFC_ASSERT(core_api::is_main_thread());
		playback_stream_capture::get()->remove_callback(this);
	}

	playback_stream_capture_callback_impl(const playback_stream_capture_callback_impl&) = delete;
	void operator=(const playback_stream_capture_callback_impl&) = delete;
};
