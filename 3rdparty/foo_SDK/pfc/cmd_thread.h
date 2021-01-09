#pragma once

#include "wait_queue.h"
#include <functional>
#include <mutex>
#include <memory>

namespace pfc {

	class cmdThread {
	public:
		typedef std::function<void()> cmd_t;
		typedef pfc::waitQueue<cmd_t> queue_t;

		void add(cmd_t c) {
			std::call_once(m_once, [this] {
				auto q = std::make_shared<queue_t>();
				pfc::splitThread([q] {
					cmd_t cmd;
					while (q->get(cmd)) {
						cmd();
					}
				});
				m_queue = q;
			});
			m_queue->put(c);
		}

		void shutdown() {
			if (m_queue) {
				m_queue->set_eof();
				m_queue.reset();
			}
		}
		~cmdThread() {
			shutdown();
		}
	private:

		std::shared_ptr< queue_t > m_queue;


		std::once_flag m_once;
	};

}