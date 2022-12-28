#include "foobar2000-sdk-pch.h"
#include "link_resolver.h"

bool link_resolver::g_find(service_ptr_t<link_resolver> & p_out,const char * p_path)
{
	const auto ext = pfc::string_extension(p_path);
	for (auto ptr : enumerate()) {
		if (ptr->is_our_path(p_path,ext)) {
			p_out = ptr;
			return true;
		}
	}
	return false;
}
