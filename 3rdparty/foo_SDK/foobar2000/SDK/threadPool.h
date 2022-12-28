#pragma once

#include <functional>
namespace fb2k {
	void inWorkerThread(std::function<void()> f);
	void inCpuWorkerThread(std::function<void()> f);

	//! \since 2.0
	class threadEntry : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE(threadEntry, service_base);
	public:
		virtual void run() = 0;

		static threadEntry::ptr make(std::function<void()> f);
	};

	//! \since 2.0
	//! A class interfacing with shared pool of threads intended for use with CPU bound tasks. \n
	//! Use this instead of creating threads directly to avoid making the system unresponsive by creating too many CPU bound threads.
	class cpuThreadPool : public service_base {
		FB2K_MAKE_SERVICE_COREAPI(cpuThreadPool);
	public:
		//! Run one time task on a thread pool. Returns immediately.
		virtual void runSingle(threadEntry::ptr trd) = 0;
		//! Run the task multiple times concurrently. Optionally block until all done. \n
		//! Exceptions: if nonblocking, they're discarded; if blocking; rethrown to caller. If multiple are thrown, one of them is rethrown, precedence undefined.
		virtual void runMulti(threadEntry::ptr trd, size_t numRuns, bool blocking) = 0;

		virtual size_t numRunsSanity(size_t suggested) = 0;

		//! Helper around blocking runMulti(), with fallback for pre-v2.0
		static void runMultiHelper(std::function<void()>, size_t numRuns);
		void runMulti_(std::function<void()>, size_t numRuns);
	};
}

