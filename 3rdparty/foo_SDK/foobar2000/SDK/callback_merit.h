#pragma once

namespace fb2k {
	// callback_merit_t controls in what order callbacks are executed. \n
	// In specific corner cases, you want your callback executed before other callbacks of the same kind.
	typedef double callback_merit_t;

	// Note REVERSE sort. HIGHER merit called first.
	static constexpr callback_merit_t callback_merit_default = 0;
	static constexpr callback_merit_t callback_merit_indexer = 1000; // indexer: does nothing else than updating internal state, called early before UI updates, in case UI updates might rely on indexed data.
	static constexpr callback_merit_t callback_merit_serializer = 2000; // serializer: does nothing else than saving new state, called early.

	//! Special class that can be optionally implemented by 'static' callbacks, such as library_callback, to control callback merit. \n
	//! See also: callback_merit_t \n
	//! Some callback classes support get_callback_merit() natively, such as metadb_io_callback_v2. \n
	//! With callbacks registered dynamically, other means of controlling merit are provided.
	class callback_with_merit : public service_base {
		FB2K_MAKE_SERVICE_INTERFACE(callback_with_merit, service_base);
	public:
		virtual callback_merit_t get_callback_merit() = 0;
	};

	callback_merit_t callback_merit_of(service_ptr obj);
}