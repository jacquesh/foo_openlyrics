#pragma once

class CPPUIResources {
public:
	static UINT get_IDI_SCROLL()
#ifdef IDI_SCROLL
	{ return IDI_SCROLL; }
#endif
	;
};