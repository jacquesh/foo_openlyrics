#pragma once


namespace fb2k {
	//! fb2k::readWriteLock: abortable readWriteLock, allowing multiple concurrent readers while not writing, or one writer while not reading. \n
	//! Safe to release locks in different threads than obtained, contrary to system object such as SRW locks.
	class readWriteLock {
		pfc::mutex m_guard;
		size_t m_readers = 0, m_writers = 0;
		typedef std::shared_ptr < pfc::event> eventRef_t;
		eventRef_t m_currentEvent;
	public:

		void shutdown() {
			for (;;) {
				eventRef_t waitFor;
				{
					PFC_INSYNC(m_guard);
					if (m_readers == 0 && m_writers == 0) return;
					PFC_ASSERT(m_currentEvent);
					waitFor = m_currentEvent;
				}
				waitFor->wait_for(-1);
			}
		}

		service_ptr beginRead(abort_callback& a) {
			for (;;) {
				a.check();
				eventRef_t waitFor;
				{
					PFC_INSYNC(m_guard);
					if (m_writers == 0) {
						if (!m_currentEvent) m_currentEvent = std::make_shared<pfc::event>();
						++m_readers;
						return fb2k::callOnRelease([this] {
							PFC_INSYNC(m_guard);
							PFC_ASSERT(m_currentEvent);
							PFC_ASSERT(m_readers > 0 && m_writers == 0);
							if (--m_readers == 0) {
								m_currentEvent->set_state(true); m_currentEvent = nullptr;
							}
						});
					}
					PFC_ASSERT(m_currentEvent);
					waitFor = m_currentEvent;
				}
				a.waitForEvent(*waitFor);
			}
		}
		service_ptr beginWrite(abort_callback& a) {
			for (;;) {
				a.check();
				eventRef_t waitFor;
				{
					PFC_INSYNC(m_guard);
					if (m_readers == 0 && m_writers == 0) {
						m_currentEvent = std::make_shared<pfc::event>();
						++m_writers;
						return fb2k::callOnRelease([this] {
							PFC_INSYNC(m_guard);
							PFC_ASSERT(m_currentEvent);
							PFC_ASSERT(m_readers == 0 && m_writers > 0);
							if (--m_writers == 0) {
								m_currentEvent->set_state(true); m_currentEvent = nullptr;
							}
						});
					}
					PFC_ASSERT(m_currentEvent);
					waitFor = m_currentEvent;
				}
				a.waitForEvent(*waitFor);
			}
		}

	};
}
