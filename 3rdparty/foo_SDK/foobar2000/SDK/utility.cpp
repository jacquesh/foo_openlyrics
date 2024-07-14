#include "foobar2000-sdk-pch.h"
#include "foosort.h"
#include <functional>

namespace pfc {
	/*
	Redirect PFC methods to shared.dll
	If you're getting linker multiple-definition errors on these, change build configuration of PFC from "Debug" / "Release" to "Debug FB2K" / "Release FB2K"
	*/
#ifdef _WIN32
	BOOL winFormatSystemErrorMessageHook(pfc::string_base & p_out, DWORD p_code) {
		return uFormatSystemErrorMessage(p_out, p_code);
	}
#endif
	void crashHook() {
		uBugCheck();
	}
}


// file_lock_manager.h functionality
#include "file_lock_manager.h"
namespace {
    class file_lock_interrupt_impl : public file_lock_interrupt {
    public:
        void interrupt( abort_callback & a ) { f(a); }
        std::function<void (abort_callback&)> f;
    };
}

file_lock_interrupt::ptr file_lock_interrupt::create( std::function< void (abort_callback&)> f ) {
    service_ptr_t<file_lock_interrupt_impl> i = new service_impl_t<file_lock_interrupt_impl>();
    i->f = f;
    return i;
}

// file_info_filter.h functionality
#include "file_info_filter.h"
namespace {
    class file_info_filter_lambda : public file_info_filter {
    public:
        bool apply_filter(trackRef p_track,t_filestats p_stats,file_info & p_info) override {
            return f(p_track, p_stats, p_info);
        }
        func_t f;
    };
}

file_info_filter::ptr file_info_filter::create(func_t f) {
    auto o = fb2k::service_new<file_info_filter_lambda>();
    o->f = f;
    return o;
}

// threadPool.h functionality
#include "threadPool.h"
namespace fb2k {
	void inWorkerThread(std::function<void()> f) {
		fb2k::splitTask(f);
	}
	void inCpuWorkerThread(std::function<void()> f) {
		cpuThreadPool::get()->runSingle(threadEntry::make(f));
	}
}
namespace {
	class threadEntryImpl : public fb2k::threadEntry {
	public:
		void run() override { f(); }
		std::function<void()> f;
	};
}
namespace fb2k {
	threadEntry::ptr threadEntry::make(std::function<void()> f) {
		auto ret = fb2k::service_new<threadEntryImpl>();
		ret->f = f;
		return ret;
	}

	void cpuThreadPool::runMulti_(std::function<void()> f, size_t numRuns) {
		this->runMulti(threadEntry::make(f), numRuns, true);
	}

	void cpuThreadPool::runMultiHelper(std::function<void()> f, size_t numRuns) {
		if (numRuns == 0) return;
#if FOOBAR2000_TARGET_VERSION >= 81
		get()->runMulti_(f, numRuns);
#else
		if (numRuns == 1) {
			f();
			return;
		}

		size_t numThreads = numRuns - 1;
		struct rec_t {
			pfc::thread2 trd;
			std::exception_ptr ex;
		};
		pfc::array_staticsize_t<rec_t> threads;
		threads.set_size_discard(numThreads);
		for (size_t walk = 0; walk < numThreads; ++walk) {
			threads[walk].trd.startHere([f, &threads, walk] {
				try {
					f();
				} catch (...) {
					threads[walk].ex = std::current_exception();
				}
			});
		}
		std::exception_ptr exMain;
		try {
			f();
		} catch (...) {
			exMain = std::current_exception();
		}

		for (size_t walk = 0; walk < numThreads; ++walk) {
			threads[walk].trd.waitTillDone();
		}
		if (exMain) std::rethrow_exception(exMain);
		for (size_t walk = 0; walk < numThreads; ++walk) {
			auto & ex = threads[walk].ex;
			if (ex) std::rethrow_exception(ex);
		}
#endif
	}
}


// timer.h functionality
#include "timer.h"
#include <memory>

