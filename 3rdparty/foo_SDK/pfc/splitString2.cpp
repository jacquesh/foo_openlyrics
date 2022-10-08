#include "pfc-lite.h"
#include "pfc.h"
#include "splitString.h"
#include "splitString2.h"

namespace {
	class counter_t {
	public:
		size_t count = 0;
		inline void operator+=(pfc::string_part_ref const& p) { ++count; }
	};
	template<typename ret_t>
	class wrapper_t {
	public:
		ret_t * ret;

		inline void operator+=(pfc::string_part_ref const & p) {
			ret->emplace_back(p);
		}
	};
}

namespace pfc {
	
	std::list<pfc::string8> splitString2(const char* str, const char* delim) {
		std::list<pfc::string8> ret;
		wrapper_t<decltype(ret)> w; w.ret = &ret;
		pfc::splitStringBySubstring(w, str, delim);
		return ret;
	}
	std::list<pfc::string8> splitStringByLines2(const char* str) {
		std::list<pfc::string8> ret;
		wrapper_t<decltype(ret)> w; w.ret = &ret;
		pfc::splitStringByLines(w, str);
		return ret;
	}

	std::vector<pfc::string8> splitString2v(const char* str, const char* delim) {
		counter_t counter;
		pfc::splitStringBySubstring(counter, str, delim);
		std::vector<pfc::string8> ret; ret.reserve(counter.count);
		wrapper_t<decltype(ret)> w; w.ret = &ret;
		pfc::splitStringBySubstring(w, str, delim);
		return ret;

	}
	std::vector<pfc::string8> splitStringByLines2v(const char* str) {
		counter_t counter;
		pfc::splitStringByLines(counter, str);
		std::vector<pfc::string8> ret; ret.reserve(counter.count);
		wrapper_t<decltype(ret)> w; w.ret = &ret;
		pfc::splitStringByLines(w, str);
		return ret;
	}

}