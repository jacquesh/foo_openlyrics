#pragma once

// NOTE: We only pull in the generated header in release builds because that file gets recreated
//       on every build, which forces every file that includes it to also be recompiled on every
//       build. This is a significant waste of time during development.
#ifdef NDEBUG
#include "openlyrics_version_generated.h" // Defines OPENLYRICS_VERSION
#else
#define OPENLYRICS_VERSION "0.0-alpha"
#endif
