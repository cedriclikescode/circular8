/*
    MIT License

    Copyright (c) 2026 cedriclikescode

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <stdio.h>
#include <stdbool.h>
#ifdef Circular8_ENABLE_CONTROLLER
#include "render.h"
#include "miscutils.h"
#include "game/ingame.h"
#include "game/scorescreen.h"
#include "main.h"
#include "events.h"
#include "controller/led.h"

#include <SDL3/SDL.h>
#include "thirdparty/serial/serial.h"

char* lightsSerialPort = NULL;
volatile bool isUsingSerialLightController = false;
SERIAL serialControllerLights;

static Uint8 getChecksum(const Uint8* data, size_t len){
    size_t total = 0;
    for(size_t i = 0; i < len; i++){
        total += data[i];
    }
    return (Uint8)(total & 0xFF);
}

static inline bool needsEscaping(Uint8 data){
    return data == 0xE0 || data == 0xD0;
}

static Uint8 queuedData[256] = {0xE0};
static int lenOfQueuedData = 0;

static bool sendLightsData(const Uint8* data, size_t len){
    //turns out 0xD0 is escape for E0 and D0, just minus 1 E0 or D0 after escaping

    Uint8 actualData[64] = {0xE0}; //first value is start
    size_t offsetI = 1; //for escaping characters
    for(size_t i = 0; i < len; i++){
        if(needsEscaping(data[i])){
            actualData[i + offsetI] = 0xD0; //escape
            offsetI++;
            actualData[i + offsetI] = data[i] - 1; //data minus one
        } else {
            actualData[i + offsetI] = data[i];
        }
    }

    Uint8 checksum = getChecksum(data, len);
    if(needsEscaping(checksum)){
        actualData[len + offsetI] = 0xD0;
        offsetI++;
        actualData[len + offsetI] = checksum - 1;
    } else {
        actualData[len + offsetI] = checksum;
    }


    for(unsigned int i = 0; i < len + offsetI + 1 &&
                    lenOfQueuedData + i < stackArraySizeInt(queuedData) /*out of bounds protection*/; i++){
        queuedData[lenOfQueuedData + i] = actualData[i];
    }
    lenOfQueuedData += len + offsetI + 1;

    return true;
}

bool setLightForOneButton(Uint8 button, Uint8 r, Uint8 g, Uint8 b){
    if_unlikely(button <= 0 || button > 8){
        return false;
    }
    //printf("%u %u %u\n", r, g, b);

    const Uint8 data[] = {0x11, 0x01, 0x05, 0x31, button - 1, r, g, b};
    return sendLightsData(data, stackArraySize(data));
}

//unused, full combo/all perfect repeatedly calls setLightForMultipleButtons instead
/*bool setFadingLightForMultipleButtons(Uint8 start, Uint8 end, Uint8 r, Uint8 g, Uint8 b, Uint8 speed){
    if_unlikely(start <= 0 || start > 8 || end <= 0 || end > 8){
        return false;
    }
    const Uint8 data[] = {0x11, 0x01, 0x08, 0x33, start - 1, end, 0x00, r, g, b, speed};
    return sendLightsData(data, stackArraySize(data));
}*/

bool setLightForMultipleButtons(Uint8 start, Uint8 end, Uint8 r, Uint8 g, Uint8 b){
    if_unlikely(start <= 0 || start > 8 || end <= 0 || end > 8){
        return false;
    }
    const Uint8 data[] = {0x11, 0x01, 0x08, 0x32, start - 1, end, 0x00, r, g, b, 0x00};
    return sendLightsData(data, stackArraySize(data));
}

/*bool noOperationLight(void){ //no clue what this does
    const Uint8 data[] = {0x11, 0x01, 0x01, 0x10};
    return sendLightsData(data, stackArraySize(data));
}*/

bool refreshLightCommands(void){ //this makes led commands apply immediately
    const Uint8 data[] = {0x11, 0x01, 0x01, 0x3c};
    sendLightsData(data, stackArraySize(data));
    serial_write(serialControllerLights, queuedData, lenOfQueuedData);
    lenOfQueuedData = 0;
    return true;
}

SDL_Color applyFCMedalLightingEffect(int lightFCType, Uint64 timeSinceLastNote){
    const Uint8 value = fadeInFadeOutAlpha(timeSinceLastNote % gameTicksPerSecond, gameTicksPerSecond / 2, gameTicksPerSecond / 2, gameTicksPerSecond) / 2;
    Uint8 r = 0;
    Uint8 g = 0;
    //blue isn't used here
    switch(lightFCType){
        case medalTypeFC:
        case medalTypeFCPlus:
            g = (32 + value) / 1.96386751422;
            break;
        case medalTypeAP:
        case medalTypeAPPlus:
            r = (32 + value);
            g = (40 + value / 2.2) / 1.96386751422;
            break;
        case medalTypeACP:
            r = (32 + value);
            g = (40 + value / 1.15) / 1.96386751422;
            break;
        default:
            break;
    }
    return (SDL_Color){r, g, 0, 255};
}

static SDL_Color oldButtonRGB[8] = {0};
static SDL_Color newButtonRGB[8] = {0};
SDL_Mutex* LEDMutex = NULL;
SDL_Thread* LEDThread = NULL;
static bool isLEDMutexLocked = false;

