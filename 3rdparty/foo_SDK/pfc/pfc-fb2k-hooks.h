#pragma once

namespace pfc {
	void crashImpl();
	BOOL winFormatSystemErrorMessageImpl(pfc::string_base & p_out, DWORD p_code);

	void crashHook();
	BOOL winFormatSystemErrorMessageHook(pfc::string_base & p_out, DWORD p_code);
}
