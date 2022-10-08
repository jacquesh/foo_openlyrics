#pragma once

namespace pfc {
	[[noreturn]] void crashImpl();
	[[noreturn]] void crashHook();
#ifdef _WIN32
	BOOL winFormatSystemErrorMessageImpl(pfc::string_base & p_out, DWORD p_code);
	BOOL winFormatSystemErrorMessageHook(pfc::string_base & p_out, DWORD p_code);
#endif
}
