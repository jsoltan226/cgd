#ifndef PRESSABLEOBJ_H
#define PRESSABLEOBJ_H

#include <stdbool.h>

typedef struct {
    bool up, down, pressed, forceReleased;
    int time;
} po_PressableObj;

po_PressableObj po_createPressableObj();
void po_updatePressableObj(po_PressableObj* po, bool pressed);
void po_forceReleasePressableObj(po_PressableObj *po);

#endif
