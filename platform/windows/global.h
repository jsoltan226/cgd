#ifndef P_GLOBAL_H_
#define P_GLOBAL_H_

#include <platform/common/guard.h>

#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN */
#include <windows.h>
#include <minwindef.h>

extern volatile HINSTANCE g_instance_handle;
extern volatile int g_n_cmd_show;

#endif /* P_GLOBAL_H_ */