namespace fb2k {
	void callLater(double timeAfter, std::function< void() > func) {
		PFC_ASSERT( core_api::is_main_thread() );
		auto releaseMe = std::make_shared<objRef>();
		*releaseMe = registerTimer(timeAfter, [=] {
			if (releaseMe->is_valid()) {
				releaseMe->release();
				func();
			}
		});
	}
	objRef registerTimer(double interval, std::function<void()> func) {
		PFC_ASSERT( core_api::is_main_thread() );
		return static_api_ptr_t<timerManager>()->addTimer(interval, makeCompletionNotify([func](unsigned) { func(); }));
	}
}

// autoplaylist.h functionality
#include "autoplaylist.h"

bool autoplaylist_client::supports_async_() {
	autoplaylist_client_v3::ptr v3;
	bool rv = false;
	if (v3 &= this) {
		rv = v3->supports_async();
	}
	return rv;
}

bool autoplaylist_client::supports_get_contents_() {
	autoplaylist_client_v3::ptr v3;
	bool rv = false;
	if (v3 &= this) {
		rv = v3->supports_get_contents();
	}
	return rv;
}

void autoplaylist_client_v3::filter(metadb_handle_list_cref data, bool * out) {
    filter_v2(data, nullptr, out, fb2k::noAbort);
}
bool autoplaylist_client_v3::sort(metadb_handle_list_cref p_items,t_size * p_orderbuffer) {
    return sort_v2(p_items, p_orderbuffer, fb2k::noAbort);
}


#include "noInfo.h"
namespace fb2k {
	noInfo_t noInfo;
}


// library_callbacks.h functionality
#include "library_callbacks.h"

bool library_callback::is_modified_from_hook() {
	auto api = library_manager_v5::tryGet();
	if (api.is_valid()) return api->library_status(library_manager_v5::status_current_callback_from_hook, 0, nullptr, 0) != 0;
	else return false;
}

void library_callback_dynamic::register_callback() {
	library_manager_v3::get()->register_callback(this);
}
void library_callback_dynamic::unregister_callback() {
	library_manager_v3::get()->unregister_callback(this);
}

void library_callback_v2_dynamic::register_callback() {
	library_manager_v4::get()->register_callback_v2(this);
}

void library_callback_v2_dynamic::unregister_callback() {
	library_manager_v4::get()->unregister_callback_v2(this);
}



// configCache.h functionality
#include "configCache.h"
#include "configStore.h"

bool fb2k::configBoolCache::get() {
	std::call_once(m_init, [this] {
		auto api = fb2k::configStore::get();
		auto refresh = [this, api] { m_value = api->getConfigBool(m_var, m_def); };
		api->addPermanentNotify(m_var, refresh);
		refresh();
	});
	return m_value;
}

int64_t fb2k::configIntCache::get() {
	std::call_once(m_init, [this] {
		auto api = fb2k::configStore::get();
		auto refresh = [this, api] { m_value = api->getConfigInt(m_var, m_def); };
		api->addPermanentNotify(m_var, refresh);
		refresh();
	});
	return m_value;
}

void fb2k::configBoolCache::set(bool v) {
	m_value = v;
	fb2k::configStore::get()->setConfigBool(m_var, v);
}

void fb2k::configIntCache::set(int64_t v) {
	m_value = v;
	fb2k::configStore::get()->setConfigBool(m_var, v);
}


// keyValueIO.h functionality
#include "keyValueIO.h"

int fb2k::keyValueIO::getInt( const char * name ) {
    auto str = get(name);
    if ( str.is_empty() ) return 0;
    return (int) pfc::atoi64_ex( str->c_str(), str->length() );
}

void fb2k::keyValueIO::putInt( const char * name, int val ) {
    put( name, pfc::format_int(val) );
}


// fileDialog.h functionality
#include "fileDialog.h"
#include "fsitem.h"

namespace {
    using namespace fb2k;
    class fileDialogNotifyImpl : public fileDialogNotify {
    public:
        void dialogCancelled() {}
        void dialogOK2(arrayRef fsItems) {
            recv(fsItems);
        }

        std::function< void (arrayRef) > recv;
    };
}

