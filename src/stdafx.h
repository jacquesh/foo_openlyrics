#pragma warning(push, 0)
// NOTE: There are constants for the various Windows versions in winsdkver.h
//       but we can't use that because including Windows headers here (before
//       the ATL/MFC headers) causes compilation errors.
#define WINVER_WIN7 0x0601
#define WINVER_WIN8 0x0602
#define WINVER_WIN10 0x0A000002 // Build 1607

// NOTE: We *technically* support running on Windows 7, but
//       we specify Windows 8 compatibility so that we can pull in the
//       DirectComposition headers (which aren't available in Windows 7).
#define WINVER WINVER_WIN8
#define _WIN32_WINNT WINVER

// Stop the Windows headers from defining all sorts of symbols that we don't want or need
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Fix crashes with mutex initialisation caused by recent updates to the std::mutex constructor.
// See:
// * https://stackoverflow.com/questions/78598141/first-stdmutexlock-crashes-in-application-built-with-latest-visual-studio
// * https://github.com/microsoft/STL/issues/4730
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR 

#include <foobar2000/helpers/foobar2000+atl.h>

#include <cwctype>

#include <algorithm>
#include <chrono>
#include <format>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#pragma warning(pop)