#include "pfc-lite.h"

#ifdef _WIN32
#include "pp-winapi.h"
#endif

#ifdef __APPLE__
#include <sys/sysctl.h>
#include <sys/stat.h>
#endif

#include "threads.h"
#include "debug.h"
#include "ptrholder.h"

#include <thread>

#ifndef _WIN32
namespace {
    class c_pthread_attr {
    public:
        c_pthread_attr() {
            pthread_attr_init(&a);
        }
        ~c_pthread_attr() {
            pthread_attr_destroy(&a);
        }
        void apply( const pfc::thread::arg_t & arg ) {
#ifdef __APPLE__
            if ( arg.appleThreadQOS != 0 ) {
                pthread_attr_set_qos_class_np( &a, (qos_class_t) arg.appleThreadQOS, arg.appleRealtivePriority );
            }
#endif
            if ( arg.nixSchedPolicy >= 0 ) {
                pthread_attr_setschedpolicy(&a, arg.nixSchedPolicy);
                pthread_attr_setschedparam(&a, &arg.nixSchedParam);
            }
        }
        pthread_attr_t a;
        
        c_pthread_attr( const c_pthread_attr & ) = delete;
        void operator=( const c_pthread_attr & ) = delete;
    };
}
#endif

namespace pfc {
	unsigned getOptimalWorkerThreadCount() {
		return std::thread::hardware_concurrency();
	}

	unsigned getOptimalWorkerThreadCountEx(size_t taskCountLimit) {
		if (taskCountLimit <= 1) return 1;
		return (unsigned)pfc::min_t<size_t>(taskCountLimit,getOptimalWorkerThreadCount());
	}
    
    
    void thread::entry() {
        try {
            threadProc();
        } catch(...) {}
    }

	void thread::couldNotCreateThread() {
	    PFC_ASSERT(!"Could not create thread");
		// Something terminally wrong, better crash leaving a good debug trace
		crash();
	}
    
#ifdef _WIN32
    thread::thread() : m_thread(INVALID_HANDLE_VALUE)
    {
    }
    
    void thread::close() {
        if (isActive()) {
            
			int ctxPriority = GetThreadPriority( GetCurrentThread() );
			if (ctxPriority > GetThreadPriority( m_thread ) ) SetThreadPriority( m_thread, ctxPriority );
            
            if (WaitForSingleObject(m_thread,INFINITE) != WAIT_OBJECT_0) couldNotCreateThread();
            CloseHandle(m_thread); m_thread = INVALID_HANDLE_VALUE;
        }
    }
    bool thread::isActive() const {
        return m_thread != INVALID_HANDLE_VALUE;
    }

	static HANDLE MyBeginThread( unsigned (__stdcall * proc)(void *) , void * arg, DWORD * outThreadID, int priority) {
		HANDLE thread;
#ifdef PFC_WINDOWS_DESKTOP_APP
		thread = (HANDLE)_beginthreadex(NULL, 0, proc, arg, CREATE_SUSPENDED, (unsigned int*)outThreadID);
#else
		thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, arg, CREATE_SUSPENDED, outThreadID);
#endif
		if (thread == NULL) thread::couldNotCreateThread();
		SetThreadPriority(thread, priority);
		ResumeThread(thread);
		return thread;
	}
	void thread::winStart(int priority, DWORD * outThreadID) {
		close();
		m_thread = MyBeginThread(g_entry, reinterpret_cast<void*>(this), outThreadID, priority);
	}
    void thread::start(arg_t const & arg) {
        winStart(arg.winThreadPriority, nullptr);
    }
    
    unsigned CALLBACK thread::g_entry(void* p_instance) {
        reinterpret_cast<thread*>(p_instance)->entry(); return 0;
    }

#else
    thread::thread() : m_thread(), m_threadValid() {
    }
    
#ifndef __APPLE__ // Apple specific entrypoint in obj-c.mm
    void * thread::g_entry( void * arg ) {
        reinterpret_cast<thread*>( arg )->entry();
        return NULL;
    }
#endif
    
    void thread::start(arg_t const & arg) {
        close();
#ifdef __APPLE__
        appleStartThreadPrologue();
#endif
        pthread_t thread;
        
        c_pthread_attr attr;
        
        pthread_attr_setdetachstate(&attr.a, PTHREAD_CREATE_JOINABLE);
        
        attr.apply( arg );
        
        if (pthread_create(&thread, &attr.a, g_entry, reinterpret_cast<void*>(this)) != 0) couldNotCreateThread();
        
        m_threadValid = true;
        m_thread = thread;
    }
    
    void thread::close() {
        if (m_threadValid) {
            void * rv = NULL;
            pthread_join( m_thread, & rv ); m_thread = 0;
            m_threadValid = false;
        }
    }
    
    bool thread::isActive() const {
        return m_threadValid;
    }
#endif
#ifdef _WIN32
    unsigned CALLBACK winSplitThreadProc(void* arg) {
		auto f = reinterpret_cast<std::function<void() > *>(arg);
		(*f)();
		delete f;
		return 0;
    }
#else
	void * nixSplitThreadProc(void * arg) {
		auto f = reinterpret_cast<std::function<void() > *>(arg);
#ifdef __APPLE__
		inAutoReleasePool([f] {
			(*f)();
			delete f;
		});
#else
		(*f)();
		delete f;
#endif
		return NULL;
	}
