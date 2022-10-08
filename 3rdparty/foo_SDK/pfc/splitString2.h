#pragma once
#include <list>
#include <vector>
namespace pfc {
	std::list<pfc::string8> splitString2(const char* str, const char* delim);
	std::list<pfc::string8> splitStringByLines2(const char* str);
	std::vector<pfc::string8> splitString2v(const char* str, const char* delim);
	std::vector<pfc::string8> splitStringByLines2v(const char* str);
}