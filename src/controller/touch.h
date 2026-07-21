#pragma once
#ifndef Circular8_TOUCH_H
#define Circular8_TOUCH_H

#ifdef Circular8_ENABLE_CONTROLLER
#include <stdbool.h>
extern bool isUsingSerialController;
extern char* touchSerialPort;
bool openSerialControllerTouch(const char* serialPort);
void updateControllerTouchscreen(void);
void makeTouchscreenControllerSendData(void);
#include "thirdparty/serial/serial.h"
extern SERIAL serialControllerTouchscreen;
#endif // Circular8_ENABLE_CONTROLLER
#endif // Circular8_TOUCH_H
