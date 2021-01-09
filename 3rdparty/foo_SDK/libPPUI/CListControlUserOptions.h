#pragma once

class CListControlUserOptions {
public:
	CListControlUserOptions() { instance = this; }
	virtual bool useSmoothScroll() = 0;

	static CListControlUserOptions * instance;
};