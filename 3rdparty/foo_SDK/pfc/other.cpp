#include "pfc-lite.h"

#ifdef _MSC_VER
#include <intrin.h>
#include <assert.h>
#endif
#ifndef _MSC_VER
#include <signal.h>
#include <stdlib.h>
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#include <math.h>

#include "string_base.h"
#include "other.h"
#include "bit_array_impl.h"
#include "order_helper.h"
#include "debug.h"
#include "byte_order.h"
#include "string_conv.h"
#include "memalign.h"
#include "platform-objects.h"
#include "synchro.h"

#include "pfc-fb2k-hooks.h"


namespace pfc {
	bool permutation_is_valid(t_size const * order, t_size count) {
		bit_array_bittable found(count);
		for(t_size walk = 0; walk < count; ++walk) {
            const size_t v = order[walk];
            if (v >= count) return false;
			if (found[v]) return false;
			found.set(v,true);
		}
		return true;
	}
	void permutation_validate(t_size const * order, t_size count) {
		if (!permutation_is_valid(order,count)) throw exception_invalid_permutation();
	}

	t_size permutation_find_reverse(t_size const * order, t_size count, t_size value) {
		if (value >= count) return SIZE_MAX;
		for(t_size walk = 0; walk < count; ++walk) {
			if (order[walk] == value) return walk;
		}
		return SIZE_MAX;
	}
    
    void create_move_item_permutation( size_t * order, size_t count, size_t from, size_t to ) {
        PFC_ASSERT( from < count );
        PFC_ASSERT( to < count );
        for ( size_t w = 0; w < count; ++w ) {
            size_t i = w;
            if ( w == to ) i = from;
            else if ( w < to && w >= from ) {
                ++i;
            } else if ( w > to && w <= from ) {
                --i;
            }
            order[w] = i;
        }
    }
    
    void create_move_items_permutation(t_size * p_output,t_size p_count,const bit_array & p_selection,int p_delta) {
		t_size * const order = p_output;
		const t_size count = p_count;

		pfc::array_t<bool> selection; selection.set_size(p_count);
		
		for(t_size walk = 0; walk < count; ++walk) {
			order[walk] = walk;
			selection[walk] = p_selection[walk];
		}

		if (p_delta<0)
		{
			for(;p_delta<0;p_delta++)
			{
				t_size idx;
				for(idx=1;idx<count;idx++)
				{
					if (selection[idx] && !selection[idx-1])
					{
						pfc::swap_t(order[idx],order[idx-1]);
						pfc::swap_t(selection[idx],selection[idx-1]);
					}
				}
			}
		}
		else
		{
			for(;p_delta>0;p_delta--)
			{
				t_size idx;
				for(idx=count-2;(int)idx>=0;idx--)
				{
					if (selection[idx] && !selection[idx+1])
					{
						pfc::swap_t(order[idx],order[idx+1]);
						pfc::swap_t(selection[idx],selection[idx+1]);
					}
				}
			}
		}
	}
    bool create_drop_permutation(size_t * out, size_t itemCount, pfc::bit_array const & maskSelected, size_t insertMark ) {
        const t_size count = itemCount;
        if (insertMark > count) insertMark = count;
        {
            t_size selBefore = 0;
            for(t_size walk = 0; walk < insertMark; ++walk) {
                if (maskSelected[walk]) selBefore++;
            }
            insertMark -= selBefore;
        }
        {
            pfc::array_t<t_size> permutation, selected, nonselected;
            
            const t_size selcount = maskSelected.calc_count( true, 0, count );
            selected.set_size(selcount); nonselected.set_size(count - selcount);
            permutation.set_size(count);
            if (insertMark > nonselected.get_size()) insertMark = nonselected.get_size();
            for(t_size walk = 0, swalk = 0, nwalk = 0; walk < count; ++walk) {
                if (maskSelected[walk]) {
                    selected[swalk++] = walk;
                } else {
                    nonselected[nwalk++] = walk;
                }
            }
            for(t_size walk = 0; walk < insertMark; ++walk) {
                permutation[walk] = nonselected[walk];
            }
            for(t_size walk = 0; walk < selected.get_size(); ++walk) {
                permutation[insertMark + walk] = selected[walk];
            }
            for(t_size walk = insertMark; walk < nonselected.get_size(); ++walk) {
                permutation[selected.get_size() + walk] = nonselected[walk];
            }
            for(t_size walk = 0; walk < permutation.get_size(); ++walk) {
                if (permutation[walk] != walk) {
                    memcpy(out, permutation.get_ptr(), count * sizeof(size_t));
                    return true;
                }
            }
        }
        return false;
    }
    
}

void order_helper::g_swap(t_size * data,t_size ptr1,t_size ptr2)
{
	t_size temp = data[ptr1];
	data[ptr1] = data[ptr2];
	data[ptr2] = temp;
}


