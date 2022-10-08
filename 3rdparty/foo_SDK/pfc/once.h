#pragma once

#include <functional>
#include "event.h"
#include <memory>
#include "lockless.h"

#ifdef __ANDROID__
#define PFC_CUSTOM_ONCE_FLAG 1
#endif

#ifndef PFC_CUSTOM_ONCE_FLAG
#define PFC_CUSTOM_ONCE_FLAG 0
#endif

#if ! PFC_CUSTOM_ONCE_FLAG
#include <mutex>
#endif

namespace pfc {
#if PFC_CUSTOM_ONCE_FLAG
    struct once_flag {
        bool inProgress = false, done = false;
        std::shared_ptr<::pfc::event> waitFor;
    };
    
    void call_once( once_flag &, std::function<void () > );
#else
    using std::once_flag;
    using std::call_once;
#endif

    //! Minimalist class to call some function only once. \n
    //! Presumes low probability of concurrent run() calls actually happening, \n
    //! but frequent calls once already initialized, hence only using basic volatile bool check. \n
    //! If using a modern compiler you might want to use std::call_once instead. \n
    //! The called function is not expected to throw exceptions.
    struct once_flag_lite {
        threadSafeInt::val_t guard = 0;
        volatile bool done = false;
    };
    
    void call_once( once_flag_lite &, std::function<void () > );

    class runOnceLock {
    public:
        void run(std::function<void()> f) {call_once(m_flag, f);}
    private:
        once_flag_lite m_flag;
    };
}
