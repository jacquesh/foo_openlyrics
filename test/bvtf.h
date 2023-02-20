#pragma once

#include <stdio.h>
#include <stdlib.h>

// TODO: This requires shared.dll to be copied to the binaries directory from the fb2k install dir.
// That kinda sucks but it's a consequence of linking to the fb2k SDK and the plugin DLL directly.
// I wonder if we can extract the relevant logic out into a static lib that doesn't depend on fb2k.

// TODO: Do a debugbreak if the debugger is attached?
#define ASSERT(CONDITION) do{if(!(CONDITION)){ *bvtf_error_count = *bvtf_error_count + 1; return; }}while(false)
#define CHECK(CONDITION) do{if(!(CONDITION)){ *bvtf_error_count = *bvtf_error_count + 1; }}while(false)

typedef void(BVTF_TEST_FUNCTION_TYPE)(int*);

typedef struct
{
    BVTF_TEST_FUNCTION_TYPE* ptr;
    const char* name;
} bvtf_function_metadata;

int bvtf_register_function(BVTF_TEST_FUNCTION_TYPE* ptr, const char* name);

#define BVTF_TEST(TEST_NAME) BVTF_TEST_FUNCTION_TYPE TEST_NAME; static int bvtf_test_##TEST_NAME = bvtf_register_function(&TEST_NAME, #TEST_NAME); void TEST_NAME(int* bvtf_error_count)

// TODO: It would be great if we could get away from using this macro and just leverage inline functions to allow every source file to be effectively equivalent in terms of infrastructure.
//       The problem is knowing where to store the data, since we need each translation unit to contribute to the same list of tests.
#ifdef BVTF_IMPLEMENTATION

static int bvtf_test_count = 0;
static bvtf_function_metadata* bvtf_test_functions = nullptr;

int bvtf_register_function(BVTF_TEST_FUNCTION_TYPE* ptr, const char* name)
{
    // TODO: Only realloc when we reach a new power of 2 in the test count, to minimise reallocs
    bvtf_test_functions = (bvtf_function_metadata*)realloc(bvtf_test_functions, sizeof(*bvtf_test_functions) * (bvtf_test_count+1));
    bvtf_test_functions[bvtf_test_count] = { ptr, name };
    bvtf_test_count++;
    return bvtf_test_count;
}

int main()
{
    int return_code = 0;
    printf("Executing %d test functions...\n", bvtf_test_count);
    for(int i=0; i<bvtf_test_count; i++)
    {
        int error_count = 0;
        bvtf_test_functions[i].ptr(&error_count);

        const char* status_str = "";
        if(error_count == 0)
        {
            status_str = "\033[32m" "PASSED" "\033[39m";
        }
        else
        {
            status_str = "\033[31m" "FAILED" "\033[39m";
            return_code = 1;
        }
        printf("[%s] %s\n", status_str, bvtf_test_functions[i].name);
    }

    printf("Done");
    return return_code;
}
#endif // BVTF_IMPLEMENTATION
