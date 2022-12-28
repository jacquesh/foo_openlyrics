#pragma once
#include <functional>
#include "completion_notify.h"

namespace fb2k {
	//! \since 2.0
	class NOVTABLE timerManager : public service_base {
		FB2K_MAKE_SERVICE_COREAPI(timerManager);
	public:
		virtual objRef addTimer( double interval, completion_notify::ptr callback ) = 0;
	};
	
	objRef registerTimer( double interval, std::function<void ()> func );
	void callLater( double timeAfter, std::function< void () > func );
}
