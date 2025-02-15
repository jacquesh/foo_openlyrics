#include "stdafx.h"

#define MVTF_IMPLEMENTATION
#include "mvtf/mvtf.h"

#define STRINGSPLIT_IMPLEMENTATION
#if MVTF_TESTS_ENABLED
#define STRINGSPLIT_TESTS_ENABLED
#define STRINGSPLIT_TEST MVTF_TEST
#define STRINGSPLIT_ASSERT ASSERT
#endif
#include "string_split.h"
