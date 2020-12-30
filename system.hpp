//
// System file.
//
// Retrieving information about platform
// (later also about memory limit, processor, GPU type, etc.)

#pragma once

namespace System
{
    // platform type of the system
    typedef enum {
        SYSTEM_LINUX,
        SYSTEM_MACOSX,
        SYSTEM_FREEBSD,
        SYSTEM_UNIX,
        SYSTEM_WINDOWS,   // both 32 and 64 bit
        SYSTEM_WINDOWS64, // only 64 bit
    } __platform_t;

    // let the pre-processor interpret the target platform
    #if defined(__linux__) || defined(linux) || defined(__linux)
        static __platform_t _system_platform = SYSTEM_LINUX;

    #elif defined(__APPLE__) || defined(__MACH__)
        static __platform_t _system_platform = SYSTEM_MACOSX;

    #elif __FreeBSD__
        static __platform_t _system_platform = SYSTEM_FREEBSD;

    #elif defined(unix) || defined(__unix) || defined(__unix__)
        static __platform_t _system_platform = SYSTEM_UNIX;

    #elif _WIN64
        static __platform_t _system_platform = SYSTEM_WINDOWS64;

    #elif _WIN32
        static __platform_t _system_platform = SYSTEM_WINDOWS;

    #else
        #error "OS not supported!"
    #endif


} // namespace system