t_size order_helper::g_find_reverse(const t_size * order,t_size val)
{
	t_size prev = val, next = order[val];
	while(next != val)
	{
		prev = next;
		next = order[next];
	}
	return prev;
}


void order_helper::g_reverse(t_size * order,t_size base,t_size count)
{
	t_size max = count>>1;
	t_size n;
	t_size base2 = base+count-1;
	for(n=0;n<max;n++)
		g_swap(order,base+n,base2-n);
}


[[noreturn]] void pfc::crashImpl() {
#ifdef _WIN32
    for (;;) { __debugbreak(); }
#else
#if defined(__ANDROID__) && PFC_DEBUG
	nixSleep(1);
#endif
    for ( ;; ) {
        *(volatile char*) 0 = 0;
        raise(SIGINT);
    }
#endif
}

[[noreturn]] void pfc::crash() {
	crashHook();
}


void pfc::byteswap_raw(void * p_buffer,const t_size p_bytes) {
	t_uint8 * ptr = (t_uint8*)p_buffer;
	t_size n;
	for(n=0;n<p_bytes>>1;n++) swap_t(ptr[n],ptr[p_bytes-n-1]);
}

static pfc::debugLineReceiver * g_debugLineReceivers = nullptr;

pfc::debugLineReceiver::debugLineReceiver() {
	m_chain = g_debugLineReceivers;
	g_debugLineReceivers = this;
}

void pfc::debugLineReceiver::dispatch( const char * msg ) {
	for( auto w = g_debugLineReceivers; w != nullptr; w = w->m_chain ) {
		w->onLine( msg );
	}
}
namespace pfc {
    void appleDebugLog( const char * str );
}

void pfc::outputDebugLine(const char * msg) {
	debugLineReceiver::dispatch( msg );
#ifdef _WIN32
	OutputDebugString(pfc::stringcvt::string_os_from_utf8(PFC_string_formatter() << msg << "\n") );
#elif defined(__ANDROID__)
	__android_log_write(ANDROID_LOG_INFO, "Debug", msg);
#elif defined(__APPLE__)
    appleDebugLog( msg );
#else
	printf("%s\n", msg);
#endif
}

#if PFC_DEBUG

#ifdef _WIN32
void pfc::myassert_win32(const wchar_t * _Message, const wchar_t *_File, unsigned _Line) {
	if (IsDebuggerPresent()) pfc::crash();
	PFC_DEBUGLOG << "PFC_ASSERT failure: " << _Message;
	PFC_DEBUGLOG << "PFC_ASSERT location: " << _File << " : " << _Line;
	_wassert(_Message,_File,_Line);
}
#else

void pfc::myassert(const char * _Message, const char *_File, unsigned _Line)
{
	PFC_DEBUGLOG << "Assert failure: \"" << _Message << "\" in: " << _File << " line " << _Line;
	crash();
}
#endif

#endif


t_uint64 pfc::pow_int(t_uint64 base, t_uint64 exp) noexcept {
	t_uint64 mul = base;
	t_uint64 val = 1;
	t_uint64 mask = 1;
	while(exp != 0) {
		if (exp & mask) {
			val *= mul;
			exp ^= mask;
		}
		mul = mul * mul;
		mask <<= 1;
	}
	return val;
}

double pfc::exp_int( const double base, const int expS ) noexcept {
    //    return pow(base, (double)v);
    
    bool neg;
    unsigned exp;
    if (expS < 0) {
        neg = true;
        exp = (unsigned) -expS;
    } else {
        neg = false;
        exp = (unsigned) expS;
    }
    double v = 1.0;
    if (exp) {
        double mul = base;
        for(;;) {
            if (exp & 1) v *= mul;
            exp >>= 1;
            if (exp == 0) break;
            mul *= mul;
        }
    }
    if (neg) v = 1.0 / v;
    return v;
}


t_int32 pfc::rint32(double p_val) { return (t_int32)lround(p_val); }
t_int64 pfc::rint64(double p_val) { return (t_int64)llround(p_val); }


// mem_block class
namespace pfc {
	void mem_block::resize(size_t newSize) {
		if (m_size != newSize) {
			if (newSize == 0) {
				free(m_ptr); m_ptr = nullptr;
			} else if (m_size == 0) {
				m_ptr = malloc( newSize );
				if (m_ptr == nullptr) throw std::bad_alloc();
			} else {
                auto newptr = realloc( m_ptr, newSize );
				if (newptr == nullptr) throw std::bad_alloc();
                m_ptr = newptr;
			}

			m_size = newSize;
		}
	}
	void mem_block::clear() noexcept {
		free(m_ptr); m_ptr = nullptr; m_size = 0;
	}
	void mem_block::move( mem_block & other ) noexcept {
		clear();
		m_ptr = other.m_ptr;
		m_size = other.m_size;
		other._clear();
	}
	void mem_block::copy( mem_block const & other ) {
		const size_t size = other.size();
		resize( size );
		if (size > 0) memcpy(ptr(), other.ptr(), size);
	}

