#include "foobar2000-sdk-pch.h"
#include "app_close_blocker.h"

bool app_close_blocker::g_query()
{
	for (auto ptr : enumerate()) {
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

void fb2k::splitTask( pfc::thread::arg_t const & arg, std::function<void ()> f) {
    auto taskref = async_task_manager::g_acquire();
    pfc::splitThread( arg, [f,taskref] {
        f();
        (void)taskref; // retain until here
        } );
}

abort_callback& fb2k::mainAborter() {
	return async_task_manager::get()->get_aborter();
}

app_close_blocking_task_impl::app_close_blocking_task_impl(const char * name) : m_name(name) { 
	PFC_ASSERT( core_api::is_main_thread() );
	app_close_blocking_task_manager::get()->register_task(this);
}

app_close_blocking_task_impl::~app_close_blocking_task_impl() { 
	PFC_ASSERT( core_api::is_main_thread() );
	app_close_blocking_task_manager::get()->unregister_task(this);
}

void app_close_blocking_task_impl::query_task_name(pfc::string_base & out) {
	out = m_name; 
}

void app_close_blocking_task_impl_dynamic::toggle_blocking(bool state) {
	PFC_ASSERT( core_api::is_main_thread() );
	if (state != m_taskActive) {
		auto api = app_close_blocking_task_manager::get();
		if (state) api->register_task(this);
		else api->unregister_task(this);
		m_taskActive = state;
	}
}

void app_close_blocking_task_impl_dynamic::query_task_name(pfc::string_base& out) {
	out = m_name;
}
