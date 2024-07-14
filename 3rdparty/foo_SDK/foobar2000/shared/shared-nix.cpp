#include "shared.h"
#include <dirent.h>
#include <pfc/wildcard.h>

// foobar2000 SDK project method... bah
namespace foobar2000_io {
    PFC_NORETURN void nix_io_op_fail();
}
using namespace foobar2000_io;

namespace {
    class uFindFileImpl : public uFindFile {
    public:
        uFindFileImpl ( DIR * dir ) : m_dir(dir) {}
        bool FindNext() override {
            m_entry = readdir(m_dir);
            return m_entry != NULL;
        }
        const char * GetFileName() override {
            return m_entry->d_name;
        }
        bool IsDirectory() override {
            return m_entry->d_type == DT_DIR;
        }
        ~uFindFileImpl() {
            closedir(m_dir);
        }
    private:
        dirent * m_entry;
        
        DIR * m_dir;
    };

    class uFindFileFiltered : public uFindFile {
    public:
        uFindFileFiltered( DIR * dir, const char * wc ) : m_impl(dir), m_wc(wc) {}
        bool FindNext() override {
            for( ;; ) {
                if (!m_impl.FindNext()) return false;
                if ( testWC() ) return true;
            }
        }
        const char * GetFileName() override {
            return m_impl.GetFileName();
        }
        bool IsDirectory() override {
            return m_impl.IsDirectory();
        }
        bool testWC() {
            return wildcard_helper::test(GetFileName(), m_wc);
        }
        
        uFindFileImpl m_impl;
        const pfc::string8 m_wc;
    };
}
    
puFindFile uFindFirstFile(const char * path) {
    
    if ( wildcard_helper::has_wildcards( path ) ) {
        size_t idx = pfc::scan_filename(path);
        
        try {
            DIR * dir = opendir( pfc::string8(path, idx) );
            if (dir == NULL) nix_io_op_fail();
            try {
                uFindFile * ff;
                try {
                    ff = new uFindFileFiltered(dir, path + idx);
                } catch(...) { closedir( dir ); throw; }
                if (ff->FindNext()) return ff;
                delete ff;
                return NULL;
            } catch(...) { closedir( dir ); throw; }
        } catch(...) {return NULL;}

    }
    
    try {
        DIR * dir = opendir( path );
        if (dir == NULL) nix_io_op_fail();
        try {
            uFindFile * ff;
            try {
                ff = new uFindFileImpl(dir);
            } catch(...) { closedir( dir ); throw; }
            if (ff->FindNext()) return ff;
            delete ff;
            return NULL;
        } catch(...) { closedir( dir ); throw; }
    } catch(...) {return NULL;}
}

pfc::string8 uStringPrintf(const char * fmt, ...) {
    pfc::string8 ret;
    va_list list;
    va_start(list,fmt);
    uPrintfV(ret,fmt,list);
    va_end(list);
    return ret;
}

void uPrintfV(pfc::string_base & out,const char * fmt,va_list arglist) {
    pfc::string_printf_here_va(out, fmt, arglist);
}

void uPrintf(pfc::string_base & out,const char * fmt,...) {
    va_list list;va_start(list,fmt);uPrintfV(out,fmt,list);va_end(list);
}


static int makeInfiniteWaitEvent() {
    int fdPipe[2];
    pfc::createPipe(fdPipe);
    return fdPipe[0]; // leak the other end
}

int GetInfiniteWaitEvent() {
    static int obj = makeInfiniteWaitEvent();
    return obj;
}


// DUMMY
void SHARED_EXPORT uAddPanicHandler(fb2k::panicHandler*) {
    
}
void SHARED_EXPORT uRemovePanicHandler(fb2k::panicHandler*) {
    
}

void SHARED_EXPORT uOutputDebugString(const char * msg) {
    // UGLY: underlying APIs want whole lines, calling code feeds lines terminated with \n or \r\n because Windows
    pfc::string8 temp ( msg );
    if ( temp.endsWith('\n') ) temp.truncate( temp.length() - 1) ;
    if ( temp.endsWith('\r') ) temp.truncate( temp.length() - 1) ;
    pfc::outputDebugLine(temp);
}
namespace pfc { PFC_NORETURN void crashImpl(); }
PFC_NORETURN void SHARED_EXPORT uBugCheck() {
    pfc::crashImpl();
}

int SHARED_EXPORT uStringCompare(const char * elem1, const char * elem2) {
    return pfc::naturalSortCompareI(elem1, elem2);
}

void fb2kDebugSelfTest() {
    
}

bool uGetTempPath(pfc::string_base & out) {
    auto var = getenv("TMPDIR");
    if ( var == nullptr ) uBugCheck();
    out = var;
    return true;
}
bool uGetTempFileName(const char * path_name,const char * prefix,unsigned unique,pfc::string_base & out) {
#if 0 // sample use
    pfc::string8 temp_path, temp_file;
    uGetTempPath(temp_path);
    uGetTempFileName(temp_path, "img", 0, temp_file);
#endif
    pfc::string8 ret;
    if ( path_name == nullptr ) uGetTempPath( ret );
    else ret = path_name;
    
    pfc::chain_list_v2_t<pfc::string8> segments;
    if ( prefix ) segments += prefix;
    if ( unique ) segments += pfc::format(unique);
    segments += pfc::print_guid(pfc::createGUID());

    pfc::string8 fn;
    for( auto & seg : segments ) {
        if (seg.length() == 0) continue;
        if ( fn.length() > 0 ) fn += "-";
        fn += seg;
    }
    
    ret.add_filename( fn );
    out = ret;
    return true;
}

pfc::string8 uGetTempFileName() {
    pfc::string8 ret;
    uGetTempFileName(nullptr, nullptr, 0, ret);
    return ret;
}


void fb2k::crashWithMessage [[noreturn]] ( const char * msg_ ) {
    // there used to be code throwing Objective-C exceptions, but those were of no use,
    pfc::crashWithMessageOnStack(msg_);
}
