#pragma once
#include <pfc/wait_queue.h>
#include <pfc/pool.h>
#include <pfc/threads.h>
#include <functional>
#include "rethrow.h"
#include <pfc/timers.h>

namespace ThreadUtils {

    typedef std::function<bool(pfc::eventHandle_t, double) > waitFunc_t;

	// Serialize access to some resource to a single thread
	// Execute blocking/nonabortable methods in with proper abortability (detach on abort and move on)
	class cmdThread {
	public:
		typedef std::function<void () > func_t;
		typedef pfc::waitQueue<func_t> queue_t;
		typedef std::function<void (abort_callback&) > funcAbortable_t;
        
    protected:
        std::function<void () > makeWorker(waitFunc_t wf = pfc::event::g_wait_for) {
            auto q = m_queue;
            auto x = m_atExit;
            return [q, x, wf] {
                for ( ;; ) {
                    func_t f;
                    wf(q->get_event_handle(), -1);
                    if (!q->get(f)) break;
                    try { f(); } catch(...) {}
                }
                // No guard for atExit access, as nobody is supposed to be still able to call host object methods by the time we get here
                for( auto i = x->begin(); i != x->end(); ++ i ) {
                    auto f = *i;
                    try { f(); } catch(...) {}
                }
            };
        };
        std::function<void () > makeWorker2( std::function<void()> updater, double interval, waitFunc_t wf = pfc::event::g_wait_for) {
            auto q = m_queue;
            auto x = m_atExit;
            return [=] {
                pfc::lores_timer t; t.start();
                for ( ;; ) {
                    {
                        bool bWorkReady = false;
                        double left = interval - t.query();
                        if ( left > 0 ) {
                            if (wf(q->get_event_handle(), left)) bWorkReady = true;
                        }
                        
                        if (!bWorkReady) {
                            updater();
                            t.start();
                            continue;
                        }
                    }
                    
                    func_t f;
                    wf(q->get_event_handle(), -1);
                    if (!q->get(f)) break;
                    try { f(); } catch(...) {}
                }
                // No guard for atExit access, as nobody is supposed to be still able to call host object methods by the time we get here
                for( auto i = x->begin(); i != x->end(); ++ i ) {
                    auto f = *i;
                    try { f(); } catch(...) {}
                }
            };
        };
        
        // For derived classes: create new instance without starting thread, supply thread using by yourself
        class noCreate {};
        cmdThread( noCreate ) {}
    public:

		cmdThread() {
			pfc::splitThread( makeWorker() );
		}
        
		void atExit( func_t f ) {
			m_atExit->push_back(f);
		}
		~cmdThread() {
			m_queue->set_eof();
		}
        void runSynchronously( func_t f ) { runSynchronously_(f, nullptr); }
        void runSynchronously_( func_t f, abort_callback * abortOrNull ) {
            auto evt = m_eventPool.make();
            evt->set_state(false);
            auto rethrow = std::make_shared<ThreadUtils::CRethrow>();
            auto worker2 = [f, rethrow, evt] {
                rethrow->exec(f);
                evt->set_state( true );
            };

            add ( worker2 );
            
            if ( abortOrNull != nullptr ) {
                abortOrNull->waitForEvent( * evt, -1 );
            } else {
                evt->wait_for(-1);
            }

            m_eventPool.put( evt );

            rethrow->rethrow();
        }
		void runSynchronously( func_t f, abort_callback & abort ) {
            runSynchronously_(f, &abort);
		}
		void runSynchronously2( funcAbortable_t f, abort_callback & abort ) {
			auto subAbort = m_abortPool.make();
			subAbort->reset();
			auto worker = [subAbort, f] {
				f(*subAbort);
			};

			try {
				runSynchronously( worker, abort );
			} catch(...) {
				subAbort->set(); throw;
			}

			m_abortPool.put( subAbort );
		}

		void add( func_t f ) { m_queue->put( f ); }
	private:
		pfc::objPool<pfc::event> m_eventPool;
		pfc::objPool<abort_callback_impl> m_abortPool;
		std::shared_ptr<queue_t> m_queue = std::make_shared<queue_t>();
		typedef std::list<func_t> atExit_t;
		std::shared_ptr<atExit_t> m_atExit = std::make_shared< atExit_t >();
	};
}
