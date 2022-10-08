#include "pfc-lite.h"
#include "debug.h"

#ifdef _WIN32
namespace pfc {
    static volatile const char* g_msg;
    [[noreturn]] void crashWithMessageOnStack(const char* msg) {
        // fb2k crash reporter will get it reliably
        g_msg = msg;
        crash();
    }
}
#else

#include "string_base.h"

#ifdef __clang__
#pragma clang optimize off
#endif

[[noreturn]] static void recur( const char * msg );

// Rationale: don't make the letter-functions bit-identical or else the optimizer might merge them!
// Set a static volatile var to a unique value in each function.
static volatile char g;

[[noreturn]] static void __(const char * msg) { g = '_'; recur(msg); }
[[noreturn]] static void _A_( const char * msg ) { g = 'A'; recur(msg); }
[[noreturn]] static void _B_( const char * msg ) { g = 'B'; recur(msg); }
[[noreturn]] static void _C_( const char * msg ) { g = 'C'; recur(msg); }
[[noreturn]] static void _D_( const char * msg ) { g = 'D'; recur(msg); }
[[noreturn]] static void _E_( const char * msg ) { g = 'E'; recur(msg); }
[[noreturn]] static void _F_( const char * msg ) { g = 'F'; recur(msg); }
[[noreturn]] static void _G_( const char * msg ) { g = 'G'; recur(msg); }
[[noreturn]] static void _H_( const char * msg ) { g = 'H'; recur(msg); }
[[noreturn]] static void _I_( const char * msg ) { g = 'I'; recur(msg); }
[[noreturn]] static void _J_( const char * msg ) { g = 'J'; recur(msg); }
[[noreturn]] static void _K_( const char * msg ) { g = 'K'; recur(msg); }
[[noreturn]] static void _L_( const char * msg ) { g = 'L'; recur(msg); }
[[noreturn]] static void _M_( const char * msg ) { g = 'M'; recur(msg); }
[[noreturn]] static void _N_( const char * msg ) { g = 'N'; recur(msg); }
[[noreturn]] static void _O_( const char * msg ) { g = 'O'; recur(msg); }
[[noreturn]] static void _P_( const char * msg ) { g = 'P'; recur(msg); }
[[noreturn]] static void _Q_( const char * msg ) { g = 'Q'; recur(msg); }
[[noreturn]] static void _R_( const char * msg ) { g = 'R'; recur(msg); }
[[noreturn]] static void _S_( const char * msg ) { g = 'S'; recur(msg); }
[[noreturn]] static void _T_( const char * msg ) { g = 'T'; recur(msg); }
[[noreturn]] static void _U_( const char * msg ) { g = 'U'; recur(msg); }
[[noreturn]] static void _V_( const char * msg ) { g = 'V'; recur(msg); }
[[noreturn]] static void _W_( const char * msg ) { g = 'W'; recur(msg); }
[[noreturn]] static void _X_( const char * msg ) { g = 'X'; recur(msg); }
[[noreturn]] static void _Y_( const char * msg ) { g = 'Y'; recur(msg); }
[[noreturn]] static void _Z_( const char * msg ) { g = 'Z'; recur(msg); }
[[noreturn]] static void _0_( const char * msg ) { g = '0'; recur(msg); }
[[noreturn]] static void _1_( const char * msg ) { g = '1'; recur(msg); }
[[noreturn]] static void _2_( const char * msg ) { g = '2'; recur(msg); }
[[noreturn]] static void _3_( const char * msg ) { g = '3'; recur(msg); }
[[noreturn]] static void _4_( const char * msg ) { g = '4'; recur(msg); }
[[noreturn]] static void _5_( const char * msg ) { g = '5'; recur(msg); }
[[noreturn]] static void _6_( const char * msg ) { g = '6'; recur(msg); }
[[noreturn]] static void _7_( const char * msg ) { g = '7'; recur(msg); }
[[noreturn]] static void _8_( const char * msg ) { g = '8'; recur(msg); }
[[noreturn]] static void _9_( const char * msg ) { g = '9'; recur(msg); }


[[noreturn]] static void recur( const char * msg ) {
    char c = *msg++;
    switch(pfc::ascii_toupper(c)) {
        case 0: pfc::crash();
        case 'A': _A_(msg); break;
        case 'B': _B_(msg); break;
        case 'C': _C_(msg); break;
        case 'D': _D_(msg); break;
        case 'E': _E_(msg); break;
        case 'F': _F_(msg); break;
        case 'G': _G_(msg); break;
        case 'H': _H_(msg); break;
        case 'I': _I_(msg); break;
        case 'J': _J_(msg); break;
        case 'K': _K_(msg); break;
        case 'L': _L_(msg); break;
        case 'M': _M_(msg); break;
        case 'N': _N_(msg); break;
        case 'O': _O_(msg); break;
        case 'P': _P_(msg); break;
        case 'Q': _Q_(msg); break;
        case 'R': _R_(msg); break;
        case 'S': _S_(msg); break;
        case 'T': _T_(msg); break;
        case 'U': _U_(msg); break;
        case 'V': _V_(msg); break;
        case 'W': _W_(msg); break;
        case 'X': _X_(msg); break;
        case 'Y': _Y_(msg); break;
        case 'Z': _Z_(msg); break;
        case '0': _0_(msg); break;
        case '1': _1_(msg); break;
        case '2': _2_(msg); break;
        case '3': _3_(msg); break;
        case '4': _4_(msg); break;
        case '5': _5_(msg); break;
        case '6': _6_(msg); break;
        case '7': _7_(msg); break;
        case '8': _8_(msg); break;
        case '9': _9_(msg); break;
        default: __(msg) ;break;
    }
}

namespace pfc {
    [[noreturn]] void crashWithMessageOnStack( const char * msg ) { recur(msg); }
}

#endif