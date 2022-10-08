#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif
namespace pfc {

	class threadSafeInt {
	public:
        typedef long val_t;
		typedef val_t t_val;

		threadSafeInt(t_val p_val = 0) : m_val(p_val) {}
		long operator++() throw() { return inc(); }
		long operator--() throw() { return dec(); }
		long operator++(int) throw() { return inc() - 1; }
		long operator--(int) throw() { return dec() + 1; }
		operator t_val() const throw() { return m_val; }

        static val_t exchangeHere( volatile val_t & here, val_t newVal ) {
#ifdef _MSC_VER
            return InterlockedExchange(&here, newVal);
#else
            return __sync_lock_test_and_set(&here, newVal);
#endif
		}

		t_val exchange(t_val newVal) {
            return exchangeHere( m_val, newVal );
		}
	private:
		t_val inc() {
#ifdef _MSC_VER
			return _InterlockedIncrement(&m_val);
#else
			return __sync_add_and_fetch(&m_val, 1);
#endif
		}
		t_val dec() {
#ifdef _MSC_VER
			return _InterlockedDecrement(&m_val);
#else
			return __sync_sub_and_fetch(&m_val, 1);
#endif
		}

		volatile t_val m_val;
	};

	typedef threadSafeInt counter;
	typedef threadSafeInt refcounter;

	void yield(); // forward declaration

}
