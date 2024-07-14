#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>

typedef int(*bvtf_func)();
int main(int argc, char** argv)
{
    if(argc == 1)
    {
        const char* program_name = argv[0];
        printf("No test library provided to %s\n", program_name);
        printf("Please pass in the path to a DLL containing your tests as the first argument");
        return 1;
    }

    const char* library_path = argv[1];
    const DWORD lib_attribs = GetFileAttributesA(library_path);
    if(lib_attribs == INVALID_FILE_ATTRIBUTES)
    {
        printf("Failed to check test library path '%s'. Does that file exist?", library_path);
        return 1;
    }

    HMODULE lib = LoadLibraryA(library_path);
    if(lib == nullptr)
    {
        printf("Failed to load test library from path '%s'", library_path);
        return 1;
    }

    const char* main_name =  "run_mvtf_tests";
    bvtf_func bvtf_main = (bvtf_func)GetProcAddress(lib, main_name);
    if(bvtf_main == nullptr)
    {
        printf("Failed to load test entrypoint '%s' from test library at '%s'", main_name, library_path);
        return 1;
    }

    int result = bvtf_main();
    if(result == 0) {
        printf("All tests passed\n");
    } else {
        printf("Some tests failed\n");
    }
    return result;
}
