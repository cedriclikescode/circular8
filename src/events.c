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
#include <stdint.h>

#ifdef Circular8_ENABLE_CONTROLLER
#include "thirdparty/serial/serial.h"
#endif // Circular8_ENABLE_CONTROLLER

#include "main.h"
#include "render.h"
#include "events.h"
#include "miscutils.h"
#include "game/chart.h"
#include "game/songselect.h"
#include "game/rendersong.h"
#include "game/options.h"

//for ingame
bool pressedButtons[8] = {0};
bool releasedButtons[8] = {0};
bool heldButtons[8] = {0};

bool realHeldButtons[8] = {0};
Uint64 timestampButtonChange[8] = {0};
bool oppositeHeldButtons[8] = {0};
Uint64 buttonDebounceDelay = 14; //controller moment
bool pressedTouchSensors[8 * 4 + 2] = {0};
bool heldTouchSensors[8 * 4 + 2] = {0};
bool releasedTouchSensors[8 * 4 + 2] = {0};
bool isMouseHeld = false;
bool didMouseMove = false;
bool didMousePressed = false;

int controllerPlayer = 1;

//keyboard mode

static const Uint32 keyboard1P[8] = {
    SDLK_W, SDLK_E, SDLK_D, SDLK_C, SDLK_X, SDLK_Z, SDLK_A, SDLK_Q
};

static const Uint32 keyboard2P[8] = {
    SDLK_KP_8, SDLK_KP_9, SDLK_KP_6, SDLK_KP_3, SDLK_KP_2, SDLK_KP_1, SDLK_KP_4, SDLK_KP_7
};


static SDL_Keycode keyboardButton(int i){
    if(controllerPlayer == 1){
        return keyboard1P[i];
    } else if(controllerPlayer == 2){
        return keyboard2P[i];
    }
    return 99999;
}

//regular touch screen stuff
SDL_Finger touchFingers[32] = {0};
int numberOfTouchFingers = 0;
bool hasTouchscreenUpdated = false;

void updateTouchscreen(void){
    hasTouchscreenUpdated = false;
    bool newTouchSensors[8 * 4 + 2] = {0};
    bool newButtons[8] = {0};
    for(int i = 0; i < numberOfTouchFingers; i++){
        const float fingerSize = screenSize / 32.f; //or 24.f?
        const SDL_FPoint offsets[9] = {
            {0.f, 0.f}, //center
            {0.f, -fingerSize}, //top
            {fingerSize * 0.70710f, -fingerSize * 0.70710f}, //top right
            {fingerSize, 0.f}, //right
            {fingerSize * 0.70710f, fingerSize * 0.70710f}, //bottom right
            {0.f, fingerSize}, //bottom
            {-fingerSize * 0.70710f, fingerSize * 0.70710f}, //bottom left
            {-fingerSize, 0.f}, //left
            {-fingerSize * 0.70710f, -fingerSize * 0.70710f} //top left
        };

        bool touchCounter[34] = {0};

        for(int n = 0; n < stackArraySizeInt(offsets); n++){
            const SDL_FPoint touchFPoint = {(touchFingers[i].x - screenXOffset + offsets[n].x) / screenSize, (touchFingers[i].y - screenYOffset + offsets[n].y) / screenSize};
            const int button = getTouchSensorAtFPoint(touchFPoint);
            if(button < 0 || touchCounter[button]){
                continue;
            }

            touchCounter[button] = true;

            if(newTouchSensors[button] && button < 8){ //apply button
                newButtons[button] = true;
            } else {
                newTouchSensors[button] = true;
            }
        }
    }

    //todo: debounce for touch? (buttons supported)
    for(int i = 0; i < stackArraySizeInt(newButtons); i++){
        realHeldButtons[i] = newButtons[i];
    }

    for(int i = 0; i < stackArraySizeInt(newTouchSensors); i++){
        if(newTouchSensors[i]){
            if(!heldTouchSensors[i]){
                pressedTouchSensors[i] = true;
            }
        } else if(heldTouchSensors[i]){
            releasedTouchSensors[i] = true;
        }

        heldTouchSensors[i] = newTouchSensors[i];
    }
}

SDL_FPoint getMousePosition(void){
    SDL_FPoint mousePos = {mouseX, mouseY};
    return mousePos;
}

bool isButtonOrTouchPressed(int button){
    return (pressedButtons[button - 1] && !heldTouchSensors[button - 1]) || pressedTouchSensors[button - 1];
}

bool isButtonAndTouchReleased(int button){
    return (releasedButtons[button - 1] || releasedTouchSensors[button - 1]) && !heldTouchSensors[button - 1];
}

void updateNormalButtons(Uint64 timestamp){
    for(int button = 0; button < 8; button++){
        if(timestamp >= timestampButtonChange[button] + buttonDebounceDelay && realHeldButtons[button] != heldButtons[button]){
            heldButtons[button] = realHeldButtons[button];
            //oppositeHeldButtons = !realHeldButtons[button];
            timestampButtonChange[button] = timestamp + buttonDebounceDelay;

            if(heldButtons[button]){
                pressedButtons[button] = true;
            } else {
                releasedButtons[button] = true;
            }
        }
    }
}

void resetButtons(void){
    for(int button = 0; button < 8; button++){
        pressedButtons[button] = false;
        releasedButtons[button] = false;
    }

    for(int sensor = 0; sensor < 34; sensor++){
        pressedTouchSensors[sensor] = false;
        releasedTouchSensors[sensor] = false;
    }
}

