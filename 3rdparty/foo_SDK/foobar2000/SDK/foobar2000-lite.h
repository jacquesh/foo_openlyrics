#pragma once

#include "foobar2000-winver.h"

#include "foobar2000-versions.h"

#include "foobar2000-pfc.h"
#include "../shared/shared.h"

#ifndef NOTHROW
#ifdef _MSC_VER
#define NOTHROW __declspec(nothrow)
#else
#define NOTHROW
#endif
#endif

#define FB2KAPI /*NOTHROW*/

typedef const char* pcchar;

#include "core_api.h"
#include "service.h"
#include "service_impl.h"
#include "service_by_guid.h"
#include "service_compat.h"

#include "forward_types.h"

#include "abort_callback.h"
#include "threadsLite.h"

#include "playable_location.h"
#include "console.h"
#include "filesystem.h"
#include "metadb.h"
