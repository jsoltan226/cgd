#ifndef EXE_INFO_H
#define EXE_INFO_H

#include "platform.h"

#if (PLATFORM_NAME == PLATFORM_LINUX)
#include <cgd/platform/linux/exe-info.h>

#elif (PLATFORM_NAME == PLATFORM_WINDOWS)
#include <cgd/platform/linux/exe-info.h>
#endif

#endif /* EXE_INFO_H */
