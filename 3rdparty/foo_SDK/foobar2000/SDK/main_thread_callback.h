#pragma once
#include <functional>

// fb2k::inMainThread moved to threadsLite.h



// ======================================================================================================
// API declarations
// ======================================================================================================

//! Callback object class for main_thread_callback_manager service. \n
//! You do not need to implement this directly - simply use fb2k::inMainThread() with a lambda.
class NOVTABLE main_thread_callback : public service_base {
public:
	//! Gets called from main app thread. See main_thread_callback_manager description for more info.
	virtual void callback_run() = 0;
    
    void callback_enqueue(); // helper

	FB2K_MAKE_SERVICE_INTERFACE(main_thread_callback,service_base);
};

/*!
Allows you to queue a callback object to be called from main app thread. This is commonly used to trigger main-thread-only API calls from worker threads.\n
This can be also used from main app thread, to avoid race conditions when trying to use APIs that dispatch global callbacks from inside some other global callback, or using message loops / modal dialogs inside global callbacks. \n
There is no need to use this API directly - use fb2k::inMainThread() with a lambda.
*/
class NOVTABLE main_thread_callback_manager : public service_base {
public:
	//! Queues a callback object. This can be called from any thread, implementation ensures multithread safety. Implementation will call p_callback->callback_run() once later. To get it called repeatedly, you would need to add your callback again. \n
    //! Queued callbacks will be called in the same order as they were queued (FIFO).
	virtual void add_callback(service_ptr_t<main_thread_callback> p_callback) = 0;

	FB2K_MAKE_SERVICE_COREAPI(main_thread_callback_manager);
};

//! \since 2.0
//! Additional method added to help recovering from method-called-in-wrong-thread scenarios.
class NOVTABLE main_thread_callback_manager_v2 : public main_thread_callback_manager {
    FB2K_MAKE_SERVICE_COREAPI_EXTENSION(main_thread_callback_manager_v2, main_thread_callback_manager)
public:
    //! Uses win32 SendMessage() to invoke your callback synchronously, blocks until done. \n
    //! If multiple threads call this, order of callbacks is undefined. \n
    //! Callbacks run_synchronously() callbacks take precedence over add_callback().
    virtual void run_synchronously( main_thread_callback::ptr ) = 0;
};

// ======================================================================================================
// Obsolete helpers - they still work, but it's easier to just use fb2k::inMainThread().
// ======================================================================================================

//! Helper, equivalent to main_thread_callback_manager::get()->add_callback(ptr)
void main_thread_callback_add(main_thread_callback::ptr ptr);

template<typename t_class> static void main_thread_callback_spawn() {
	main_thread_callback_add(new service_impl_t<t_class>);
}
template<typename t_class, typename t_param1> static void main_thread_callback_spawn(const t_param1 & p1) {
	main_thread_callback_add(new service_impl_t<t_class>(p1));
}
template<typename t_class, typename t_param1, typename t_param2> static void main_thread_callback_spawn(const t_param1 & p1, const t_param2 & p2) {
	main_thread_callback_add(new service_impl_t<t_class>(p1, p2));
}

// More legacy helper code moved to helpers/callInMainThreadHelper.h