fb2k::fileDialogNotify::ptr fb2k::fileDialogNotify::create( std::function<void (arrayRef) > recv ) {
    service_ptr_t<fileDialogNotifyImpl> obj = new service_impl_t< fileDialogNotifyImpl >();
    obj->recv = recv;
    return obj;
}

void fb2k::fileDialogSetup::run(fileDialogReply_t reply) {
    this->run(fb2k::fileDialogNotify::create(reply));
}
void fb2k::fileDialogSetup::runSimple(fileDialogGetPath_t reply) {
    fb2k::fileDialogReply_t wrapper = [reply] (fb2k::arrayRef arg) {
        if ( arg.is_empty() ) {PFC_ASSERT(!"???"); return; }
        if ( arg->size() != 1 ) { PFC_ASSERT(!"???"); return; }
        auto obj = arg->itemAt(0);
        fsItemBase::ptr fsitem;
        if ( fsitem &= obj ) {
            reply( fsitem->canonicalPath() ); return;
        }
        fb2k::stringRef str;
        if ( str &= obj ) {
            reply(str); return;
        }
        PFC_ASSERT( !"???" );
    };
    this->run(wrapper);
}

#include "input_file_type.h"

void fb2k::fileDialogSetup::setAudioFileTypes() {
    pfc::string8 temp;
    input_file_type::build_openfile_mask(temp);
    this->setFileTypes( temp );
}


#include "search_tools.h"

void search_filter_v2::test_multi_here(metadb_handle_list& ref, abort_callback& abort) {
	pfc::array_t<bool> mask; mask.resize(ref.get_size());
	this->test_multi_ex(ref, mask.get_ptr(), abort);
	ref.filter_mask(mask.get_ptr());
}



// core_api.h

namespace fb2k {
	bool isDebugModeActive() {
#if PFC_DEBUG
		return true;
#else
		auto api = fb2k::configStore::tryGet();
		if (api.is_empty()) return false;
		return api->getConfigBool("core.debugMode");
#endif
	}

#if FB2K_SUPPORT_LOW_MEM_MODE
	static bool _isLowMemModeActive() {
		auto api = fb2k::configStore::tryGet();
		if (api.is_empty()) return false;
		return api->getConfigBool("core.lowMemMode");
	}

	bool isLowMemModeActive() {
		static bool cached = _isLowMemModeActive();
		return cached;
	}
#endif
}

// callback_merit.h
namespace fb2k {
	callback_merit_t callback_merit_of(service_ptr obj) {
		{
			callback_with_merit::ptr q;
			if (q &= obj) return q->get_callback_merit();
		}
		{
			metadb_io_callback_v2::ptr q;
			if (q &= obj) return q->get_callback_merit();
		}
		return callback_merit_default;
	}
}

#ifdef _WIN32
#include "message_loop.h"
message_filter_impl_base::message_filter_impl_base() {
	PFC_ASSERT( core_api::is_main_thread() );
	message_loop::get()->add_message_filter(this);
}
message_filter_impl_base::message_filter_impl_base(t_uint32 lowest, t_uint32 highest) {
	PFC_ASSERT( core_api::is_main_thread() );
	message_loop_v2::get()->add_message_filter_ex(this, lowest, highest);
}
message_filter_impl_base::~message_filter_impl_base() {
	PFC_ASSERT( core_api::is_main_thread() );
	message_loop::get()->remove_message_filter(this);
}

bool message_filter_impl_accel::pretranslate_message(MSG * p_msg) {
	if (m_wnd != NULL) {
		if (GetActiveWindow() == m_wnd) {
			if (TranslateAccelerator(m_wnd,m_accel.get(),p_msg) != 0) {
				return true;
			}
		}
	}
	return false;
}

message_filter_impl_accel::message_filter_impl_accel(HINSTANCE p_instance,const TCHAR * p_accel) {
	m_accel.load(p_instance,p_accel);
}

bool message_filter_remap_f1::pretranslate_message(MSG * p_msg) {
	if (IsOurMsg(p_msg) && m_wnd != NULL && GetActiveWindow() == m_wnd) {
		::PostMessage(m_wnd, WM_SYSCOMMAND, SC_CONTEXTHELP, -1);
		return true;
	}
	return false;
}

#endif
