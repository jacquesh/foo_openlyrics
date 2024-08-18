#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <stdio.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#define TARGET_PROCESS_NAME "foobar2000.exe"
#define PLUGIN_FILE_NAME "foo_openlyrics.dll"

// foo_pluginstall
// Set the PLUGIN_FILE_NAME macro to the name of your plugin dll, and then run this in your build output directory.
// It will find running instance(s) of fb2k, check where they have your plugin installed to, then close those instances,
// copy over your plugin and restart (one instance of) fb2k.
// The intended workflow for you to add this project to your solution, set it as the default launch project with
// your build output directory as the working directory, and then just run it whenever you want to test your plugin.

static const char* GetErrorString(DWORD error)
{
    static char errorMsgBuffer[4096];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error, 0, errorMsgBuffer, sizeof(errorMsgBuffer), nullptr);
    return errorMsgBuffer;
}

static const char* GetLastErrorString()
{
    return GetErrorString(GetLastError());
}

struct InstanceInfo
{
    DWORD pid;
    HANDLE process;
    std::vector<HWND> windows;
};

BOOL CALLBACK window_proc(HWND hwnd, LPARAM lparam)
{
    DWORD process_id = 0;
    GetWindowThreadProcessId(hwnd, &process_id);
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, false, process_id);
    if(process == nullptr)
    {
        return true;
    }

    char exe_path[1024] = {};
    char exe_path_full[1024] = {};
    char* exe_name_ptr = nullptr;
    GetProcessImageFileNameA(process, exe_path, sizeof(exe_path));
    GetFullPathNameA(exe_path, sizeof(exe_path_full), exe_path_full, &exe_name_ptr);
    std::string_view exe_name(exe_name_ptr);
    if(exe_name != TARGET_PROCESS_NAME)
    {
        CloseHandle(process);
        return true;
    }

    std::vector<InstanceInfo>& instances = *((std::vector<InstanceInfo>*)lparam);
    const auto is_our_instance = [process_id](const InstanceInfo& i) { return i.pid == process_id; };
    auto instance_iter = std::find_if(instances.begin(), instances.end(), is_our_instance);
    if(instance_iter == instances.end())
    {
        instances.push_back(InstanceInfo
        {
            process_id,
            process,
            {hwnd},
        });
        // Don't close the process handle because we just stored it and we'll use it again later
    }
    else
    {
        instance_iter->windows.push_back(hwnd);
        CloseHandle(process);
    }

    return true;
}

int main()
{
    std::vector<InstanceInfo> instances;
    EnumWindows(window_proc, (LPARAM)&instances);

    std::string plugin_path;
    std::string fb2k_path;

    for(InstanceInfo& instance : instances)
    {
        if(plugin_path.empty())
        {
            std::vector<HMODULE> modules;
            modules.resize(4096);

            DWORD module_bytes_required = 0;
            const BOOL enum_modules_success = EnumProcessModules(instance.process, modules.data(), modules.size() * sizeof(decltype(modules)::value_type), &module_bytes_required);
            modules.resize(module_bytes_required/sizeof(HMODULE));
            if(!enum_modules_success)
            {
                printf("Failed to enumerate process modules: %lu/%s", GetLastError(), GetLastErrorString());
                __debugbreak();
                continue;
            }

            char name_buffer[1024] = {};
            DWORD name_length = GetModuleFileNameExA(instance.process, modules[0], name_buffer, sizeof(name_buffer));
            fb2k_path = std::string(name_buffer, name_length);

            for(HMODULE module : modules)
            {
                name_length = GetModuleBaseNameA(instance.process, module, name_buffer, sizeof(name_buffer));
                std::string_view module_base_name(name_buffer, name_length);
                if(module_base_name != PLUGIN_FILE_NAME)
                {
                    continue;
                }

                name_length = GetModuleFileNameExA(instance.process, module, name_buffer, sizeof(name_buffer));
                plugin_path = std::string(name_buffer, name_length);
            }
        }

        for(HWND hwnd : instance.windows)
        {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        const DWORD timeout_ms = 10'000;
        DWORD wait_result = WaitForSingleObject(instance.process, timeout_ms);
        if(wait_result != WAIT_OBJECT_0)
        {
            printf("Failed to wait for the process to terminate\n");
            if(wait_result == WAIT_FAILED)
            {
                printf("Wait failure: %lu/%s", GetLastError(), GetLastErrorString());
            }
            __debugbreak();
            return 1;
        }
        CloseHandle(instance.process);
    }

    if(plugin_path.empty() || fb2k_path.empty())
    {
        printf("Failed to determine plugin path or fb2k exe path!\n");
        __debugbreak();
        return 1;
    }

    const char* source_path = PLUGIN_FILE_NAME;
    const bool fail_if_exists = false;
    BOOL copy_success = CopyFileA(source_path, plugin_path.c_str(), fail_if_exists);
    if(!copy_success)
    {
        printf("Failed to copy file: %lu/%s", GetLastError(), GetLastErrorString());
        __debugbreak();
    }

    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process_info = {};
    BOOL create_success = CreateProcessA(
        fb2k_path.c_str(),
        nullptr, nullptr, nullptr,
        false, 0, nullptr, nullptr,
        &startup,
        &process_info
    );
    if(!create_success)
    {
        printf("Failed to spawn new process: %lu/%s", GetLastError(), GetLastErrorString());
        __debugbreak();
    }
}