bool getMousePressedOnce(void){
    if(didMousePressed){
        didMousePressed = false;
        return true;
    }
    return didMousePressed;
}

void updateScaling(void){
    SDL_SyncWindow(window); //fixes some issues?
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    const int newWidth = isScreenRotatingInPortrait() ? h : w;
    const int newHeight =  isScreenRotatingInPortrait() ? w : h;
    const int smallestSize = SDL_min(newWidth, newHeight);
    const int biggestSize = SDL_max(newWidth, newHeight);
    if(!useSubscreen){
        SDL_SetRenderLogicalPresentation(renderer, smallestSize, smallestSize, newWidth == newHeight ?
                                                                                SDL_LOGICAL_PRESENTATION_DISABLED : //same size on both size, performance increased?
                                                                                SDL_LOGICAL_PRESENTATION_LETTERBOX); //might be a bit slowish
    }

    screenWidth = smallestSize;
    screenHeight = useSubscreen ? biggestSize : smallestSize;
    screenMainHeight = smallestSize;
    guiScale = SDL_min(smallestSize / (float)guiWidth, smallestSize / (float)guiHeight);
}

static void liftTouchFinger(SDL_FingerID fingerID){
    hasTouchscreenUpdated = true;
    for(int i = 0; i < numberOfTouchFingers; i++){
        if(touchFingers[i].id == fingerID){
            touchFingers[i] = touchFingers[numberOfTouchFingers - 1];
            numberOfTouchFingers = SDL_max(numberOfTouchFingers - 1, 0);
            break;
        }
    }
}

static void putTouchFinger(SDL_Event *event){
    if(numberOfTouchFingers < stackArraySizeInt(touchFingers)){
        hasTouchscreenUpdated = true;
        SDL_ConvertEventToRenderCoordinates(renderer, event);
        touchFingers[numberOfTouchFingers].id = event->tfinger.fingerID;
        touchFingers[numberOfTouchFingers].x = event->tfinger.x;
        touchFingers[numberOfTouchFingers].y = event->tfinger.y;
        numberOfTouchFingers++;
    }
}

static void moveTouchFinger(SDL_Event *event){
    hasTouchscreenUpdated = true;
    SDL_ConvertEventToRenderCoordinates(renderer, event);
    for(int i = 0; i < numberOfTouchFingers; i++){
        if(touchFingers[i].id != event->tfinger.fingerID){
            touchFingers[i].x = event->tfinger.x;
            touchFingers[i].y = event->tfinger.y;
            break;
        }
    }
}

static void applyKeyToButton(SDL_Keycode key, bool value){
    for(int button = 0; button < 8; button++){
        if(key == keyboardButton(button)){
            realHeldButtons[button] = value;
            break;
        }
    }
}

SDL_AppResult handleEvent(SDL_Event *event){
    bool justPressed = false;
    if(event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT){
        didMousePressed = true;
        isMouseHeld = true;
        justPressed = true;
    }

    if(justPressed || event->type == SDL_EVENT_MOUSE_MOTION){
        SDL_ConvertEventToRenderCoordinates(renderer, event);
        mouseX = event->motion.x;
        mouseY = event->motion.y;

        switch(rotateScreenBy){
            case 90:
                mouseX = screenWidth - mouseX;
                break;
            case 180:
                mouseX = screenWidth - mouseX;
                mouseY = screenHeight - mouseY - (screenHeight - screenMainHeight);
                break;
            case 270:
                mouseX -= (screenHeight - screenMainHeight);
                mouseY = screenWidth - mouseY;
                break;
            case 0:
            default:
                break;
        }

        if(isScreenRotatingInPortrait()){
            const int x = mouseX;
            mouseX = mouseY;
            mouseY = x;
        }

        if(!isMouseHeld){
            return SDL_APP_CONTINUE;
        }

        const SDL_FPoint mouseFPoint = {(mouseX - screenXOffset) / screenSize, (mouseY - screenYOffset) / screenSize};
        const int button = getTouchSensorAtFPoint(mouseFPoint);

        for(int i = 0; i < 8 * 4 + 2; i++){
            if(button == i){
                if(!heldTouchSensors[i]){
                    heldTouchSensors[i] = true;
                    pressedTouchSensors[i] = true;
                }
            } else if(heldTouchSensors[i]){
                heldTouchSensors[i] = false;
                releasedTouchSensors[i] = true;
            }
        }
        return SDL_APP_CONTINUE;
    }

    switch(event->type){
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            updateScaling();
            break;
        case SDL_EVENT_FINGER_DOWN:
            putTouchFinger(event);
            break;
        case SDL_EVENT_FINGER_MOTION:
            moveTouchFinger(event);
            break;
        case SDL_EVENT_FINGER_UP:
            liftTouchFinger(event->tfinger.fingerID);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if(event->button.button != SDL_BUTTON_LEFT){
                break;
            }

            for(int i = 0; i < 8 * 4 + 2; i++){
                if(heldTouchSensors[i]){
                    heldTouchSensors[i] = false;
                    releasedTouchSensors[i] = true;
                }
            }
            isMouseHeld = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            if(event->key.repeat){
                break;
            }

            switch(event->key.key){
                case SDLK_F11:
                    SDL_SetWindowFullscreen(window, (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != SDL_WINDOW_FULLSCREEN);
                    break;
                default:
                    applyKeyToButton(event->key.key, true);
                    break;
            }
            break;
        case SDL_EVENT_KEY_UP:
            if(event->key.repeat){
                break;
            }

            applyKeyToButton(event->key.key, false);
            break;
        default:
            break;
    }

    return SDL_APP_CONTINUE;
}
