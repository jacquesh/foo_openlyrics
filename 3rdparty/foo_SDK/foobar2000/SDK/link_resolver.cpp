#include "foobar2000.h"

bool link_resolver::g_find(service_ptr_t<link_resolver> & p_out,const char * p_path)
{
	service_enum_t<link_resolver> e;
	service_ptr_t<link_resolver> ptr;
	auto ext = pfc::string_extension(p_path);
	while(e.next(ptr))
	{
		if (ptr->is_our_path(p_path,ext))
		{
			p_out = ptr;
			return true;
		}
	}
	return false;
}
