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
