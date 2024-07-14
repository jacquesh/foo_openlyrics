#pragma once
#include "string_base.h"
#include "fixed_map.h"

// This converts to ASCII *and* lowercases for simplified search matching
namespace pfc {
    class CharStorage {
    public:
        CharStorage() { }
        template<typename TSource> CharStorage(const TSource& in) { Import(in); }
        template<typename TSource> const CharStorage& operator=(const TSource& in) { Import(in); return *this; }

        const char* ptr() const { return m_data; }

        void Import(t_uint32 c) {
            t_size l = pfc::utf8_encode_char(c, m_data);
            m_data[l] = 0;
        }
        void Import(const char* c) {
            PFC_ASSERT(strlen(c) < PFC_TABSIZE(m_data));
#ifdef _MSC_VER
            strcpy_s(m_data, c);
#else
            strcpy(m_data, c);
#endif
        }

        char m_data[16];
    };

    class CharDownConvert {
    public:
        CharDownConvert();

        void TransformCharCachedAppend(t_uint32 c, pfc::string_base& out);
        void TransformStringAppend(pfc::string_base& out, const char* src, size_t len = SIZE_MAX);
        void TransformStringHere(pfc::string_base& out, const char* src, size_t len = SIZE_MAX);
        string8 TransformString(const char* src) { pfc::string8 ret; TransformStringAppend(ret, src); return ret; }
        void TransformString(pfc::string_base& out, const char* src) {
            out.reset(); TransformStringAppend(out, src);
        }

        static CharDownConvert& instance();

        struct charMapping_t {
            uint16_t from, to;
        };
        static const charMapping_t* mappings();
        static size_t numMappings();

    private:
        fixed_map<uint32_t, CharStorage> m_charConvertMap;
    };
}
