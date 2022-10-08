#pragma once

/*
foobar2000 shared.dll hook implementations
If you're getting linker multiple-definition errors on these, change build configuration of PFC from "Debug" / "Release" to "Debug FB2K" / "Release FB2K"
Configurations with "FB2K" suffix disable compilation of pfc-fb2k-hooks.cpp allowing these methods to be redirected to shared.dll calls
*/

namespace pfc {
	[[noreturn]] void crashImpl();
	[[noreturn]] void crashHook() {
		crashImpl();
	}
#ifdef _WIN32
	BOOL winFormatSystemErrorMessageImpl(pfc::string_base & p_out, DWORD p_code);
	BOOL winFormatSystemErrorMessageHook(pfc::string_base & p_out, DWORD p_code) {
		return winFormatSystemErrorMessageImpl(p_out, p_code);
	}
#endif
}
