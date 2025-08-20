#ifndef P_ERROR_H_
#define P_ERROR_H_

#include <platform/common/guard.h>

/* A wrapper around `GetLastError` */
const char * get_last_error_msg(void);

#endif /* P_ERROR_H_ */
