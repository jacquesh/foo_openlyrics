#pragma once

namespace pfc {

    // Unicode normalization.
    // If you have no idea WTF this is about - good for you. The less you know the better for your sanity.

    // _Lite versions are simplified portable versions implemented using hardcoded tables, they're not meant to be complete. They cover only a subset of known two-char sequences.
    // Normal versions (not _Lite) are implemented using operating system methods where possible (MS, Apple), fall back to calling _Lite otherwise.

    // Normalize to form C
    pfc::string8 unicodeNormalizeC(const char * in);
    pfc::string8 unicodeNormalizeC_Lite(const char* in);

    // Normalize to form D
    pfc::string8 unicodeNormalizeD(const char* in);
    pfc::string8 unicodeNormalizeD_Lite(const char* in);

    bool stringContainsFormD(const char* in);
}
