#include "pressable-obj.h"
#include <stdlib.h>
#include <assert.h>

po_PressableObj* po_createPressableObj()
{
    /* Allocate space on the heap... */
    po_PressableObj *po = malloc(sizeof(po_PressableObj));
    assert(po != NULL);

    /* ...And populate it with the default values for all the members */
    po->up = false;
    po->down = false;
    po->pressed = false;
    po->forceReleased = false;
    po->time = 0;

    return po;
}

void po_updatePressableObj(po_PressableObj* po, bool state)
{
    /* Down should only be true if on the current tick the object is pressed, but wasn't pressed on the previous one */
    po->down = (state && !po->pressed && !po->forceReleased);

    /* Down is active only when the object WAS pressed, and now is not. */
    po->up = (!state && po->pressed && !po->forceReleased);

    /* Update the po->pressed member only if the object isn't force released */
    po->pressed = state & !po->forceReleased;

    /* Time pressed should always be incremented when the object is pressed */
    if(state) {
        po->time++;
    } else {
        /* Otherwise reset the time, as well as the forceReleased state */
        po->time = 0;
        po->forceReleased = false;
    }
}

void po_forceReleasePressableObj(po_PressableObj *po)
{
    /* The pressed, up and down values as well as the time should be 0 until the forceReleased state is reset */
    po->pressed = false;
    po->up = false;
    po->down = false;
    po->time = 0;
    po->forceReleased = true;
}

void po_destroyPressableObj(po_PressableObj *po)
{
    /* Free the memory occupied by the now not needed pressable object struct */
    free(po);
}
