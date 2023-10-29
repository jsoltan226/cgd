#include "pressable-obj.h"

po_PressableObj po_createPressableObj()
{
    po_PressableObj po;

    po.up = false;
    po.down = false;
    po.pressed = false;
    po.forceReleased = false;
    po.time = 0;

    return po;
}

void po_updatePressableObj(po_PressableObj* po, bool state)
{
    po->down = (state && !po->pressed);
    po->up = (!state && (po->pressed || po->forceReleased));

    po->forceReleased &= !po->up;

    po->pressed = state & !po->forceReleased;
    if(po->pressed)
        po->time++;
    else
        po->time = 0;
}

void po_forceReleasePressableObj(po_PressableObj *po)
{
    po->pressed = false;
    po->up = false;
    po->down = false;
    po->time = 0;
    po->forceReleased = true;
}
