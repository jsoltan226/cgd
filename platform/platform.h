#ifndef PLATFORM_H
#define PLATFORM_H

/* Credits to this post: https://stackoverflow.com/a/42040445 */

#define PLATFORM_LIST                                   \
    X_(PLATFORM_UNSUPPORTED,    0)                      \
    X_(PLATFORM_LINUX,          1)                      \
    X_(PLATFORM_WINDOWS,        2)                      \
    X_(PLATFORM_MACOS,          PLATFORM_UNSUPPORTED)   \
    X_(PLATFORM_ANDROID,        PLATFORM_UNSUPPORTED)   \
    X_(PLATFORM_IOS,            PLATFORM_UNSUPPORTED)   \
    X_(PLATFORM_BSD,            PLATFORM_UNSUPPORTED)   \
    X_(PLATFORM_HPUX,           PLATFORM_UNSUPPORTED)   \
    X_(PLATFORM_SOLARIS,        PLATFORM_UNSUPPORTED)   \
    X_(PLATFORM_AIX,            PLATFORM_UNSUPPORTED)   \

#define PLATFORM_UNSUPPORTED    0
#define PLATFORM_LINUX          1
#define PLATFORM_WINDOWS        2
#define PLATFORM_MACOS          PLATFORM_UNSUPPORTED
#define PLATFORM_ANDROID        PLATFORM_UNSUPPORTED
#define PLATFORM_IOS            PLATFORM_UNSUPPORTED
#define PLATFORM_BSD            PLATFORM_UNSUPPORTED
#define PLATFORM_HPUX           PLATFORM_UNSUPPORTED
#define PLATFORM_SOLARIS        PLATFORM_UNSUPPORTED
#define PLATFORM_AIX            PLATFORM_UNSUPPORTED

#define X_(name, value) #name,
const char * const PLATFORM_NAMES[] = {
    PLATFORM_LIST
};
#undef X_

#if defined(_WIN32)
    #define PLATFORM PLATFORM_WINDOWS /* Windows */
#elif defined(_WIN64)
    #define PLATFORM PLATFORM_WINDOWS /* Windows */
#elif defined(__CYGWIN__) && !defined(_WIN32)
    #define PLATFORM PLATFORM_WINDOWS /* Windows (Cygwin POSIX under Microsoft Windows) */
#elif defined(__ANDROID__)
    #define PLATFORM PLATFORM_ANDROID /* Android (implies Linux, so it must come first) */
#elif defined(__linux__)
    #define PLATFORM PLATFORM_LINUX /* Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other */
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
    #include <sys/param.h>
    #if defined(BSD)
        #define PLATFORM PLATFORM_BSD /* FreeBSD, NetBSD, OpenBSD, DragonFly BSD */
    #endif
#elif defined(__hpux)
    #define PLATFORM "hp-ux" /* HP-UX */
#elif defined(_AIX)
    #define PLATFORM "aix" /* IBM AIX */
#elif defined(__APPLE__) && defined(__MACH__) /* Apple OSX and iOS (Darwin) */
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM "ios" /* Apple iOS */
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM "ios" /* Apple iOS */
    #elif TARGET_OS_MAC == 1
        #define PLATFORM "osx" /* Apple OSX */
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM "solaris" /* Oracle Solaris, Open Indiana */
#else
    #define PLATFORM PLATFORM_UNSUPPORTED
#endif

#endif /* PLATFORM_H */
