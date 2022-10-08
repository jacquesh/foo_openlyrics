#include "pfc-lite.h"
#include "bigmem.h"

namespace pfc {
	void bigmem::resize(size_t newSize) {
		clear();
		m_data.set_size((newSize + slice - 1) / slice);
		m_data.fill_null();
		for (size_t walk = 0; walk < m_data.get_size(); ++walk) {
			size_t thisSlice = slice;
			if (walk + 1 == m_data.get_size()) {
				size_t cut = newSize % slice;
				if (cut) thisSlice = cut;
			}
			void* ptr = malloc(thisSlice);
			if (ptr == NULL) { clear(); throw std::bad_alloc(); }
			m_data[walk] = (uint8_t*)ptr;
		}
		m_size = newSize;
	}
	void bigmem::clear() {
		for (size_t walk = 0; walk < m_data.get_size(); ++walk) free(m_data[walk]);
		m_data.set_size(0);
		m_size = 0;
	}
	void bigmem::read(void* ptrOut, size_t bytes, size_t offset) {
		PFC_ASSERT(offset + bytes <= size());
		uint8_t* outWalk = (uint8_t*)ptrOut;
		while (bytes > 0) {
			size_t o1 = offset / slice, o2 = offset % slice;
			size_t delta = slice - o2; if (delta > bytes) delta = bytes;
			memcpy(outWalk, m_data[o1] + o2, delta);
			offset += delta;
			bytes -= delta;
			outWalk += delta;
		}
	}
	void bigmem::write(const void* ptrIn, size_t bytes, size_t offset) {
		PFC_ASSERT(offset + bytes <= size());
		const uint8_t* inWalk = (const uint8_t*)ptrIn;
		while (bytes > 0) {
			size_t o1 = offset / slice, o2 = offset % slice;
			size_t delta = slice - o2; if (delta > bytes) delta = bytes;
			memcpy(m_data[o1] + o2, inWalk, delta);
			offset += delta;
			bytes -= delta;
			inWalk += delta;
		}
	}
	uint8_t* bigmem::_slicePtr(size_t which) { return m_data[which]; }
	size_t bigmem::_sliceCount() { return m_data.get_size(); }
	size_t bigmem::_sliceSize(size_t which) {
		if (which + 1 == _sliceCount()) {
			size_t s = m_size % slice;
			if (s) return s;
		}
		return slice;
	}
}