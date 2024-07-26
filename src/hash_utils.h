#pragma once
#include <stdint.h>

struct Sha256Context
{
    void* m_algorithm_handle;
    void* m_hash_handle;
    void* m_internal_storage;

    Sha256Context();
    ~Sha256Context();

    void add_data(uint8_t* buffer, size_t buffer_len);
    void finalise(uint8_t (&output)[32]);
};

