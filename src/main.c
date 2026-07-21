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

//Standard C libraries
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>

//External libraries
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_platform_defines.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#ifdef Circular8_ENABLE_CONTROLLER
#include "thirdparty/serial/serial.h"
#include "controller/touch.h"
#include "controller/led.h"
#endif // Circular8_ENABLE_CONTROLLER

#ifdef Circular8_ENABLE_FFmpeg
#include "video.h"
#endif // Circular8_ENABLE_FFmpeg

//Project headers
#include "main.h"
#include "render.h"
#include "miscutils.h"
#include "load.h"

#include "gameinit.h"
#include "events.h"
#include "texture.h"
#include "language.h"
#include "game/rendersong.h"
#include "game/options.h"
#include "game/chart.h"


SDL_Window* window;

int mouseX, mouseY;

bool didCursorSet = false;
SDL_Cursor* arrowCursor;
SDL_Cursor* handCursor;

bool reRunGameFunction = false;
int rendererVSync = 1;

Uint8 globalBrightness = 255;

//sound effects
//MIX_Audio* buttonHoverSfx;

//useless for now
/*int getRandom(int max) {
    return rand() % max;
}*/

TTF_Font* bigFont = NULL;
TTF_Font* mediumFont = NULL;
TTF_Font* smallFont = NULL;

SDL_Texture* numbersText;
SDL_FRect numbersTextFRects[15] = {0};
char numberChars[] = "0123456789%+-.,";



void loadAllText(void) {
    //unsigned int textTime = SDL_GetTicks();
    //font loading
    const char* fontLocation = "fonts/NotoSansJP/NotoSansJP-Regular.ttf";

    TTF_CloseFont(bigFont);
    TTF_CloseFont(mediumFont);
    TTF_CloseFont(smallFont);

    bigFont = TTF_OpenFont(fontLocation, bigPtSize * guiScale);
    mediumFont = TTF_OpenFont(fontLocation, mediumPtSize * guiScale);
    smallFont = TTF_OpenFont(fontLocation, smallPtSize * guiScale);

    //text time
    loadTextSimple(&gameVersion, mediumFont, "V0.0.0", white, false);

    numbersText = loadText(mediumFont, numberChars, white);
    int x = 0;
    int y = 0;
    for(unsigned int i = 0; i < stackArraySize(numbersTextFRects); i++){
        char singleChar[4] = {0};
        simple_snprintf(singleChar, "%c", numberChars[i]);
        int w, h;
        TTF_GetStringSize(mediumFont, singleChar, 0, &w, &h);
        numbersTextFRects[i].x = x;
        numbersTextFRects[i].y = y;
        numbersTextFRects[i].w = w;
        numbersTextFRects[i].h = h;
        x += w;

    }
    //printf("WIDTH %d\n", x);
}

static int loadAllEarlyGuiItems(void) {
    runLoadFunction(loadMainMenuGui, mainMenuSlot);
    return 0;
}

static SDL_Texture* mainViewPort = NULL;
static SDL_Texture* subscreenBackground = NULL;
SDL_Texture* gameVersion = NULL;

static SDL_Texture* customScene = NULL;

static int loadEssentialImages(void) {
    char viewPortString[256] = {0};

    //todo: find way to keep benefit of 25% memory usage of viewport while odd screen res
    const bool screenResIsOdd = screenWidth % 2 == 1;
    const int newWidth = screenWidth / (1 + !screenResIsOdd);
    const int newHeight = screenMainHeight / (1 + !screenResIsOdd);
    simple_snprintf(viewPortString, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %.6f %.6f\"><path d=\"M1 2a1 1 0 0 0 0-2h1v2H0V0h1a1 1 0 0 0 0 2Z\"/></svg>",
                    newWidth, newHeight, 1.0 + screenResIsOdd, 1.0 + screenResIsOdd);
    loadRawSVGSimple(&mainViewPort, viewPortString, wh(newWidth, newHeight));

    if(useSubscreen){
        loadImageSimple(&subscreenBackground, "gfx/subscreenbg.jpg", wh(screenWidth, subGuiHeight * guiScale));
    }

    if(rotateScreenBy != 0){
        int width = screenWidth;
        int height = useSubscreen ? screenHeight : screenMainHeight;
        if(!isScreenRotatingInPortrait()){
            width = screenWidth;
            height = useSubscreen ? screenHeight : screenMainHeight;
            //width = screenHeight;
            //height = screenWidth;
            //screenWidth = width;
            //screenHeight = height;
        }

        destroyTextureIfNotNull(customScene);
        customScene = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, width, height);
    }
    return 0;
}

