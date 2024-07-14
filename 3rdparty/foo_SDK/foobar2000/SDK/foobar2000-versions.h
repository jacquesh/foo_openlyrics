#pragma once
// foobar2000-versions.h
// foobar2000 SDK version and target API levels are declared in this header

#ifdef _WIN32
// Windows

// This SDK does NOT SUPPORT targets older than API 80 / foobar2000 v1.5
#define FOOBAR2000_TARGET_VERSION 80 // 1.5, 1.6
// #define FOOBAR2000_TARGET_VERSION 81 // 2.0


#ifdef _M_IX86
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 72
#else
// x64 & ARM64 targets
// Allow components made with special foobar2000 v1.6 SDK with x64 & ARM64 targets
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 80
#endif


#else // _WIN32
// Not Windows
#define FOOBAR2000_TARGET_VERSION 81
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 81
#endif // _WIN32

// Can safely use foobar2000 v2.0 features?
#define FOOBAR2020 (FOOBAR2000_TARGET_VERSION>=81)


// Use this to determine what foobar2000 SDK version is in use, undefined for releases older than 2018
#define FOOBAR2000_SDK_VERSION 20230913

// cfg_var downgrade support, experimental, intended for specific components only.
// Allows new style configStore data to be imported back to old foobar2000 friendly cfg_vars.
// Intended to retain config when reverting FOOBAR2000_TARGET_VERSION value of 81 or newer to 80.
// Takes effect with FOOBAR2000_TARGET_VERSION 80 only.
// Place FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE somewhere in your code to declare init calls for cfg_var downgrade. Or, if you wish to call manually, call cfg_var_reader::downgrade_main() before accessing your cfg_vars.
// Spurious calls to cfg_var_reader::downgrade_main() will be ignored, only first one will take effect.
#define FOOBAR2000_SUPPORT_CFG_VAR_DOWNGRADE 0