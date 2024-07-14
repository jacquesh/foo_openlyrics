#pragma once
#include <functional>
#include <list>

#include <SDK/threadPool.h>

namespace fb2k {
    class workerTool {
    public:
        struct work_t {
            std::function<void()> work, done;
        };

        void operator+=(work_t && work) {
            m_workQueue.push_back( std::make_shared< work_t> ( std::move(work) ) );
            kickWork();
        }
        void flush() {
            if ( m_working ) {
                m_abort->set();
                m_abort = std::make_shared<abort_callback_impl>();
                m_working = false;
            }
            m_workQueue.clear();
        }

        void workDone() {
            m_working = false;
            kickWork();
        }
        void kickWork() {
            PFC_ASSERT( core_api::is_main_thread() );
            if (!m_working && !m_workQueue.empty()) {
                m_working = true;
                auto iter = m_workQueue.begin();
                auto work = std::move(*iter); m_workQueue.erase(iter);
                auto pThis = this;
                auto a = m_abort;
                fb2k::inCpuWorkerThread( [ work, pThis, a] {
                    try {
                        // release lambdas early, synchronously from same context as they're executed
                        { auto f = std::move(work->work); if (f) f(); }
                        a->check();
                        fb2k::inMainThread( [work, pThis, a] {
                            if ( ! a->is_set() ) {
                                // release lambdas early, synchronously from same context as they're executed
                                { auto f = std::move(work->done); if (f) f(); }
                                pThis->workDone();
                            }
                        });
                    } catch(exception_aborted) {}
                } );
            }
        }
        
        workerTool() {}
        ~workerTool() { m_abort->set(); }
        workerTool(const workerTool&) = delete;
        void operator=(const workerTool&) = delete;
        
        std::shared_ptr<abort_callback_impl> aborter() const { return m_abort; }
    private:
        std::shared_ptr<abort_callback_impl> m_abort = std::make_shared<abort_callback_impl>();
        
        bool m_working = false;
        std::list< std::shared_ptr<work_t> > m_workQueue;
    };

}