static int runLEDStuffOnAnotherThread(void* data){
    UNUSED(data);
    int repeatHP = 0;
    bool keepRunningLEDLoop = true;
    while(keepRunningLEDLoop){
        SDL_Delay(15);
        bool hasLEDChanged = false;
        bool sectionsModified[8] = {0};

        SDL_LockMutex(LEDMutex);

        for(int i = 0; i < 8; i++){ //check for equality and apply for stuff
            if(sectionsModified[i] || (compareTwoColors(newButtonRGB[i], oldButtonRGB[i]) && repeatHP <= 0)){
                continue;
            }

            //try to save some bytes and apply error fixing at the same time

            if(repeatHP <= 0){
                repeatHP = 2;
            }
            bool onlyOneSectionModified = true;
            sectionsModified[i] = true;
            int minI = i;
            int maxI = i;
            //I have no idea what i am doing
            for(int n = i + 1; n < 8 && !sectionsModified[n] && (compareTwoColors(newButtonRGB[i], oldButtonRGB[n]) || //if new rgb is same as old rgb
                                                                 compareTwoColors(newButtonRGB[i], newButtonRGB[n]) /* if new rgb is same as current new rgb*/); n++){
                onlyOneSectionModified = false;
                sectionsModified[i] = true;
                maxI = n;
                oldButtonRGB[n] = newButtonRGB[i];
            }

            for(int n = i - 1; n >= 0 && !sectionsModified[n] && (compareTwoColors(newButtonRGB[i], oldButtonRGB[n]) || //if new rgb is same as old rgb
                                                                  compareTwoColors(newButtonRGB[i], newButtonRGB[n]) /* if new rgb is same as current new rgb*/); n--){
                onlyOneSectionModified = false;
                sectionsModified[i] = true;
                minI = n;
                oldButtonRGB[n] = newButtonRGB[i];
            }

            oldButtonRGB[i] = newButtonRGB[i];

            hasLEDChanged = true;
            if(onlyOneSectionModified){
                setLightForOneButton(i + 1, newButtonRGB[i].r, newButtonRGB[i].g, newButtonRGB[i].b);
            } else {
                setLightForMultipleButtons(minI + 1, maxI + 1, newButtonRGB[i].r, newButtonRGB[i].g, newButtonRGB[i].b);
            }

            #if 0
            if(!sectionsModified[i] && (!compareTwoColors(newButtonRGB[i], oldButtonRGB[i]) || repeatHP > 0)){
                //try to save some bytes and apply error fixing at the same time
                if(repeatHP <= 0){
                    repeatHP = 2;
                }
                bool onlyOneSectionModified = true;
                sectionsModified[i] = true;
                int minI = i;
                int maxI = i;
                //I have no idea what i am doing
                for(int n = i + 1; n < 8 && !sectionsModified[n] && (compareTwoColors(newButtonRGB[i], oldButtonRGB[n]) || //if new rgb is same as old rgb
                                                                     compareTwoColors(newButtonRGB[i], newButtonRGB[n]) /* if new rgb is same as current new rgb*/); n++){
                    onlyOneSectionModified = false;
                    sectionsModified[i] = true;
                    maxI = n;
                    oldButtonRGB[n] = newButtonRGB[i];
                }

                for(int n = i - 1; n >= 0 && !sectionsModified[n] && (compareTwoColors(newButtonRGB[i], oldButtonRGB[n]) || //if new rgb is same as old rgb
                                                                      compareTwoColors(newButtonRGB[i], newButtonRGB[n]) /* if new rgb is same as current new rgb*/); n--){
                    onlyOneSectionModified = false;
                    sectionsModified[i] = true;
                    minI = n;
                    oldButtonRGB[n] = newButtonRGB[i];
                }

                oldButtonRGB[i] = newButtonRGB[i];

                hasLEDChanged = true;
                if(onlyOneSectionModified){
                    setLightForOneButton(i + 1, newButtonRGB[i].r, newButtonRGB[i].g, newButtonRGB[i].b);
                } else {
                    setLightForMultipleButtons(minI + 1, maxI + 1, newButtonRGB[i].r, newButtonRGB[i].g, newButtonRGB[i].b);
                }
            }
            #endif
        }

        keepRunningLEDLoop = isUsingSerialLightController;

        SDL_UnlockMutex(LEDMutex);

        if(hasLEDChanged){
            repeatHP--;
            refreshLightCommands();
        }
    }
    return 0;
}

bool setVirtualLightForOneButton(Uint8 button, Uint8 r, Uint8 g, Uint8 b){
    if_unlikely(button <= 0 || button > 8){
        return false;
    }
    //printf("%u %u %u\n", r, g, b);
    if(!isLEDMutexLocked){
        isLEDMutexLocked = true;
        SDL_LockMutex(LEDMutex);
    }

    newButtonRGB[button - 1].r = r;
    newButtonRGB[button - 1].g = g;
    newButtonRGB[button - 1].b = b;
    return true;
}


bool setVirtualLightForMultipleButtons(Uint8 start, Uint8 end, Uint8 r, Uint8 g, Uint8 b){
    if_unlikely(start <= 0 || start > 8 || end <= 0 || end > 8){
        return false;
    }

    for(Uint8 i = start; i <= end; i++){
        setVirtualLightForOneButton(i, r, g, b);
    }

    return true;
}

bool tryToMakeVirtualLightsReal(void){
    if(isLEDMutexLocked){
        isLEDMutexLocked = false;
        SDL_UnlockMutex(LEDMutex);
    }
    return true;
}

bool openSerialControllerLights(const char* serialPort){
    if(!SDL_strnlen(serialPort, 1)){
        return false;
    }

    serialControllerLights = serial_open(serialPort, SERIAL_B115200);

    if_unlikely(serialControllerLights == INVALID_SERIAL){
        printf("Lights controller failed to open!\n");
        return false;
    }

    LEDMutex = SDL_CreateMutex();
    LEDThread = SDL_CreateThread(runLEDStuffOnAnotherThread, "LED", NULL);
    return true;
}
#endif // Circular8_ENABLE_CONTROLLER
