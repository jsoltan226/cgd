#ifndef MOUSE_H
#define MOUSE_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct {
    bool up;
    bool down;
    bool pressed;
    int time;
} input_MouseButton;

typedef struct {
    input_MouseButton buttonLeft;
    input_MouseButton buttonRight;
    input_MouseButton buttonMiddle;
    int x;
    int y;
} input_Mouse;

extern void input_updateMouse(input_Mouse* mouse);

#endif