    // aligned alloc
    void alignedAlloc( void* & m_ptr, size_t & m_size, size_t s, size_t alignBytes) {
        if (s == m_size) {
            // nothing to do
        } else if (s == 0) {
            alignedFree(m_ptr);
            m_ptr = NULL;
        } else {
            void * ptr;
#ifdef _MSC_VER
            if (m_ptr == NULL) ptr = _aligned_malloc(s, alignBytes);
            else ptr = _aligned_realloc(m_ptr, s, alignBytes);
            if ( ptr == NULL ) throw std::bad_alloc();
#else
#ifdef __ANDROID__
            if ((ptr = memalign( alignBytes, s )) == NULL) throw std::bad_alloc();
#else
            if (posix_memalign( &ptr, alignBytes, s ) < 0) throw std::bad_alloc();
#endif
            if (m_ptr != NULL) {
                memcpy( ptr, m_ptr, min_t<size_t>( m_size, s ) );
                alignedFree( m_ptr );
            }
#endif
            m_ptr = ptr;
        }
        m_size = s;
    }

    void* alignedAlloc( size_t s, size_t alignBytes ) {
        void * ptr;
#ifdef _MSC_VER
        ptr = _aligned_malloc(s, alignBytes);
        throw std::bad_alloc();
#else
#ifdef __ANDROID__
        if ((ptr = memalign( alignBytes, s )) == NULL) throw std::bad_alloc();
#else
        if (posix_memalign( &ptr, alignBytes, s ) < 0) throw std::bad_alloc();
#endif
#endif
        return ptr;
    }
 
    void alignedFree( void * ptr ) {
#ifdef _MSC_VER
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}


#include "once.h"

namespace pfc {
#if PFC_CUSTOM_ONCE_FLAG
    static pfc::once_flag_lite g_onceGuardGuard;
    static mutex * g_onceGuard;
    
    static mutex & onceGuard() {
        call_once(g_onceGuardGuard, [] {
            g_onceGuard = new mutex();
        } );
        return * g_onceGuard;
    }
    
    void call_once( once_flag & flag, std::function<void () > work ) {
        
        if ( flag.done ) return;
        
        mutex & guard = onceGuard();
        for ( ;; ) {
            std::shared_ptr<pfc::event> waitFor;
            {
                insync(guard);
                if ( flag.done ) {
                    PFC_ASSERT( ! flag.inProgress );
                    return;
                }
                if ( flag.inProgress ) {
                    if ( ! flag.waitFor ) flag.waitFor = std::make_shared< event > ();
                    waitFor = flag.waitFor;
                } else {
                    flag.inProgress = true;
                }
            }
            
            if ( waitFor ) {
                waitFor->wait_for( -1 );
                continue;
            }
            
            try {
                work();
            } catch(...) {
                insync( guard );
                PFC_ASSERT( ! flag.done );
                PFC_ASSERT( flag.inProgress );
                flag.inProgress = false;
                if ( flag.waitFor ) {
                    flag.waitFor->set_state( true );
                    flag.waitFor.reset();
                }
                throw;
            }
            
            // succeeded
            insync( guard );
            PFC_ASSERT( ! flag.done );
            PFC_ASSERT( flag.inProgress );
            flag.inProgress = false;
            flag.done = true;
            if ( flag.waitFor ) {
                flag.waitFor->set_state( true );
                flag.waitFor.reset();
            }
            return;
        }
    }
#endif
    void call_once( once_flag_lite & flag, std::function<void ()> work ) {
        for ( ;; ) {
            if ( flag.done ) return;
            if (! threadSafeInt::exchangeHere(flag.guard, 1)) {
                try {
                    work();
                } catch(...) {
                    flag.guard = 0;
                    throw;
                }
                flag.done = true;
                return;
            }
            yield();
        }
    }


#ifdef PFC_SET_THREAD_DESCRIPTION_EXTERNAL
    static  std::function<void (const char*)> g_setCurrentThreadDescription;
	void initSetCurrentThreadDescription( std::function<void (const char*)> f ) { g_setCurrentThreadDescription = f; }
#endif	

    void setCurrentThreadDescription( const char * msg ) {
#ifdef __APPLE__
        appleSetThreadDescription( msg );
#endif

#ifdef PFC_WINDOWS_DESKTOP_APP
        winSetThreadDescription(GetCurrentThread(), pfc::stringcvt::string_wide_from_utf8( msg ) );;
#endif
#ifdef PFC_SET_THREAD_DESCRIPTION_EXTERNAL
        if (g_setCurrentThreadDescription) g_setCurrentThreadDescription(msg);
#endif
    }
}


