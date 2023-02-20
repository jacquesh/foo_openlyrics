#pragma warning(push, 0)
#include <foobar2000/helpers/foobar2000+atl.h>

#include <cwctype>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#pragma warning(pop)

#ifdef BUILDING_OPENLYRICS_DLL
#define OPENLYRICS_TESTABLE_FUNC __declspec(dllexport)
#else
#define OPENLYRICS_TESTABLE_FUNC __declspec(dllimport)
#endif // BUILDING_OPENLYRICS_DLL