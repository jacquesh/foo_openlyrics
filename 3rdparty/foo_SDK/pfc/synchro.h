#pragma once
// added for compatibility with fb2k mobile

namespace pfc {
	class dummyLock {
	public:
		void enterRead() {}
		void enterWrite() {}
		void leaveRead() {}
		void leaveWrite() {}
		void enter() {}
		void leave() {}
		void lock() {}
		void unlock() {}
	};
}