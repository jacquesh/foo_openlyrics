#include "foobar2000-sdk-pch.h"
#include "main_thread_callback.h"
#include "abort_callback.h"
#include <memory>
#include <pfc/synchro.h>


void main_thread_callback::callback_enqueue() {
	main_thread_callback_manager::get()->add_callback(this);
}

void main_thread_callback_add(main_thread_callback::ptr ptr) {
	main_thread_callback_manager::get()->add_callback(ptr);
}

namespace {
    typedef std::function<void ()> func_t;
    class mtcallback_func : public main_thread_callback {
    public:
        mtcallback_func(func_t const & f) : m_f(f) {}
        
        void callback_run() {
            m_f();
        }
        
    private:
        func_t m_f;
    };
}

void fb2k::inMainThread( std::function<void () > f ) {
    main_thread_callback_add( new service_impl_t<mtcallback_func>(f));
}

void fb2k::inMainThread2( std::function<void () > f ) {
	if ( core_api::is_main_thread() ) {
		f();
	} else {
		inMainThread(f);
	}
}

void fb2k::inMainThreadSynchronous(std::function<void() > f, abort_callback & abort) {
	abort.check();

	if (core_api::is_main_thread()) {
		f(); return;
	}

	auto evt = std::make_shared<pfc::event>();
	auto f2 = [f, evt] {
		f();
		evt->set_state(true);
	};
	inMainThread(f2);
	abort.waitForEvent(*evt);
}

void fb2k::inMainThreadSynchronous2(std::function<void() > f) {
	// Have new API?
	auto api = main_thread_callback_manager_v2::tryGet();
	if (api.is_valid()) {
		api->run_synchronously( fb2k::service_new<mtcallback_func>(f) );
		return;
	}

	inMainThreadSynchronous(f, fb2k::noAbort);
}