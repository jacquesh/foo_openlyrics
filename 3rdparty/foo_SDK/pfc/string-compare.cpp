#include "pfc-lite.h"

#include "string-compare.h"
#include "string_base.h"
#include "debug.h"

namespace pfc {
    int stricmp_ascii_partial(const char* str, const char* substr) throw() {
        size_t walk = 0;
        for (;;) {
            char c1 = str[walk];
            char c2 = substr[walk];
            c1 = ascii_tolower(c1); c2 = ascii_tolower(c2);
            if (c2 == 0) return 0; // substr terminated = ret0 regardless of str content
            if (c1 < c2) return -1; // ret -1 early
            else if (c1 > c2) return 1; // ret 1 early
            // else c1 == c2 and c2 != 0 so c1 != 0 either
            ++walk; // go on
        }
    }

    int stricmp_ascii_ex(const char* const s1, t_size const len1, const char* const s2, t_size const len2) throw() {
        t_size walk1 = 0, walk2 = 0;
        for (;;) {
            char c1 = (walk1 < len1) ? s1[walk1] : 0;
            char c2 = (walk2 < len2) ? s2[walk2] : 0;
            c1 = ascii_tolower(c1); c2 = ascii_tolower(c2);
            if (c1 < c2) return -1;
            else if (c1 > c2) return 1;
            else if (c1 == 0) return 0;
            walk1++;
            walk2++;
        }

    }

    int wstricmp_ascii(const wchar_t* s1, const wchar_t* s2) throw() {
        for (;;) {
            wchar_t c1 = *s1, c2 = *s2;

            if (c1 > 0 && c2 > 0 && c1 < 128 && c2 < 128) {
                c1 = ascii_tolower_lookup((char)c1);
                c2 = ascii_tolower_lookup((char)c2);
            } else {
                if (c1 == 0 && c2 == 0) return 0;
            }
            if (c1 < c2) return -1;
            else if (c1 > c2) return 1;
            else if (c1 == 0) return 0;

            s1++;
            s2++;
        }
    }

    int stricmp_ascii(const char* s1, const char* s2) throw() {
        for (;;) {
            char c1 = *s1, c2 = *s2;

            if (c1 > 0 && c2 > 0) {
                c1 = ascii_tolower_lookup(c1);
                c2 = ascii_tolower_lookup(c2);
            } else {
                if (c1 == 0 && c2 == 0) return 0;
            }
            if (c1 < c2) return -1;
            else if (c1 > c2) return 1;
            else if (c1 == 0) return 0;

            s1++;
            s2++;
        }
    }

    static int naturalSortCompareInternal(const char* s1, const char* s2, bool insensitive) throw() {
        for (;; ) {
            unsigned c1, c2;
            size_t d1 = utf8_decode_char(s1, c1);
            size_t d2 = utf8_decode_char(s2, c2);
            if (d1 == 0 && d2 == 0) {
                return 0;
            }
            if (char_is_numeric(c1) && char_is_numeric(c2)) {
                // Numeric block in both strings, do natural sort magic here
                size_t l1 = 1, l2 = 1;
                while (char_is_numeric(s1[l1])) ++l1;
                while (char_is_numeric(s2[l2])) ++l2;

                size_t l = max_t(l1, l2);
                for (size_t w = 0; w < l; ++w) {
                    char digit1, digit2;

                    t_ssize off;

                    off = w + l1 - l;
                    if (off >= 0) {
                        digit1 = s1[w - l + l1];
                    } else {
                        digit1 = 0;
                    }
                    off = w + l2 - l;
                    if (off >= 0) {
                        digit2 = s2[w - l + l2];
                    } else {
                        digit2 = 0;
                    }
                    if (digit1 < digit2) return -1;
                    if (digit1 > digit2) return 1;
                }

                s1 += l1; s2 += l2;
                continue;
            }


            if (insensitive) {
                c1 = charLower(c1);
                c2 = charLower(c2);
            }
            if (c1 < c2) return -1;
            if (c1 > c2) return 1;

            s1 += d1; s2 += d2;
        }
    }
    int naturalSortCompare(const char* s1, const char* s2) throw() {
        int v = naturalSortCompareInternal(s1, s2, true);
        if (v) return v;
        v = naturalSortCompareInternal(s1, s2, false);
        if (v) return v;
        return strcmp(s1, s2);
    }

    int naturalSortCompareI(const char* s1, const char* s2) throw() {
        return naturalSortCompareInternal(s1, s2, true);
    }

    const char* _stringComparatorCommon::myStringToPtr(string_part_ref) {
        pfc::crash(); return nullptr;
    }

    int stringCompareCaseInsensitiveEx(string_part_ref s1, string_part_ref s2) {
        t_size w1 = 0, w2 = 0;
        for (;;) {
            unsigned c1, c2; t_size d1, d2;
            d1 = utf8_decode_char(s1.m_ptr + w1, c1, s1.m_len - w1);
            d2 = utf8_decode_char(s2.m_ptr + w2, c2, s2.m_len - w2);
            if (d1 == 0 && d2 == 0) return 0;
            else if (d1 == 0) return -1;
            else if (d2 == 0) return 1;
            else {
                c1 = charLower(c1); c2 = charLower(c2);
                if (c1 < c2) return -1;
                else if (c1 > c2) return 1;
            }
            w1 += d1; w2 += d2;
        }
    }
    int stringCompareCaseInsensitive(const char* s1, const char* s2) {
        for (;;) {
            unsigned c1, c2; t_size d1, d2;
            d1 = utf8_decode_char(s1, c1);
            d2 = utf8_decode_char(s2, c2);
            if (d1 == 0 && d2 == 0) return 0;
            else if (d1 == 0) return -1;
            else if (d2 == 0) return 1;
            else {
                c1 = charLower(c1); c2 = charLower(c2);
                if (c1 < c2) return -1;
                else if (c1 > c2) return 1;
            }
            s1 += d1; s2 += d2;
        }
    }
}

