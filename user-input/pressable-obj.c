#include "pressable-obj.h"

void po_initPressableObj(po_PressableObj* po)
{
    po->up = false;
    po->down = false;
    po->pressed = false;
    po->time = 0;
}

void po_updatePressableObj(po_PressableObj* po, bool state)
{
    po->down = (state && !po->pressed);
    po->up = (!state && po->pressed);

    po->pressed = state;
    if(po->pressed)
        po->time++;
    else
        po->time = 0;
}
