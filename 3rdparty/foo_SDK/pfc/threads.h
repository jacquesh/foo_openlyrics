#pragma once

#include <functional>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif
namespace pfc {
	unsigned getOptimalWorkerThreadCount();
	unsigned getOptimalWorkerThreadCountEx(size_t taskCountLimit);

	//! IMPORTANT: all classes derived from thread must call waitTillDone() in their destructor, to avoid object destruction during a virtual function call!
	class thread {
	public:
#ifdef _WIN32
        typedef HANDLE handle_t;
#else
        typedef pthread_t handle_t;
#endif
        // Returns a psuedo-handle meant only for local use, see win32 GetCurrentThread() semantics
        static handle_t handleToCurrent();

        struct arg_t {
#ifdef _WIN32
            int winThreadPriority = 0; // THREAD_PRIORITY_NORMAL
#endif
#ifdef __APPLE__
            // see: pthread_set_qos_class_self_np
            int appleThreadQOS = 0; // QOS_CLASS_UNSPECIFIED
            int appleRealtivePriority = 0; // valid only if appleThreadQOS is set
#endif
#ifndef _WIN32
            int nixSchedPolicy = -1; // not set - cannot use 0 because SCHED_OTHER is 0 on Linux
            sched_param nixSchedParam = {}; // valid only if nixSchedPolicy is set
#endif
        };

		static arg_t argDefault();        
        static arg_t argCurrentThread();
        static arg_t argBackground();
        static arg_t argHighPriority();
        static arg_t argPlayback();
        static arg_t argUserInitiated();
#ifdef _WIN32
        static arg_t argWinPriority(int priority);
#endif
#ifdef __APPLE__
		static arg_t argAppleQOS(int qos);
#endif
#ifndef _WIN32
		static arg_t argNixPriority( int policy, int percent );
#endif

		//! Critical error handler function, never returns
		PFC_NORETURN static void couldNotCreateThread();

		thread();
		~thread() {PFC_ASSERT(!isActive()); waitTillDone();}
        void start( arg_t const & arg = argCurrentThread() );
		bool isActive() const;
		void waitTillDone() {close();}
#ifdef _WIN32
		void winStart(int priority, DWORD * outThreadID); 
		HANDLE winThreadHandle() { return m_thread; }
#else
		pthread_t posixThreadHandle() { return m_thread; }
#endif
#ifdef __APPLE__
		static void appleStartThreadPrologue();
#endif
        
        static void setCurrentPriority(arg_t const&);
	protected:
		virtual void threadProc() {PFC_ASSERT(!"Stub thread entry - should not get here");}
	private:
		void close();
#ifdef _WIN32
		static unsigned CALLBACK g_entry(void* p_instance);
#else
		static void * g_entry( void * arg );
#endif
        void entry();
        
        handle_t m_thread;
#ifndef _WIN32
        bool m_threadValid; // there is no invalid pthread_t, so we keep a separate 'valid' flag
#endif
        
		PFC_CLASS_NOT_COPYABLE_EX(thread)
	};

	//! Thread class using lambda entrypoint rather than function override
	class thread2 : public thread {
	public:
		~thread2() { waitTillDone(); }
		void startHere(std::function<void()> e);
		void startHere(arg_t const& arg, std::function<void()> e);
		void setEntry(std::function<void()> e);
	private:
		void threadProc();

		std::function<void()> m_entryPoint;
	};

	void splitThread(std::function<void() > f);
    void splitThread(pfc::thread::arg_t const & arg, std::function<void()> f);

	//! Apple specific; executes the function in a release pool scope. \n
	//! On non Apple platforms it just invokes the function.
	void inAutoReleasePool(std::function<void()> f);
}