#endif

    void splitThread( std::function<void ()> f ) {
        splitThread( thread::argCurrentThread(), f );
    }
	
    void splitThread(thread::arg_t const & arg, std::function<void() > f) {
		ptrholder_t< std::function<void() > > arg2 ( new std::function<void() >(f) );
#ifdef _WIN32
		HANDLE h = MyBeginThread(winSplitThreadProc, arg2.get_ptr(), NULL, arg.winThreadPriority );
		CloseHandle(h);
#else
#ifdef __APPLE__
		thread::appleStartThreadPrologue();
#endif
		pthread_t thread;

        c_pthread_attr attr;
                
        attr.apply( arg );

        if (pthread_create(&thread, &attr.a, nixSplitThreadProc, arg2.get_ptr()) != 0) thread::couldNotCreateThread();

		pthread_detach(thread);
#endif
        arg2.detach();
	}
#ifndef __APPLE__
	// Stub for non Apple
	void inAutoReleasePool(std::function<void()> f) { f(); }
#endif


    void thread2::startHere(std::function<void()> e) {
        setEntry(e); start();
    }
    void thread2::startHere(arg_t const& arg, std::function<void()> e) {
        setEntry(e); start(arg);
    }

	void thread2::setEntry(std::function<void()> e) {
		PFC_ASSERT(!isActive());
		m_entryPoint = e;
	}

	void thread2::threadProc() {
		m_entryPoint();
	}

    thread::handle_t thread::handleToCurrent() {
#ifdef _WIN32
        return GetCurrentThread();
#else
        return pthread_self();
#endif
    }

    thread::arg_t thread::argDefault() {
	    return arg_t();
    }

    thread::arg_t thread::argCurrentThread() {
        arg_t arg;
#ifdef _WIN32
        arg.winThreadPriority = GetThreadPriority( GetCurrentThread() );
#endif
#ifdef __APPLE__
        qos_class_t qos_class = QOS_CLASS_UNSPECIFIED;
        int relative_priority = 0;
        if (pthread_get_qos_class_np(pthread_self(), &qos_class, &relative_priority) == 0) {
            arg.appleThreadQOS = (int) qos_class;
            arg.appleRealtivePriority = relative_priority;
        }
#endif
#ifndef _WIN32
        {
            int policy; sched_param param;
            if (pthread_getschedparam( pthread_self(), &policy, &param) == 0) {
                arg.nixSchedPolicy = policy;
                arg.nixSchedParam = param;
            }
        }
#endif
        return arg;
    }

    thread::arg_t thread::argBackground() {
#ifdef _WIN32
        return argWinPriority(THREAD_PRIORITY_LOWEST);
#elif defined(__APPLE__)
        return argAppleQOS((int)QOS_CLASS_BACKGROUND);
#else
        return argNixPriority(SCHED_OTHER, 0);
#endif
    }

    thread::arg_t thread::argHighPriority() {
#ifdef _WIN32
        return argWinPriority(THREAD_PRIORITY_HIGHEST);
#elif defined(__ANDROID__)
        // No SCHED_FIFO or SCHED_RR on Android
        return argNixPriority(SCHED_OTHER, 100);
#else
        return argNixPriority(SCHED_RR, 75);
#endif
    }

    thread::arg_t thread::argPlayback() {
        // It would be nice to use realtime priority yet lock ourselves to the slow cores, but that just doesn't work
        // Using priority overrides the request for the slow cores (QOS_CLASS_BACKGROUND)
        // ret.appleThreadQOS = (int) QOS_CLASS_BACKGROUND;

#ifdef _WIN32
        return argWinPriority(THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(__ANDROID__)
        // No SCHED_FIFO or SCHED_RR on Android
        return argNixPriority(SCHED_OTHER, 100);
#else
        return argNixPriority(SCHED_FIFO, 100);
#endif

    }

    thread::arg_t thread::argUserInitiated() {
#ifdef _WIN32
        return argWinPriority(THREAD_PRIORITY_BELOW_NORMAL);
#elif defined(__APPLE__)
        return argAppleQOS((int) QOS_CLASS_USER_INITIATED);
#else
        return argNixPriority(SCHED_OTHER, 25);
#endif
    }
    
#ifdef _WIN32
    thread::arg_t thread::argWinPriority(int priority) {
        arg_t arg;
        arg.winThreadPriority = priority;
        return arg;
    }
#endif
#ifdef __APPLE__
    thread::arg_t thread::argAppleQOS(int qos) {
        arg_t arg;
        arg.appleThreadQOS = qos;
        return arg;
    }
#endif
#ifndef _WIN32
    thread::arg_t thread::argNixPriority(int policy, int percent) {
        int pval;
        if ( percent <= 0 ) pval = sched_get_priority_min(policy);
        else if ( percent >= 100 ) pval = sched_get_priority_max(policy);
        else {
            const int pmin = sched_get_priority_min(policy), pmax = sched_get_priority_max(policy);
            if (pmax > pmin) {
                pval = pmin + ((pmax - pmin) * percent / 100);
            } else {
                pval = pmin;
            }
        }
        arg_t ret;
        ret.nixSchedPolicy = policy;
        ret.nixSchedParam = sched_param { pval };
        return ret;
    }
#endif
    void thread::setCurrentPriority(arg_t const& arg) {
#ifdef _WIN32
        ::SetThreadPriority(GetCurrentThread(), arg.winThreadPriority);
#else
#ifdef __APPLE__
        if (arg.appleThreadQOS != 0) {
            pthread_set_qos_class_self_np((qos_class_t)arg.appleThreadQOS, arg.appleRealtivePriority);
        }

#endif
        if (arg.nixSchedPolicy >= 0) {
            pthread_setschedparam(pthread_self(), arg.nixSchedPolicy, &arg.nixSchedParam);
        }
#endif
    }

}
