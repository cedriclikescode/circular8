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
#include "miscutils.h"
#include "events.h"

#ifndef SDL_PLATFORM_WINDOWS
#include <errno.h>
#endif // SDL_PLATFORM_WINDOWS
#include <SDL3/SDL.h>

#ifdef Circular8_ENABLE_CONTROLLER
#include "thirdparty/serial/serial.h"
#include "controller/touch.h"

char* touchSerialPort = NULL;
bool isUsingSerialController = false;
SERIAL serialControllerTouchscreen;

//false for no controller
void makeTouchscreenControllerSendData(void){
    printf("RSAT: %d\n", serial_write(serialControllerTouchscreen, "{RSAT}", 6)); //data time
}

bool openSerialControllerTouch(const char* serialPort){
    if(!SDL_strnlen(serialPort, 1)){
        return false;
    }
    serialControllerTouchscreen = serial_open(serialPort, SERIAL_B115200);


    if(serialControllerTouchscreen == INVALID_SERIAL){
        printf("Controller failed to open!\n");
        return false;
    }

    printf("%s\n", serialPort);
    SDL_Delay(200); //write might not be acknowledged right away
    serial_write(serialControllerTouchscreen, "{HALT}", 6); //halt in case it already was sending touch info for some reason and to fix possible issues
    SDL_Delay(100); //ensure the request went through
    makeTouchscreenControllerSendData();

    return true;
}

static void processTouchPacket(const char packet[7]){
    const int numOfBits = 8 - 3; //the last 3 bits for each byte is all 0
    for(int i = 0; i < 7; i++){
        for(int b = 0; b < numOfBits; b++){
            const int location = i * numOfBits + b;
            //worlds craziest memory bug fix (off by one error moment)
            if(location >= 34){ //prevents out of bound access, do not remove
                break;
            }

            const bool newTouch = packet[i] & (1 << b);

            if(newTouch){
                if(!heldTouchSensors[location]){
                    pressedTouchSensors[location] = true;
                }
            } else if(heldTouchSensors[location]){
                releasedTouchSensors[location] = true;
            }

            heldTouchSensors[location] = newTouch;
        }
    }
}

void updateControllerTouchscreen(void){
    static char packet[7] = {0};
    static int packetLen = -1;
    if(!isUsingSerialController){
        return;
    }

    char buffer[2048] = {0};
    const size_t maxSizeToRead = 9 * 227;
    const int numOfBytes = serial_read(serialControllerTouchscreen, buffer, maxSizeToRead);

    if(numOfBytes < 0){
        #ifdef SDL_PLATFORM_WINDOWS
        printf("serial reading error (GetLastError): %d\n", (int)GetLastError());
        #else
        if_unlikely(errno != EAGAIN){
            printf("serial reading error (errno): %d\n", errno);
        }
        #endif // SDL_PLATFORM_WINDOWS
        return;
    }


    if((size_t)numOfBytes >= maxSizeToRead || numOfBytes < 9 || numOfBytes % 9 != 0){
        return;
    }

    for(int i = 0; i < numOfBytes; i++){
        if(packetLen <= -1){
            if_likely(buffer[i] == 0x28){ //character (
                packetLen = 0;
            }
        } else if(packetLen < 7){
            if_unlikely(buffer[i] == 0x29){ //shall never happen, character )
                packetLen = -1;
                printf("invalid ) character in controller packet, corrupted memory?\n");
            } else {
                packet[packetLen] = buffer[i];
                packetLen++;
            }
        } else if_likely(packetLen == 7 && buffer[i] == 0x29){
            processTouchPacket(packet);
            packetLen = -1;
        } else {
            packetLen = -1;
            printf("improper packet data\n");
        }
    }

    /*if((size_t)numOfBytes < maxSizeToRead && numOfBytes >= 9 && numOfBytes % 9 == 0){
        for(int i = 0; i < numOfBytes; i++){
            if(packetLen <= -1){
                if_likely(buffer[i] == 0x28){ //character (
                    packetLen = 0;
                }
            } else if(packetLen < 7){
                if_unlikely(buffer[i] == 0x29){ //shall never happen, character )
                    packetLen = -1;
                    printf("invalid ) character in controller packet, corrupted memory?\n");
                } else {
                    packet[packetLen] = buffer[i];
                    packetLen++;
                }
            } else if_likely(packetLen == 7 && buffer[i] == 0x29){
                processTouchPacket(packet);
                packetLen = -1;
            } else {
                packetLen = -1;
                printf("improper packet data\n");
            }
        }
    } else if(numOfBytes < 0){
        #ifdef SDL_PLATFORM_WINDOWS
        printf("serial reading error (GetLastError): %d\n", (int)GetLastError());
        #else
        if_unlikely(errno != EAGAIN){
            printf("serial reading error (errno): %d\n", errno);
        }
        #endif // SDL_PLATFORM_WINDOWS
    }*/
}
#endif // Circular8_ENABLE_CONTROLLER
