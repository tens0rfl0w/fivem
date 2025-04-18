/***
* Copyright (C) Microsoft. All rights reserved.
* Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
*
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Pre-compiled headers
*
* For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#pragma once

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#pragma clang diagnostic ignored "-Winfinite-recursion"
#endif


#ifdef _WIN32
#ifdef CPPREST_TARGET_XP
#include <winsdkver.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WS03 //Windows XP with SP2
#endif
#endif
#include <SDKDDKVer.h>
// use the debug version of the CRT if _DEBUG is defined
#ifdef _DEBUG
    #define _CRTDBG_MAP_ALLOC
    #include <stdlib.h>
    #include <crtdbg.h>
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#define NOMINMAX
#endif

#include <windows.h>
#include <objbase.h>

// Windows Header Files:
#if !defined(__cplusplus_winrt)
#include <winhttp.h>

#endif // #if !defined(__cplusplus_winrt)
#else // LINUX or APPLE
#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <cstdint>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <signal.h>
#include "pthread.h"
#if (defined(ANDROID) || defined(__ANDROID__))
// Boost doesn't recognize libstdcpp on top of clang correctly
#include "boost/config.hpp"
#include "boost/config/stdlib/libstdcpp3.hpp"
#undef BOOST_NO_CXX11_SMART_PTR
#undef BOOST_NO_CXX11_NULLPTR
#endif
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include "boost/bind/bind.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#endif // _WIN32

// Macro indicating the C++ Rest SDK product itself is being built.
// This is to help track how many developers are directly building from source themselves.
#define _CASA_BUILD_FROM_SRC

#include <iostream>
#include <fstream>
#include <algorithm>
#include <exception>
#include <assert.h>
#include <streambuf>
#include <atomic>
#include <mutex>
#include <array>
#include <vector>
#include <memory>
#include <thread>
#include <set>

#include "pplx/pplxtasks.h"

#if defined(max)
#error: max macro defined -- make sure to #define NOMINMAX before including windows.h
#endif
#if defined(min)
#error: min macro defined -- make sure to #define NOMINMAX before including windows.h
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
