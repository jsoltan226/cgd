#include "pressable-obj.h"
#include "core/log.h"
#include <stdlib.h>

pressable_obj_t * pressable_obj_create(void)
{
    pressable_obj_t *po = calloc(1, sizeof(pressable_obj_t));
    if (po == NULL) {
        s_log_error("pressable-obj", "malloc() for pressable obj failed");
        return NULL;
    }

    return po;
}

void pressable_obj_update(pressable_obj_t *po, bool state)
{
    /* Down is active if on the current tick the object is pressed, but wasn't pressed on the previous one */
    po->down = (state && !po->pressed && !po->forceReleased);

    /* Up is active immidiately after object was released. */
    po->up = (!state && po->pressed && !po->forceReleased);

    /* Update the po->pressed member only if the object isn't force released */
    po->pressed = state && !po->forceReleased;

    /* Time pressed should always be incremented when the object is pressed */
    if(state) {
        po->time++;
    } else {
        /* Otherwise reset the time, as well as the forceReleased state */
        po->time = 0;
        po->forceReleased = false;
    }
}

void pressable_obj_force_release(pressable_obj_t *po)
{
    /* The pressed, up and down values as well as the time should be 0 until the forceReleased state is reset */
    po->pressed = false;
    po->up = false;
    po->down = false;
    po->time = 0;
    po->forceReleased = true;
}

void pressable_obj_destroy(pressable_obj_t *po)
{
    free(po);
}
