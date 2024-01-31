#ifdef __linux__
#ifndef EXE_INFO_H_LINUX_
#define EXE_INFO_H_LINUX_

#include <stddef.h>

int p_getExePath(char *buf, size_t buf_size);

#endif /* EXE_INFO_H_LINUX_ */
#endif /* __linux__ */
