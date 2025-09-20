#include "stdafx.h"

#include <bcrypt.h>
#include <windows.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "hash_utils.h"
#include "mvtf/mvtf.h"

Sha256Context::Sha256Context()
    : m_algorithm_handle(nullptr)
    , m_hash_handle(nullptr)
    , m_internal_storage(nullptr)
    , m_error(false)
{
    m_error = m_error
              || FAILED(BCryptOpenAlgorithmProvider((BCRYPT_ALG_HANDLE*)&m_algorithm_handle,
                                                    BCRYPT_SHA256_ALGORITHM,
                                                    nullptr,
                                                    BCRYPT_HASH_REUSABLE_FLAG));

    // BCryptGetProperty wants to output the number of bytes written to the output buffer.
    // This parameter is meant to be optional but we get failures without it.
    ULONG get_property_bytes_written = 0;
    DWORD hash_obj_len = 0;
    m_error = m_error
              || FAILED(BCryptGetProperty((BCRYPT_ALG_HANDLE)m_algorithm_handle,
                                          BCRYPT_OBJECT_LENGTH,
                                          (PBYTE)&hash_obj_len,
                                          sizeof(hash_obj_len),
                                          &get_property_bytes_written,
                                          0));
    m_internal_storage = malloc(hash_obj_len);
    assert(m_internal_storage != nullptr);

    m_error = m_error
              || FAILED(BCryptCreateHash((BCRYPT_ALG_HANDLE)m_algorithm_handle,
                                         (BCRYPT_HASH_HANDLE*)&m_hash_handle,
                                         (UCHAR*)m_internal_storage,
                                         hash_obj_len,
                                         nullptr,
                                         0,
                                         BCRYPT_HASH_REUSABLE_FLAG));
}

Sha256Context::~Sha256Context()
{
    if(m_hash_handle != nullptr)
    {
        BCryptDestroyHash(m_hash_handle);
    }
    free(m_internal_storage);
    if(m_algorithm_handle != nullptr)
    {
        BCryptCloseAlgorithmProvider(m_algorithm_handle, 0);
    }
}

void Sha256Context::add_data(const uint8_t* buffer, size_t buffer_len)
{
    // The docs specify that BCryptHashData will not modify the contents of the given pointer.
    // It's not clear why it doesn't just take a const pointer.
    m_error = m_error || FAILED(BCryptHashData(m_hash_handle, const_cast<uint8_t*>(buffer), (ULONG)buffer_len, 0));
}

void Sha256Context::finalise(uint8_t (&output)[32])
{
    if(m_error)
    {
        memset(output, 0x00, sizeof(output));
    }
    else
    {
        m_error = FAILED(BCryptFinishHash(m_hash_handle, output, sizeof(output), 0));
    }
}

// ============
// Tests
// ============
#if MVTF_TESTS_ENABLED
MVTF_TEST(sha256_correctly_hashes_input_text)
{
    const std::string_view input = "some random example text";
    uint8_t output[32];
    Sha256Context ctx;
    ctx.add_data((const uint8_t*)input.data(), input.length());
    ctx.finalise(output);

    pfc::string8 dumped = pfc::format_hexdump(output, sizeof(output), "");
    const pfc::string8 expected_output = "AB5FE4152E4ECEC0FE1863076E21AF2A99C87D1F109B66BB72A8A751D7DFAFA0";
    ASSERT(output[0] == 0xAB);
    ASSERT(dumped == expected_output);
    ASSERT(!ctx.m_error);
}
#endif
