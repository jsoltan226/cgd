#ifndef PRESSABLEOBJ_H
#define PRESSABLEOBJ_H

#include <stdbool.h>

typedef struct {
    bool up, down, pressed;
    int time;
} po_PressableObj;

po_PressableObj po_createPressableObj();
void po_updatePressableObj(po_PressableObj* po, bool pressed);

#endif
