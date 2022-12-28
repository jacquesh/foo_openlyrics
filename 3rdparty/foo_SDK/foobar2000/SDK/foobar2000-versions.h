#pragma once
// foobar2000-versions.h
// foobar2000 SDK version and target API levels are declared in this header

// This SDK does NOT SUPPORT targets older than API 80 / foobar2000 v1.5
#define FOOBAR2000_TARGET_VERSION 80 // 1.5, 1.6
// #define FOOBAR2000_TARGET_VERSION 81 // 2.0

#ifdef _M_IX86
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 72
#else
// x64 & ARM64 targets
// Allow components made with new foobar2000 v1.6 SDK with x64 & ARM64 targets
#define FOOBAR2000_TARGET_VERSION_COMPATIBLE 80
#endif

// Can safely use foobar2000 v2.0 features?
#define FOOBAR2020 (FOOBAR2000_TARGET_VERSION>=81)


// Use this to determine what foobar2000 SDK version is in use, undefined for releases older than 2018
#define FOOBAR2000_SDK_VERSION 20221116