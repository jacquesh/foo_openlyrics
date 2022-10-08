#pragma once

#include "array.h"

namespace pfc {
    class bigmem {
    public:
        enum {slice = 1024*1024};
        bigmem() : m_size() {}
        ~bigmem() {clear();}
        
        void resize(size_t newSize);
        size_t size() const {return m_size;}
        void clear();
        void read(void * ptrOut, size_t bytes, size_t offset);
        void write(const void * ptrIn, size_t bytes, size_t offset);
        uint8_t * _slicePtr(size_t which);
        size_t _sliceCount();
        size_t _sliceSize(size_t which);
    private:
        array_t<uint8_t*> m_data;
        size_t m_size;
        
        PFC_CLASS_NOT_COPYABLE_EX(bigmem)
    };
    
}
