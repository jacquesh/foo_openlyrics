#include "foobar2000.h"
#include "component.h"

foobar2000_api * g_foobar2000_api = NULL;

service_class_ref service_factory_base::enum_find_class(const GUID & p_guid)
{
	PFC_ASSERT(core_api::are_services_available() && g_foobar2000_api);
	return g_foobar2000_api->service_enum_find_class(p_guid);
}

bool service_factory_base::enum_create(service_ptr_t<service_base> & p_out,service_class_ref p_class,t_size p_index)
{
	PFC_ASSERT(core_api::are_services_available() && g_foobar2000_api);
	return g_foobar2000_api->service_enum_create(p_out,p_class,p_index);
}

t_size service_factory_base::enum_get_count(service_class_ref p_class)
{
	PFC_ASSERT(core_api::are_services_available() && g_foobar2000_api);
	return g_foobar2000_api->service_enum_get_count(p_class);
}

service_factory_base * service_factory_base::__internal__list = NULL;


namespace service_impl_helper {
	void release_object_delayed(service_base * ptr) {
		ptr->service_add_ref();
        fb2k::inMainThread( [ptr] {
            try { ptr->service_release(); } catch(...) {}
        } );
	}
};


void _standard_api_create_internal(service_ptr & out, const GUID & classID) {
	service_class_ref c = service_factory_base::enum_find_class(classID);
	switch(service_factory_base::enum_get_count(c)) {
		case 0:
#if PFC_DEBUG
            if ( core_api::are_services_available() ) {
                FB2K_DebugLog() << "Service not found of type: " << pfc::print_guid(classID);
            }
#endif
			throw exception_service_not_found();
		case 1:
			PFC_ASSERT_SUCCESS( service_factory_base::enum_create(out, c, 0) );
			break;
		default:
			throw exception_service_duplicated();
	}
}

bool _standard_api_try_get_internal(service_ptr & out, const GUID & classID) {
	service_class_ref c = service_factory_base::enum_find_class(classID);
	switch (service_factory_base::enum_get_count(c)) {
	case 1:
		PFC_ASSERT_SUCCESS(service_factory_base::enum_create(out, c, 0));
		return true;
	default:
		return false;
	}
}

void _standard_api_get_internal(service_ptr & out, const GUID & classID) {
	if (!_standard_api_try_get_internal(out, classID) ) uBugCheck();
}