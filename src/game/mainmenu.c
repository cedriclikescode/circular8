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

#include <SDL3/SDL.h>
#include "main.h"
#include "events.h"
#include "gameinit.h"
#include "load.h"
#include "render.h"
#include "texture.h"
#include "game/ingame.h"
#include "game/mainmenu.h"
#include "game/profileselect.h"

#ifdef Circular8_ENABLE_CONTROLLER
#include "game/chart.h"
#include "controller/touch.h"
#endif // Circular8_ENABLE_CONTROLLER

SDL_Texture* mainMenuTitle = NULL;
SDL_Texture* mainMenuStartButton = NULL;
SDL_Texture* buildDate = NULL;
SDL_Texture* SDLVersions[4] = {0};

bool hoveringOnStartButton = false;
SDL_FRect startButtonFRect = {0};

static int renderMainmenu(void){
    //main menu button
    renderTextureSimpler(mainMenuTitle, getCenterX(), 96.f * guiScale);
    renderTextureSimpler(gameVersion, getCenterX(), 160.f * guiScale);
    renderTextureSimpler(buildDate, getCenterX(), screenMainHeight - 224.f * guiScale);
    for(int i = 0; i < 4; i++){
        renderTextureSimpler(SDLVersions[i], getCenterX(), screenMainHeight - (180.f - 30.f * i) * guiScale);
    }
    if(hoveringOnStartButton){
        setRenderDrawColor(128 + 64, 0, 0, SDL_ALPHA_OPAQUE / 2);
        renderFillRectF(&startButtonFRect);
    }
    renderTexture(mainMenuStartButton, NULL, &startButtonFRect);
    return 0;
}

void runProfileLoadingHelper(void){
    for(int i = 0; i < numberOfProfiles; i++){
        destroyTextureIfNotNull(profiles[i].profileIcon);
        destroyTextureIfNotNull(profiles[i].nameTexture);
        destroyTextureIfNotNull(profiles[i].ratingTexture);
        SDL_free(profiles[i].name);
        SDL_free(profiles[i].title);
        SDL_free(profiles[i].profileLocation);
        SDL_free(profiles[i].fullPath);
    }

    if(numberOfProfiles > 0){
        numberOfProfiles = 0;
        SDL_free(profiles);
        profiles = NULL;
    }
    SDL_EnumerateDirectory("profiles", loadProfilesCallback, NULL);
    if(numberOfProfiles > 0){
        SDL_qsort(profiles, numberOfProfiles, sizeof(profiles[0]), profileSortingCompare);
        runLoadFunction(loadProfileSelect, profileSelectSlot);
    } else {
        runLoadFunction(loadSongSelect, songSelectSlot);
    }
}

Uint64 mainMenu(Uint64 delta) {
    static Uint64 timeInMainMenu = 0;
    timeInMainMenu += delta;
    renderFunction = renderMainmenu;
    const SDL_FPoint mousePos = getMousePosition();
    startButtonFRect = FRectWithTexSimple(mainMenuStartButton, getCenterX(), getCenterY(), 1.f, true);
    hoveringOnStartButton = SDL_PointInRectFloat(&mousePos, &startButtonFRect);
    #ifdef Circular8_ENABLE_CONTROLLER
    const bool isCSensorTouched = isUsingSerialController && (pressedTouchSensors[getSensor('C', 1)] || releasedTouchSensors[getSensor('C', 2)]);
    #else
    const bool isCSensorTouched = false;
    #endif // Circular8_ENABLE_CONTROLLER

    if(timeInMainMenu < gameTicksPerSecond || !((getMousePressedOnce() && hoveringOnStartButton) || isCSensorTouched)){
        return 0;
    }

    if(isCSensorTouched){
        SDL_HideCursor();
    }
    //hack
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");

    timeInMainMenu = 0;
    destroyTextureIfNotNull(mainMenuTitle);
    destroyTextureIfNotNull(mainMenuStartButton);
    destroyStackArrayOfTextures(SDLVersions);

    runProfileLoadingHelper();
    return 0;
}
