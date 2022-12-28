#pragma once
namespace fb2k {
	//! pfc::splitThread() + async_task_manager::acquire
	void splitTask(std::function<void()>);
	void splitTask(pfc::thread::arg_t const&, std::function<void()>);
}



// ======================================================================================================
// Most of main_thread_callback.h declares API internals and obsolete helpers.
// In modern code, simply use fb2k::inMainThread() declared below and disregard the rest.
// ======================================================================================================
namespace fb2k {
	//! Queue a call in main thread. Returns immediately. \n
	//! You can call this from any thread, including main thread - to execute some code outside the current call stack / global fb2k callbacks / etc.
	void inMainThread(std::function<void() > f);
	//! Call f synchronously if called from main thread, queue call if called from another.
	void inMainThread2(std::function<void() > f);

	//! Synchronous version.
	void inMainThreadSynchronous(std::function<void() > f, abort_callback& abort);
}