MIX_Mixer* mainMixer = NULL;

static int preLoadAllSounds(void) {
    Uint64 audioLoadTime = SDL_GetTicks();


    if_unlikely(!MIX_Init()) {
        return setError("Mix_Init has failed.\nError: %s\n", SDL_GetError());
    }

    Uint64 audioTimeStart = SDL_GetTicks();

    mainMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if_unlikely(mainMixer == NULL){
        return setError("MIX_CreateMixerDevice has failed.\nError: %s\n", SDL_GetError());
    }
    printf("start: %u time to load audio device: %u\n", (unsigned int)audioTimeStart, (unsigned int)(SDL_GetTicks() - audioTimeStart));

    MIX_SetMixerGain(mainMixer, machineMasterVolume);

    printf("audio load time: %u\n", (unsigned int)(SDL_GetTicks() - audioLoadTime));
    return 0;
}

int (*renderFunction)(void) = {0};
int (*subscreenRenderFunction)(void) = {0};
void (*reloadFunction)(int) = {0};
Uint64 (*functionPointers[16])(Uint64) = {0};
int currentSlot = 0;

void addGameFunctionToCurrentSlot(Uint64 (*gameFunction)(Uint64 delta)){
    functionPointers[currentSlot] = gameFunction;
}

char* basePathLocation = NULL;
static bool isUsingSoftwareRenderer = false;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv){
    UNUSED(appstate);
    SDL_SetHint(SDL_HINT_TIMER_RESOLUTION, "0"); //don't tamper with time resolution
    loadOptionsINI("options");

    rendererVSync = optionVSync;


    generateTouchSensors(298); //make touch sensors work properly

    if_unlikely(!SDL_Init(SDL_INIT_VIDEO)) {
        setError("An error occurred while initializing video.\nError: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if_unlikely(!SDL_Init(SDL_INIT_AUDIO)) {
        setError("An error occurred while initializing audio.\nError: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    //basePathLocation = SDL_GetBasePath();
    if(basePathLocation == NULL){
        basePathLocation = calloc(1, sizeof(char));
    }

    loadLangTexts(currentLocaleName == NULL ? "en-US" : currentLocaleName);

    SDL_PropertiesID props = SDL_CreateProperties();
    if_unlikely(props == 0){
        SDL_Log("Unable to create properties: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, NULL);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    //this is broken on x11
    /*if(fullscreenMode == 1){
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
    }*/
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, windowWidth);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, windowHeight);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);

    window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    if_unlikely(window == NULL) {
        setError("An error occurred while initializing creating a window.\nError: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    int renderI = rendererThing(argc, argv);
    if(renderI >= 0){
        printf("renderI: %d %s\n", renderI, SDL_GetRenderDriver(renderI));
    } else {
        puts("render driver automatically chosen.\n");
    }


    renderer = SDL_CreateRenderer(window, renderI == -1 ? NULL : SDL_GetRenderDriver(renderI));//initializeRenderer(window);
    if_unlikely(renderer == NULL) {
        printf("Render creation failed: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    isUsingSoftwareRenderer = SDL_strcmp(SDL_GetRendererName(renderer), SDL_SOFTWARE_RENDERER) == 0; //todo: should it be case insensitive?

    if(fullscreenMode == 2){
        int count = 0;
        SDL_DisplayMode** listOfModes = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(window), &count);
        if_unlikely(count <= 0){ //???
            printf("listOfModes count is 0: %s\n", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        //special case
        if(exclusiveWidth == 0 && exclusiveHeight == 0 && exclusiveHz == 0){
            SDL_SetWindowFullscreenMode(window, listOfModes[0]);
            SDL_SetWindowFullscreen(window, true);
        } else {
            for(int i = 0; i < count; i++){
                if((exclusiveWidth == 0 || listOfModes[i]->w == exclusiveWidth) &&
                   (exclusiveHeight == 0 || listOfModes[i]->h == exclusiveHeight) &&
                   (exclusiveHz == 0 || SDL_fabs(exclusiveHz - listOfModes[i]->refresh_rate) < 0.1)){
                    SDL_SetWindowFullscreenMode(window, listOfModes[i]);
                    SDL_SetWindowFullscreen(window, true);
                    break;
                   }
            }
        }

        SDL_free(listOfModes);
    } else if(fullscreenMode == 1){
        SDL_SetWindowFullscreen(window, true);
    }

    SDL_SyncWindow(window);

    if_unlikely(!SDL_GetWindowSizeInPixels(window, &screenWidth, &screenHeight)){
        printf("SDL_GetWindowSizeInPixels errored: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    updateScaling();

    screenMainHeight = SDL_min(screenWidth, screenHeight); //fix possible issues

    //print all available texture formats
    SDL_PropertiesID renderProperties = SDL_GetRendererProperties(renderer);
    const Uint32 *supportedFormats = SDL_GetPointerProperty(renderProperties, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, NULL);
    maxTextureSize = SDL_GetNumberProperty(renderProperties, SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 2048);

    #ifdef Circular8_ENABLE_FFmpeg
    bool supportsYUV = false;
    #endif // Circular8_ENABLE_FFmpeg

    for(int i = 0; supportedFormats[i] != SDL_PIXELFORMAT_UNKNOWN; i++){
        #ifdef Circular8_ENABLE_FFmpeg
        if(supportedFormats[i] == SDL_PIXELFORMAT_IYUV){
            supportsYUV = true;
        }
        #endif // Circular8_ENABLE_FFmpeg
        printf("Supported texture format %d: %s\n", i, SDL_GetPixelFormatName(supportedFormats[i]));
    }

    #ifdef Circular8_ENABLE_FFmpeg
    if(isVideoEnabled && !supportsYUV){
        isVideoEnabled = false; //render doesn't support SDL_PIXELFORMAT_IYUV, disable video as hack
    }
    #endif // Circular8_ENABLE_FFmpeg

    SDL_DestroyProperties(renderProperties);

    loadEssentialImages();
    preLoadAllSounds();

    //loads all texts (must be ran after creating renderer or else error)
    TTF_Init();
    loadAllText();

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    loadAllEarlyGuiItems();

    SDL_RenderClear(renderer);

    SDL_SetWindowTitle(window, "Circular8");

    arrowCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);

    //disable some events for some performance gains
    SDL_SetEventEnabled(SDL_EVENT_TEXT_INPUT, false); //performance go brr
    SDL_SetEventEnabled(SDL_EVENT_WINDOW_FOCUS_LOST, false);
    SDL_SetEventEnabled(SDL_EVENT_WINDOW_MOUSE_LEAVE, false);
    SDL_SetEventEnabled(SDL_EVENT_MOUSE_WHEEL, false); //revisit?

    #ifdef Circular8_ENABLE_CONTROLLER
    if(isUsingSerialController){
        #ifdef SDL_PLATFORM_WINDOWS
        const char defaultSerialPort[] = "COM3";
        const char defaultLightPort[] = "COM21";
        #else
        const char defaultSerialPort[] = "/dev/ttyACM0";
        const char defaultLightPort[] = "/dev/ttyUSB0";
        #endif // SDL_PLATFORM_WINDOWS
        if(!enableCustomControllerPorts){
            touchSerialPort = SDL_strdup(defaultSerialPort);
            lightsSerialPort = SDL_strdup(defaultLightPort);
        } else {
            if(touchSerialPort == NULL){
                touchSerialPort = SDL_strdup(defaultSerialPort);
            }

            if(lightsSerialPort == NULL){
                lightsSerialPort = SDL_strdup(defaultLightPort);
            }
        }

        isUsingSerialController = openSerialControllerTouch(touchSerialPort);
        if(isUsingSerialController && isUsingSerialLightController){
            isUsingSerialLightController = openSerialControllerLights(lightsSerialPort);
        }

        SDL_free(touchSerialPort);
        SDL_free(lightsSerialPort);
    }
    #endif // Circular8_ENABLE_CONTROLLER
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate){
    UNUSED(appstate);
    static bool didSlotChange = false; //hack
    static bool hasStaticVarsInitialized = false;
    static Uint64 newTicks, oldTicks;
    static float oldGuiscale;

    newTicks = SDL_GetTicksNS();
    if_unlikely(!hasStaticVarsInitialized){
        oldTicks = newTicks;
        oldGuiscale = guiScale;
        hasStaticVarsInitialized = true;
    }

    #ifdef Circular8_ENABLE_CONTROLLER
    updateControllerTouchscreen();
    #endif // Circular8_ENABLE_CONTROLLER



    if(hasTouchscreenUpdated){
        updateTouchscreen();
    }
    updateNormalButtons(newTicks);

    didMouseMove = false;
    didCursorSet = false;

    bool updateImages = false;
    restartUpdate:;

    if(oldGuiscale != guiScale){
        oldGuiscale = guiScale;
        updateImages = true;
        SDL_Delay(20);
        goto restartUpdate;
    }

    if(updateImages){
        oldGuiscale = guiScale;
        loadAllText();
        isLoadImagePart = true;
        loadEssentialImages(); //real moment
        reloadFunction(currentSlot);

        runLoadOnly(currentSlot, true);
        updateImages = oldGuiscale != guiScale; //fixing bugs moment
        isLoadImagePart = false;
        goto restartUpdate;
    }

    const int oldSlot = currentSlot;

    renderFunction = NULL;
    subscreenRenderFunction = NULL;
    Uint64 timeSinceLastFrame = didSlotChange ? 0 : newTicks - oldTicks;
    didSlotChange = false;
    do {
        reRunGameFunction = false;
        timeSinceLastFrame = (*functionPointers[currentSlot])(timeSinceLastFrame); //todo: use return value as remaining time
        //printf("value: %d\n", returnValue);
        if(currentSlot != oldSlot) {
            didSlotChange = true;
            goto restartUpdate; //cppcheck is wrong
        }
    } while(reRunGameFunction);

    //benchmark only
    /*do {
        reRunGameFunction = false;
        for(int i = 0; i < 200000; i++){
            (*functionPointers[currentSlot])(timeSinceLastFrame * (i == 0)); //todo: use return value as remaining time
        }
        //printf("value: %d\n", returnValue);
        if(currentSlot != oldSlot) {
            goto restartUpdate; //cppcheck is wrong
        }
    } while(reRunGameFunction);*/


    if(customScene != NULL){
        SDL_SetRenderTarget(renderer, customScene);
    }
    setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    float texW = 0.f;
    float texH = 0.f;

    if(useSubscreen && SDL_GetTextureSize(subscreenBackground, &texW, &texH)){
        const SDL_FRect subscreenBGFrect = xywh(screenXOffset, 0, screenSize, subGuiHeight * guiScale);
        SDL_SetRenderViewport(renderer, NULL);
        renderTexture(subscreenBackground, NULL, &subscreenBGFrect);
        if(SDL_GetTextureSize(gameVersion, &texW, &texH)){
            SDL_FRect gameVersionFRect = xywh(screenXOffset + screenWidth - texW - 40.f * guiScale, 25.f * guiScale, texW, texH);
            changeFRectSizeBy(4.f * guiScale, &gameVersionFRect);
            setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE);
            renderFillRectF(&gameVersionFRect);
            changeFRectSizeBy(-4.f * guiScale, &gameVersionFRect);
            renderTexture(gameVersion, NULL, &gameVersionFRect);
        }

        if(subscreenRenderFunction != NULL) {
            subscreenRenderFunction();
        }

        const SDL_Rect mainViewport = ((SDL_Rect){0, screenHeight - screenWidth, screenWidth, screenMainHeight});
        SDL_SetRenderViewport(renderer, &mainViewport);
    }

    if(renderFunction != NULL) {
        renderFunction();
    }

    if(SDL_GetTextureSize(mainViewPort, &texW, &texH)){
        if(screenWidth % 2 == 0){
            if(isUsingSoftwareRenderer){ //faster to render without triangles in software rendering mode
                SDL_FRect borderFRect = {screenXOffset, useSubscreen ? 0 : screenYOffset, screenSize / 2, screenSize / 2};
                renderTexture(mainViewPort, NULL, &borderFRect);
                borderFRect.x += borderFRect.w;
                renderTextureRotated(mainViewPort, NULL, &borderFRect, 0, NULL, SDL_FLIP_HORIZONTAL);
                borderFRect.y += borderFRect.h;
                renderTextureRotated(mainViewPort, NULL, &borderFRect, 0, NULL, SDL_FLIP_VERTICAL | SDL_FLIP_HORIZONTAL);
                borderFRect.x -= borderFRect.w;
                renderTextureRotated(mainViewPort, NULL, &borderFRect, 0, NULL, SDL_FLIP_VERTICAL);
            } else { //skip rendering transparent parts of viewport to get faster performance
                SDL_Vertex viewPortVertices[12] = {0};
                for(int i = 0; i < stackArraySizeInt(viewPortVertices); i++){
                    viewPortVertices[i].color.r = 1.f;
                    viewPortVertices[i].color.g = 1.f;
                    viewPortVertices[i].color.b = 1.f;
                    viewPortVertices[i].color.a = 1.f;
                }

                #define viewPortVertexSet(index, xPos, yPos, xTex, yTex) do {\
                    viewPortVertices[index].position.x = (xPos);\
                    viewPortVertices[index].position.y = (yPos);\
                    viewPortVertices[index].tex_coord.x = (xTex);\
                    viewPortVertices[index].tex_coord.y = (yTex);\
                } while(0)

                //top left
                viewPortVertexSet(0, 0.f, 0.f, 0.f, 0.f);
                viewPortVertexSet(1, screenSize / 2.f, 0.f, 1.f, 0.f);
                viewPortVertexSet(2, 0.f, screenSize / 2.f, 0.f, 1.f);

                //top right
                viewPortVertexSet(3, screenSize, 0.f, 0.f, 0.f);
                viewPortVertexSet(4, screenSize / 2.f, 0.f, 1.f, 0.f);
                viewPortVertexSet(5, screenSize, screenSize / 2.f, 0.f, 1.f);

                //bottom left
                viewPortVertexSet(6, 0.f, screenSize, 0.f, 0.f);
                viewPortVertexSet(7, screenSize / 2.f, screenSize, 1.f, 0.f);
                viewPortVertexSet(8, 0.f, screenSize / 2.f, 0.f, 1.f);

                //bottom right
                viewPortVertexSet(9, screenSize, screenSize, 0.f, 0.f);
                viewPortVertexSet(10, screenSize / 2.f, screenSize, 1.f, 0.f);
                viewPortVertexSet(11, screenSize, screenSize / 2.f, 0.f, 1.f);
                #undef viewPortVertexSet
                SDL_RenderGeometry(renderer, mainViewPort, viewPortVertices, 12, NULL, 0);
            }
        } else {
            renderTexture(mainViewPort, NULL, NULL);
        }
    }

    if(globalBrightness < SDL_ALPHA_OPAQUE) {
        setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE - globalBrightness);
        renderFillRect(NULL);
    }


    static bool oldDidCursorSet = true;
    if_unlikely(oldDidCursorSet != didCursorSet) {
        oldDidCursorSet = didCursorSet;
        if(didCursorSet) {
            SDL_SetCursor(handCursor);
        } else {
            SDL_SetCursor(arrowCursor);
        }
    }

    static int oldRendererVSync = false;
    if_unlikely(oldRendererVSync != rendererVSync) {
        oldRendererVSync = rendererVSync;
        if_unlikely(!SDL_SetRenderVSync(renderer, rendererVSync)) {
            printf("Setting VSync state failed.\nError:%s\n", SDL_GetError());
        }
    }

    if(customScene != NULL){
        SDL_SetRenderTarget(renderer, NULL);
        const SDL_FRect screenFRect = xywh(rotateScreenBy == 180 ? 0 : SDL_abs(screenWidth - screenHeight) / 2,
                                           rotateScreenBy == 180 ? 0 : -SDL_abs((screenWidth - screenHeight) / 2),
                                           screenWidth, screenHeight);
        setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_GetTextureSize(customScene, &texW, &texH);

        renderTextureRotated(customScene, NULL, &screenFRect, rotateScreenBy, NULL, SDL_FLIP_NONE);
    }

    SDL_RenderPresent(renderer);
    oldTicks = newTicks;
    resetButtons();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
    UNUSED(appstate);
    return handleEvent(event);
}

void SDL_AppQuit(void *appstate, SDL_AppResult result){
    UNUSED(appstate);
    UNUSED(result);

    #ifdef Circular8_ENABLE_FFmpeg
    endVideoThread();
    #endif // Circular8_ENABLE_FFmpeg

    #ifdef Circular8_ENABLE_CONTROLLER
    if(isUsingSerialController){
        serial_write(serialControllerTouchscreen, "{HALT}", 6);
        SDL_LockMutex(LEDMutex);
        if(isUsingSerialLightController){
            isUsingSerialLightController = false;
            SDL_UnlockMutex(LEDMutex);
            SDL_WaitThread(LEDThread, NULL);
            serial_close(serialControllerLights);
            SDL_DestroyMutex(LEDMutex);
        }
        SDL_Delay(200);
        serial_close(serialControllerTouchscreen);
    }
    #endif // Circular8_ENABLE_CONTROLLER

    TTF_CloseFont(bigFont);
    TTF_CloseFont(mediumFont);
    TTF_CloseFont(smallFont);
    TTF_Quit();
    MIX_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
