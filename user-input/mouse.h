#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include "pressable-obj.h"

typedef struct {
    po_PressableObj buttonLeft;
    po_PressableObj buttonRight;
    po_PressableObj buttonMiddle;
    int x;
    int y;
} input_Mouse;

extern void input_updateMouse(input_Mouse* mouse);

#endif
