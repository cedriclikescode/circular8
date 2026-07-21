#pragma once
#ifndef Circular8_EVENTS_H
#define Circular8_EVENTS_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#ifdef Circular8_ENABLE_CONTROLLER
#include "thirdparty/serial/serial.h"
#endif // Circular8_ENABLE_CONTROLLER

extern bool pressedButtons[8];
extern bool releasedButtons[8];
extern bool heldButtons[8];
extern bool pressedTouchSensors[8 * 4 + 2];
extern bool heldTouchSensors[8 * 4 + 2];
extern bool releasedTouchSensors[8 * 4 + 2];
extern bool realHeldButtons[8];
SDL_FPoint getMousePosition(void);
extern Uint64 buttonDebounceDelay;
extern bool didMousePressed;
bool getMousePressedOnce(void);
void updateNormalButtons(Uint64 timestamp);
SDL_AppResult handleEvent(SDL_Event *event);
void resetButtons(void);

bool isButtonOrTouchPressed(int button);
bool isButtonAndTouchReleased(int button);
extern int controllerPlayer;
void updateScaling(void);
void updateTouchscreen(void);
extern bool hasTouchscreenUpdated;
#endif // Circular8_EVENTS_H
