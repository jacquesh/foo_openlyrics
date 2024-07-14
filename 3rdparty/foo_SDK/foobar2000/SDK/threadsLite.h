#pragma once
namespace fb2k {
	//! pfc::splitThread() + async_task_manager::acquire
	void splitTask(std::function<void()>);
	void splitTask(pfc::thread::arg_t const&, std::function<void()>);
	abort_callback& mainAborter();

	void inCpuWorkerThread(std::function<void()> f);
}



// ======================================================================================================
// Most of main_thread_callback.h declares API internals and obsolete helpers.
// In modern code, simply use fb2k::inMainThread() declared below and disregard the rest.
// ======================================================================================================
namespace fb2k {
	//! Queue a call in main thread. Returns immediately. \n
	//! You can call this from any thread, including main thread - to execute some code outside the current call stack / global fb2k callbacks / etc. \n
	//! Guaranteed FIFO order of execution. See also: main_thread_callback::add_callback().
	void inMainThread(std::function<void() > f);
	//! Call f synchronously if called from main thread, queue call if called from another.
	void inMainThread2(std::function<void() > f);

	//! Synchronous / abortable version. May exit *before* f() finishes, if abort becomes set.
	void inMainThreadSynchronous(std::function<void() > f, abort_callback& abort);

	//! Synchronous blocking version. \n
	//! Uses new foobar2000 v2.0 methods if available, synchronizing to main thread via SendMessage(). \n
	//! Introduced to help recovering from method-called-from-wrong-context scenarios. Does *not* guarentee FIFO execution order contrary to plain inMainThread().
	void inMainThreadSynchronous2(std::function<void() > f);

	//! Helper class for threads that call fb2k objects. Mainly needed for Android shims. You can safely ignore this. \n
	//! Guaranteed to have startHere(), isActive() and waitTillDone() methods only.
	typedef pfc::thread2 thread;
}
