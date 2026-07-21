#pragma once
#ifndef Circular8_LED_H
#define Circular8_LED_H

#ifdef Circular8_ENABLE_CONTROLLER
#include <stdbool.h>
#include "thirdparty/serial/serial.h"
bool openSerialControllerLights(const char* serialPort);
SDL_Color applyFCMedalLightingEffect(int lightFCType, Uint64 timeSinceLastNote);
bool setLightForOneButton(Uint8 button, Uint8 r, Uint8 g, Uint8 b);
bool setLightForMultipleButtons(Uint8 start, Uint8 end, Uint8 r, Uint8 g, Uint8 b);

bool setVirtualLightForOneButton(Uint8 button, Uint8 r, Uint8 g, Uint8 b);
bool setVirtualLightForMultipleButtons(Uint8 start, Uint8 end, Uint8 r, Uint8 g, Uint8 b);
bool tryToMakeVirtualLightsReal(void);
extern SDL_Mutex* LEDMutex;
extern SDL_Thread* LEDThread;

bool refreshLightCommands(void);
extern SERIAL serialControllerLights;
extern volatile bool isUsingSerialLightController;
extern char* lightsSerialPort;
#endif // Circular8_ENABLE_CONTROLLER
#endif // Circular8_LED_H
