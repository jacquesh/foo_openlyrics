#include "foobar2000.h"

bool app_close_blocker::g_query()
{
	service_ptr_t<app_close_blocker> ptr;
	service_enum_t<app_close_blocker> e;
	while(e.next(ptr))
	{
		if (!ptr->query()) return false;
	}
	return true;
}

service_ptr async_task_manager::g_acquire() {
#if FOOBAR2000_TARGET_VERSION >= 80
	return get()->acquire();
#else
	ptr obj;
	if ( tryGet(obj) ) return obj->acquire();
	return nullptr;
#endif
}

void fb2k::splitTask( std::function<void ()> f) {
	auto taskref = async_task_manager::g_acquire();
	pfc::splitThread( [f,taskref] {
		f();
		(void)taskref; // retain until here
		} );
}