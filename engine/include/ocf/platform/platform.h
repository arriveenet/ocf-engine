// SPDX-License-Identifier: MIT

#pragma once

#if defined(OCF_LIBRARY_STATIC)
  #define OCF_API
#elif defined(_WIN32) || defined(__CYGWIN__)
  #ifdef OCF_LIBRARY_EXPORTS
    #define OCF_API __declspec(dllexport)
  #else
    #define OCF_API __declspec(dllimport)
  #endif
#else
  #if __GNUC__ >= 4
    #define OCF_API __attribute__((visibility("default")))
  #else
    #define OCF_API
  #endif
#endif

#define OCF_PLATFORM_UNKNOWN 0 // Unknown platform
#define OCF_PLATFORM_WIN32 1   // Windows platform
#define OCF_PLATFORM_LINUX 2   // Linux platform

// Define the target platform
#define OCF_TARGET_PLATFORM OCF_PLATFORM_UNKNOWN

// Windows
#if defined(_WIN32) || defined(_WIN64)
#undef OCF_TARGET_PLATFORM
#define OCF_TARGET_PLATFORM OCF_PLATFORM_WIN32
#endif

// Linux
#if defined(__linux__)
#undef OCF_TARGET_PLATFORM
#define OCF_TARGET_PLATFORM OCF_PLATFORM_LINUX
#endif
