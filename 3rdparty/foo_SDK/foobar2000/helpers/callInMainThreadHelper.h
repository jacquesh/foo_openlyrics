#pragma once
#include <SDK/main_thread_callback.h>

// ======================================================================================================
// Obsolete helpers - they still work, but it's easier to just use fb2k::inMainThread().
// ======================================================================================================


// Proxy class - friend this to allow callInMainThread to access your private methods
class callInMainThread {
public:
    template<typename host_t, typename param_t>
    static void callThis(host_t * host, param_t & param) {
        host->inMainThread(param);
    }
    template<typename host_t>
    static void callThis( host_t * host ) {
        host->inMainThread();
    }
};

// Internal class, do not use.
template<typename service_t, typename param_t> 
class _callInMainThreadSvc_t : public main_thread_callback {
public:
    _callInMainThreadSvc_t(service_t * host, param_t const & param) : m_host(host), m_param(param) {}
    void callback_run() {
        callInMainThread::callThis(m_host.get_ptr(), m_param);
    }
private:
    service_ptr_t<service_t> m_host;
    param_t m_param;
};


// Main thread callback helper. You can use this to easily invoke inMainThread(someparam) on your class without writing any wrapper code.
// Requires myservice_t to be a fb2k service class with reference counting.
template<typename myservice_t, typename param_t> 
static void callInMainThreadSvc(myservice_t * host, param_t const & param) {
    typedef _callInMainThreadSvc_t<myservice_t, param_t> impl_t;
    service_ptr_t<impl_t> obj = new service_impl_t<impl_t>(host, param);
    main_thread_callback_manager::get()->add_callback( obj );
}

//! Helper class to call methods of your class (host class) in main thread with convenience. \n
//! Deals with the otherwise ugly scenario of your class becoming invalid while a method is queued. \n
//! Have this as a member of your class, then use m_mthelper.add( this, somearg ) ; to defer a call to this->inMainThread(somearg). \n
//! If your class becomes invalid before inMainThread is executed, the pending callback is discarded. \n
//! You can optionally call shutdown() to invalidate all pending callbacks early (in a destructor of your class - without waiting for callInMainThreadHelper destructor to do the job. \n
//! In order to let callInMainThreadHelper access your private methods, declare friend class callInMainThread. \n
//! Note that callInMainThreadHelper is expected to be created and destroyed in main thread.
class callInMainThreadHelper {
public:

    typedef std::function< void () > func_t;

    typedef pfc::rcptr_t< bool > killswitch_t;

    class entryFunc : public main_thread_callback {
    public:
        entryFunc( func_t const & func, killswitch_t ks ) : m_ks(ks), m_func(func) {}

        void callback_run() {
            if (!*m_ks) m_func();
        }

    private:
        killswitch_t m_ks;
        func_t m_func;
    };

    template<typename host_t, typename arg_t>
    class entry : public main_thread_callback {
    public:
        entry( host_t * host, arg_t const & arg, killswitch_t ks ) : m_ks(ks), m_host(host), m_arg(arg) {}
        void callback_run() {
            if (!*m_ks) callInMainThread::callThis( m_host, m_arg );
        }
    private:
        killswitch_t m_ks;
        host_t * m_host;
        arg_t m_arg;
    };
    template<typename host_t>
    class entryVoid : public main_thread_callback {
    public:
        entryVoid( host_t * host, killswitch_t ks ) : m_ks(ks), m_host(host) {}
        void callback_run() {
            if (!*m_ks) callInMainThread::callThis( m_host );
        }
    private:
        killswitch_t m_ks;
        host_t * m_host;
    };

    void add(func_t f) {
        add_( new service_impl_t< entryFunc > ( f, m_ks ) );
    }

    template<typename host_t, typename arg_t>
    void add( host_t * host, arg_t const & arg) {
        add_( new service_impl_t< entry<host_t, arg_t> >( host, arg, m_ks ) );
    }
    template<typename host_t>
    void add( host_t * host ) {
        add_( new service_impl_t< entryVoid<host_t> >( host, m_ks ) );
    }
    void add_( main_thread_callback::ptr cb ) {
        main_thread_callback_add( cb );
    }

    callInMainThreadHelper() {
        m_ks.new_t();
        * m_ks = false;
    }
    void shutdown() {
        PFC_ASSERT( core_api::is_main_thread() );
        * m_ks = true;
    }
    ~callInMainThreadHelper() {
        shutdown();
    }

private:
    killswitch_t m_ks;

};
