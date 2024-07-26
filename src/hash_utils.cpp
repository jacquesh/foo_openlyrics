#include "stdafx.h"

#include <windows.h>
#include <bcrypt.h>

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "hash_utils.h"

#define ASSERT_SUCCESS(Status) assert(((NTSTATUS)(Status)) >= 0)

Sha256Context::Sha256Context()
{
    ASSERT_SUCCESS(BCryptOpenAlgorithmProvider((BCRYPT_ALG_HANDLE*)&m_algorithm_handle,
                                               BCRYPT_SHA256_ALGORITHM,
                                               nullptr,
                                               BCRYPT_HASH_REUSABLE_FLAG));

    // BCryptGetProperty wants to output the number of bytes written to the output buffer.
    // This parameter is meant to be optional but we get failures without it.
    ULONG get_property_bytes_written = 0;
    DWORD hash_obj_len = 0;
    ASSERT_SUCCESS(BCryptGetProperty((BCRYPT_ALG_HANDLE)m_algorithm_handle,
                                     BCRYPT_OBJECT_LENGTH,
                                     (PBYTE)&hash_obj_len,
                                     sizeof(hash_obj_len),
                                     &get_property_bytes_written,
                                     0));
    m_internal_storage = malloc(hash_obj_len);
    assert(m_internal_storage != nullptr);

    ASSERT_SUCCESS(BCryptCreateHash((BCRYPT_ALG_HANDLE)m_algorithm_handle,
                                    (BCRYPT_HASH_HANDLE*)&m_hash_handle,
                                    (UCHAR*)m_internal_storage,
                                    hash_obj_len,
                                    nullptr,
                                    0,
                                    BCRYPT_HASH_REUSABLE_FLAG));
}

Sha256Context::~Sha256Context()
{
    BCryptDestroyHash(m_hash_handle);
    free(m_internal_storage);
    BCryptCloseAlgorithmProvider(m_algorithm_handle, 0);
}

void Sha256Context::add_data(uint8_t* buffer, size_t buffer_len)
{
    ASSERT_SUCCESS(BCryptHashData(m_hash_handle, buffer, (ULONG)buffer_len, 0));
}

void Sha256Context::finalise(uint8_t (&output)[32])
{
    ASSERT_SUCCESS(BCryptFinishHash(m_hash_handle, output, sizeof(output), 0));
}
