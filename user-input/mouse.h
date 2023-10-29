#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "pressable-obj.h"

typedef enum {
    MS_LEFTBUTTONMASK = 1,
    MS_RIGHTBUTTONMASK = 2,
    MS_MIDDLEBUTTONMASK = 4,
} ms_Mouse_ButtonMask;

#define MS_EVERYBUTTONMASK (MS_LEFTBUTTONMASK | MS_RIGHTBUTTONMASK | MS_MIDDLEBUTTONMASK)

typedef struct {
    po_PressableObj *buttonLeft;
    po_PressableObj *buttonRight;
    po_PressableObj *buttonMiddle;
    int x;
    int y;
} ms_Mouse;

ms_Mouse* ms_initMouse();
void ms_updateMouse(ms_Mouse *mouse);
void ms_forceReleaseMouse(ms_Mouse *mouse, int buttons);
void ms_destroyMouse(ms_Mouse *mouse);

#endif
