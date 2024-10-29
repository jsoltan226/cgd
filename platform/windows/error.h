#ifndef ERROR_H_
#define ERROR_H_
#endif /* ERROR_H_ */

#ifndef P_INTERNAL_GUARD__
#error This header file is internal to the cgd platform module and is not intended to be used elsewhere
#endif /* P_INTERNAL_GUARD__ */

/* A wrapper around `GetLastError` */
const char * get_last_error_msg(void);
