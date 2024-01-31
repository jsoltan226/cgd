#ifndef PLATFORM_H
#define PLATFORM_H

/* Credits to this post: https://stackoverflow.com/a/42040445 */

enum PLATFORMS {
    PLATFORM_UNSUPPORTED,
    PLATFORM_LINUX,
    PLATFORM_WINDOWS,
    PLATFORM_MACOS = PLATFORM_UNSUPPORTED,
    PLATFORM_ANDROID = PLATFORM_UNSUPPORTED,
    PLATFORM_IOS = PLATFORM_UNSUPPORTED,
    PLATFORM_BSD = PLATFORM_UNSUPPORTED,
    PLATFORM_HPUX = PLATFORM_UNSUPPORTED,
    PLATFORM_SOLARIS = PLATFORM_UNSUPPORTED,
    PLATFORM_AIX = PLATFORM_UNSUPPORTED,
};
const char * const PLATFORM_NAMES[] = {
    /* [PLATFORM_UNSUPPORTED] = */ "Unsupported",
    /* [PLATFORM_LINUX]       = */ "Linux",
    /* [PLATFORM_WINDOWS]     = */ "Windows"
};
#if defined(_WIN32)
    #define PLATFORM_NAME PLATFORM_WINDOWS /* Windows */
#elif defined(_WIN64)
    #define PLATFORM_NAME PLATFORM_WINDOWS /* Windows */
#elif defined(__CYGWIN__) && !defined(_WIN32)
    #define PLATFORM_NAME PLATFORM_WINDOWS /* Windows (Cygwin POSIX under Microsoft Windows) */
#elif defined(__ANDROID__)
    #define PLATFORM_NAME PLATFORM_ANDROID /* Android (implies Linux, so it must come first) */
#elif defined(__linux__)
    #define PLATFORM_NAME PLATFORM_LINUX /* Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other */
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
    #include <sys/param.h>
    #if defined(BSD)
        #define PLATFORM_NAME PLATFORM_BSD /* FreeBSD, NetBSD, OpenBSD, DragonFly BSD */
    #endif
#elif defined(__hpux)
    #define PLATFORM_NAME "hp-ux" /* HP-UX */
#elif defined(_AIX)
    #define PLATFORM_NAME "aix" /* IBM AIX */
#elif defined(__APPLE__) && defined(__MACH__) /* Apple OSX and iOS (Darwin) */
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM_NAME "ios" /* Apple iOS */
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM_NAME "ios" /* Apple iOS */
    #elif TARGET_OS_MAC == 1
        #define PLATFORM_NAME "osx" /* Apple OSX */
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM_NAME "solaris" /* Oracle Solaris, Open Indiana */
#else
    #define PLATFORM_NAME PLATFORM_UNSUPPORTED
#endif

#endif /* PLATFORM_H */
