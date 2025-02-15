#pragma once
// ============================================================================
// MVTF: Minimum Viable Test Framework
//
// Include this header and write your tests directly in your source files.
// In *one* translation unit, #define MVTF_IMPLEMENTATION before the #include,
// to generate the code required to store & run all the tests.
// To compile out the test code, just define NDEBUG and enable compiler optimisations.
// Alternatively, the MVTF_TESTS_ENABLE #define is provided so you can be 100% extra-sure.
//
// To run the tests, simply load the DLL and call `run_mvtf_tests`.
// ============================================================================

#ifndef NDEBUG
#define MVTF_TESTS_ENABLED 1
#define MVTF_TEST(TEST_NAME) MVTF_TEST_FUNCTION_TYPE TEST_NAME; static int mvtf_test_##TEST_NAME = mvtf_register_function(&TEST_NAME, #TEST_NAME); void TEST_NAME(int* mvtf_error_count)

#else // Tests are *not* enabled
// static functions that aren't called/referenced anywhere should be removed during compilation.
// Some compilers always do this, some will only remove the code if optimisations are enabled.
#define MVTF_TESTS_ENABLED 0
#define MVTF_TEST(TEST_NAME) static void TEST_NAME(int* mvtf_error_count)
#endif

// TODO: Do a debugbreak if the debugger is attached?
#define ASSERT(CONDITION) do{if(!(CONDITION)){ *mvtf_error_count = *mvtf_error_count + 1; return; }}while(false)
#define CHECK(CONDITION) do{if(!(CONDITION)){ *mvtf_error_count = *mvtf_error_count + 1; }}while(false)

typedef void(MVTF_TEST_FUNCTION_TYPE)(int*);

typedef struct
{
    MVTF_TEST_FUNCTION_TYPE* ptr;
    const char* name;
} mvtf_function_metadata;

int mvtf_register_function(MVTF_TEST_FUNCTION_TYPE* ptr, const char* name);

#if defined(MVTF_IMPLEMENTATION) && MVTF_TESTS_ENABLED
#include <stdio.h> // printf
#include <stdlib.h> // Realloc

static int mvtf_test_count = 0;
static mvtf_function_metadata* mvtf_test_functions = nullptr;

int mvtf_register_function(MVTF_TEST_FUNCTION_TYPE* ptr, const char* name)
{
    // TODO: Only realloc when we reach a new power of 2 in the test count, to minimise reallocs
    mvtf_test_functions = (mvtf_function_metadata*)realloc(mvtf_test_functions, sizeof(*mvtf_test_functions) * (mvtf_test_count+1));
    mvtf_test_functions[mvtf_test_count] = { ptr, name };
    mvtf_test_count++;
    return mvtf_test_count;
}

static int sort_by_name(const void* lhs_void, const void* rhs_void)
{
    return strcmp(
            ((const mvtf_function_metadata*)lhs_void)->name,
            ((const mvtf_function_metadata*)rhs_void)->name
        );
}

extern "C" __declspec(dllexport) int run_mvtf_tests()
{
    int return_code = 0;
    printf("Executing %d test functions...\n", mvtf_test_count);
    LARGE_INTEGER start_time = {};
    QueryPerformanceCounter(&start_time);
    qsort(mvtf_test_functions, mvtf_test_count, sizeof(*mvtf_test_functions), sort_by_name);
    for(int i=0; i<mvtf_test_count; i++)
    {
        int error_count = 0;
        mvtf_test_functions[i].ptr(&error_count);

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
        printf("[%s] %s\n", status_str, mvtf_test_functions[i].name);
    }

    LARGE_INTEGER end_time = {};
    QueryPerformanceCounter(&end_time);
    LARGE_INTEGER tick_freq = {};
    QueryPerformanceFrequency(&tick_freq);
    uint64_t elapsed_ticks = end_time.QuadPart - start_time.QuadPart;
    double elapsed_sec = double(elapsed_ticks) / double(tick_freq.QuadPart);
    printf("Test execution completed in %.2fms\n", elapsed_sec * 1000.0);
    return return_code;
}
#endif // MVTF_IMPLEMENTATION
