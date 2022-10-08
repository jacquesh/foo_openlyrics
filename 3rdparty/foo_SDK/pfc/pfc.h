#ifndef ___PFC_H___
#define ___PFC_H___

#include "pfc-lite.h"

#include "int_types.h"
#include "mem_block.h"
#include "traits.h"
#include "bit_array.h"
#include "primitives.h"
#include "alloc.h"
#include "array.h"
#include "bit_array_impl.h"
#include "binary_search.h"
#include "bsearch_inline.h"
#include "bsearch.h"
#include "sort.h"
#include "order_helper.h"
#include "list.h"
#include "ptr_list.h"
#include "string-lite.h"
#include "string_base.h"
#include "splitString.h"
#include "string_list.h"
#include "lockless.h"
#include "ref_counter.h"
#include "iterators.h"
#include "avltree.h"
#include "map.h"
#include "bit_array_impl_part2.h"
#include "timers.h"
#include "guid.h"
#include "byte_order.h"
#include "debug.h"
#include "ptrholder.h"
#include "fpu.h"
#include "other.h"
#include "chain_list_v2.h"
#include "rcptr.h"
#include "com_ptr_t.h"
#include "string_conv.h"
#include "pathUtils.h"
#include "threads.h"
#include "base64.h"
#include "primitives_part2.h"
#include "cpuid.h"
#include "memalign.h"

#include "synchro.h"

#include "syncd_storage.h"

#include "platform-objects.h"

#include "event.h"

#include "audio_sample.h"
#include "wildcard.h"
#include "filehandle.h"
#include "bigmem.h"

#include "string_simple.h"

#define PFC_INCLUDED 1

namespace pfc {
    void selftest();
}

#ifndef PFC_SET_THREAD_DESCRIPTION
#define PFC_SET_THREAD_DESCRIPTION(X)
#endif

#endif //___PFC_H___
