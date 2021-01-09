#pragma once

namespace fb2k {
	
	// foosort
	// abortable multithreaded quicksort
	// expects cb to handle concurrent calls as long as they do not touch the same items concurrently
	void sort(pfc::sort_callback & cb, size_t count, size_t concurrency, abort_callback & aborter);

}
