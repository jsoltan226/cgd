#ifndef P_KEYBOARD_H_
#define P_KEYBOARD_H_

#include "core/int.h"
struct p_keyboard;

struct p_keyboard * p_keyboard_init();
void p_keyboard_destroy(struct p_keyboard *kb);

#endif /* P_KEYBOARD_H_ */
