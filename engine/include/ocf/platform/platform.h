// SPDX-License-Identifier: MIT

#pragma once

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
