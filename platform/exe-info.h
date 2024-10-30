#ifndef P_EXE_INFO_H_
#define P_EXE_INFO_H_

#include <core/int.h>

/* Store the UNIX path to the executable in `buf`,
 * limiting the string length to `buf_size`.
 *
 * Returns 0 on success and non-zero on failure.
 */
i32 p_get_exe_path(char *buf, u32 buf_size);

#endif /* P_EXE_INFO_H_ */
