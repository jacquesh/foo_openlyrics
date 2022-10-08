#pragma once
#include <memory>
#include "ptrholder.h"

namespace pfc {

	// autoref<> : turn arbitrary ptr that needs to be delete'd into a shared_ptr<> alike
	template<typename obj_t> class autoref {
	public:
		autoref() {}
		autoref(std::nullptr_t) {}
		autoref(obj_t * source) {
			attach(source);
		}
		void attach(obj_t * source) {
			PFC_ASSERT( source != nullptr );
			m_obj = std::make_shared<holder_t>(source);
		}
		void reset() {
			m_obj.reset();
		}

		obj_t * operator->() const {
			return m_obj->get_ptr();
		}

		obj_t * get() const {
			if (! m_obj ) return nullptr;
			return m_obj->get_ptr();
		}

		operator bool() const {
			return !!m_obj;
		}
	private:
		typedef pfc::ptrholder_t< obj_t > holder_t;
		std::shared_ptr< holder_t > m_obj;
	};
}
