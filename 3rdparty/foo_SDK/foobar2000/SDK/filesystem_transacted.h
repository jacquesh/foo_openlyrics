#pragma once

#include "filesystem.h"

// Since 1.5, transacted filesystem is no longer supported 
// as it adds extra complexity without actually solving any problems.
// Even Microsoft recommends not to use this API.
#define FB2K_SUPPORT_TRANSACTED_FILESYSTEM 0
