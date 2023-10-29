#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "pressable-obj.h"

typedef enum {
    INPUT_MOUSE_LEFTBUTTONMASK = 1,
    INPUT_MOUSE_RIGHTBUTTONMASK = 2,
    INPUT_MOUSE_MIDDLEBUTTONMASK = 4,
} input_Mouse_ButtonMask;

#define INPUT_MOUSE_EVERYBUTTONMASK (INPUT_MOUSE_LEFTBUTTONMASK | INPUT_MOUSE_RIGHTBUTTONMASK | INPUT_MOUSE_MIDDLEBUTTONMASK)

typedef struct {
    po_PressableObj buttonLeft;
    po_PressableObj buttonRight;
    po_PressableObj buttonMiddle;
    int x;
    int y;
} input_Mouse;

input_Mouse* input_initMouse();
void input_updateMouse(input_Mouse *mouse);
void input_forceReleaseMouse(input_Mouse *mouse, int buttons);
void input_destroyMouse(input_Mouse *mouse);

#endif
