#pragma once

#include <memory>

namespace pfc {
	struct killSwitchData_t {
		killSwitchData_t() : m_val() {}
		volatile bool m_val;
	};

	
	typedef std::shared_ptr<killSwitchData_t> killSwitchRef_t;
	
	class killSwitchRef {
	public:
		killSwitchRef( killSwitchRef_t ks ) : m_ks(ks) {}

		operator bool () const {
			return m_ks->m_val;
		}
		
		void set(bool val = true) {
			m_ks->m_val = val;
		}
	private:
		killSwitchRef_t m_ks;
	};
	
	killSwitchRef killSwitchMake() {
		return std::make_shared<killSwitchData_t>();
	}
}
