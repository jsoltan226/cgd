#pragma once
#ifndef MOUSE_H
#define MOUSE_H

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <cgd/user-input/pressable-obj.h>

enum ms_Mouse_Button {
    MS_BUTTON_LEFT,
    MS_BUTTON_RIGHT,
    MS_BUTTON_MIDDLE,
    MS_N_BUTTONS
};

enum ms_Mouse_ButtonMask {
    MS_LEFTBUTTONMASK   = 1 << MS_BUTTON_LEFT,
    MS_RIGHTBUTTONMASK  = 1 << MS_BUTTON_RIGHT,
    MS_MIDDLEBUTTONMASK = 1 << MS_BUTTON_MIDDLE,
};

#define MS_EVERYBUTTONMASK ((1 << MS_N_BUTTONS) - 1)

typedef struct {
    po_PressableObj buttons[MS_N_BUTTONS];
    int x, y;
} ms_Mouse;

ms_Mouse* ms_initMouse();
void ms_updateMouse(ms_Mouse *mouse);
void ms_forceReleaseMouse(ms_Mouse *mouse, int button_masks);
void ms_destroyMouse(ms_Mouse *mouse);

#endif
