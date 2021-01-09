#pragma once

// PFC weakRef class
// Create weak references to objects that automatically become invalidated upon destruction of the object
// Note that this is NOT thread safe and meant for single thread usage. If you require thread safety, provide your own means, such as mutexlocking of weakRef::get() and the destruction of your objects.

#include <memory> // std::shared_ptr

namespace pfc {
	typedef std::shared_ptr<const bool> weakRefKillSwitch_t;

	template<typename target_t>
	class weakRef {
		typedef weakRef<target_t> self_t;
	public:
		static self_t _make(target_t * target, weakRefKillSwitch_t ks) {
			self_t ret;
			ret.m_target = target;
			ret.m_ks = ks;
			return ret;
		}


		bool isValid() const {
			return m_ks && !*m_ks;
		}

		target_t * get() const {
			if (!isValid()) return nullptr;
			return m_target;
		}
		target_t * operator() () const {
			return get();
		}
	private:
		target_t * m_target = nullptr;
		weakRefKillSwitch_t m_ks;
	};

	template<typename class_t>
	class weakRefTarget {
	public:
		weakRefTarget() {}

		typedef weakRef<class_t> weakRef_t;

		weakRef<class_t> weakRef() {
			PFC_ASSERT(!*m_ks);
			class_t * ptr = static_cast<class_t*>(this);
			return weakRef_t::_make(ptr, m_ks);
		}

		// Optional: explicitly invalidates all weak references to this object. Called implicitly by the destructor, but you can do it yourself earlier.
		void weakRefShutdown() {
			*m_ks = true;
		}
		~weakRefTarget() {
			weakRefShutdown();
		}

		// Optional: obtain a reference to the killswitch if you wish to use it directly
		weakRefKillSwitch_t weakRefKS() const {
			return m_ks;
		}
	private:
		std::shared_ptr<bool> m_ks = std::make_shared<bool>(false);

		weakRefTarget(const weakRefTarget &) = delete;
		void operator=(const weakRefTarget &) = delete;
	};
}
