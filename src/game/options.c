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

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "thirdparty/inih/ini.h"

#include "main.h"
#include "gameinit.h"
#include "load.h"
#include "events.h"
#include "render.h"
#include "miscutils.h"
#include "options.h"
#include "language.h"

#ifdef Circular8_ENABLE_CONTROLLER
#include "controller/touch.h"
#include "controller/led.h"
bool enableCustomControllerPorts = false;
#endif // Circular8_ENABLE_CONTROLLER

#ifdef Circular8_ENABLE_FFmpeg
#include "video.h"
#endif // Circular8_ENABLE_FFmpeg

#include "game/songselect.h"
#include "game/ingame.h"
#include "game/profileselect.h"
#include "game/scorescreen.h"

SDL_Texture* groupOptions[4] = {0};

SDL_Texture* gameplayOptions[5] = {0};
SDL_Texture* displayOptions[4] = {0};
SDL_Texture* judgeOptions[4] = {0};
SDL_Texture* audioOptions[5] = {0};

SDL_Texture* changeMinus = NULL;
SDL_Texture* changePlus = NULL;

static const int changeMinusI = 7;
static const int changePlusI = 2;

static bool insideGroupOption = false;
int groupOptionSelected = 0;
static int gameOptionSelected = 0;

SDL_Texture* optionOffOnTexts[2] = {0};

//gameplay
double tapSpeed = 2.0;
double touchSpeed = 2.0;
bool rotateStarNote = true;
int mirrorType = 0;
SDL_Texture* mirrorTexts[3] = {0};
int trackSkip = 0;
SDL_Texture* trackSkipTexts[3] = {0};

//display
int displayType = achievementCombo;
double backgroundBrightness = 0.5;
int showSensors = false;
SDL_Texture* sensorTypes[3] = {0};
bool useAlternateStarNote = false;
SDL_Texture* slideTapNoteNames[2] = {0};

//judge
int showFastLate = false;
SDL_Texture* fastLateTypes[3] = {0};
int showCriticalPerfect = false;
SDL_Texture* criticalPerfectTypes[2] = {0};
SDL_Texture* judgeDisplaySpeedTypes[5] = {0};
double chartOffset60fps = 0.0;
double inputOffset60fps = 0.0;
Sint64 realChartOffset = 0;
Sint64 realInputOffset = 0;


//audio
double gameplaySfxVolume = 0.7;
double userMusicVolume = 0.8;
bool enableFireworksSFX = true;
bool enableSlideSFX = true;
double guideOffset60fps = 0.0;
Sint64 realGuideOffset = 0;

bool useSubscreen = false;
SDL_Texture* achievementTexts[achievementDXScoreStars] = {0};

int optionVSync = 1;
int optionIngameVSync = 0;
int rotateScreenBy = 0;

//audio
double machineMasterVolume = 0.5;

static void renderWithRightX(SDL_Texture* texture, float leftX, float y){
    float texW = 0;
    SDL_GetTextureSize(texture, &texW, NULL);
    const float rightX = screenWidth - texW - leftX;
    renderTextureSimple(texture, rightX, y, 1.f, false);
}

static void renderOffOn(float leftX, float y, bool enabled){
    renderWithRightX(optionOffOnTexts[enabled], leftX, y);
}

static void changeGameplayOption(int i, int changeValue){
    switch(i){
        case 0:;
            const double newTapSpeed = tapSpeed + 0.25 * changeValue;
            tapSpeed = SDL_round(clampDouble(newTapSpeed, 1.00, 10.00) * 4.0) / 4.0;
            break;
        case 1:;
            const double newTouchSpeed = touchSpeed + 0.25 * changeValue;
            touchSpeed = SDL_round(clampDouble(newTouchSpeed, 1.00, 10.00) * 4.0) / 4.0;
            break;
        case 2:
            rotateStarNote = changeValue > 0;
            break;
        case 3:
            mirrorType = clampInt(mirrorType + changeValue, 0, stackArraySizeInt(mirrorTexts));
            break;
        case 4:
            trackSkip = clampInt(trackSkip + changeValue, 0, stackArraySizeInt(trackSkipTexts));
            break;
        default:
            break;
    }
}

static void changeDisplayOption(int i, int changeValue){
    switch(i){
        case 0:;
            const double newBrightness = backgroundBrightness + 0.1 * changeValue;
            backgroundBrightness = SDL_round(clampDouble(newBrightness, 0.0, 1.0) * 10.0) / 10.0;
            break;
        case 1:
            useAlternateStarNote = changeValue > 0;
            break;
        case 2:
            showSensors = clampInt(showSensors + changeValue, 0, stackArraySizeInt(sensorTypes));
            break;
        case 3:
            displayType = clampInt(displayType + changeValue, 0, stackArraySizeInt(achievementTexts));
            break;
        default:
            break;
    }
}

static void changeJudgeOption(int i, int changeValue){
    switch(i){
        case 0:
            showFastLate = clampInt(showFastLate + changeValue, 0, stackArraySizeInt(fastLateTypes));
            break;
        case 1:
            showCriticalPerfect = clampInt(showCriticalPerfect + changeValue, 0, stackArraySizeInt(criticalPerfectTypes));
            break;
        case 2:;
            //negative 0 moment (x87)
            volatile double newChartOffset = SDL_round(clampDouble(chartOffset60fps + 0.1 * changeValue, -10.0, 10.0) * 10.0) / 10.0;
            if(newChartOffset >= -0.0001 && newChartOffset <= 0.0001){
                newChartOffset = 0.0;
            }

            chartOffset60fps = newChartOffset;

            realChartOffset = SDL_round(chartOffset60fps / 60.0 * gameTicksPerSecond);
            break;
        case 3:;
            //negative 0 moment (x87)
            volatile double newInputOffset =  SDL_round(clampDouble(inputOffset60fps + 0.1 * changeValue, -10.0, 10.0) * 10.0) / 10.0;
            if(newInputOffset >= -0.0001 && newInputOffset <= 0.0001){
                newInputOffset = 0.0;
            }

            inputOffset60fps = newInputOffset;
            realInputOffset = SDL_round(inputOffset60fps / 60.0 * gameTicksPerSecond);
            break;
        default:
            break;
    }
}

static void changeAudioOption(int i, int changeValue){
    switch(i){
        case 0:
            gameplaySfxVolume = SDL_round(clampDouble(gameplaySfxVolume + 0.1 * changeValue, 0.0, 1.0) * 10.0) / 10.0;
            break;
        case 1:
            userMusicVolume = SDL_round(clampDouble(userMusicVolume + 0.1 * changeValue, 0.0, 1.0) * 10.0) / 10.0;
            break;
        case 2:
            enableFireworksSFX = clampInt(enableFireworksSFX + changeValue, 0, 1);
            break;
        case 3:
            enableSlideSFX = clampInt(enableSlideSFX + changeValue, 0, 1);
            break;
        case 4:;
            //negative 0 moment
            volatile double newGuideOffset = SDL_round(clampDouble(guideOffset60fps + 0.1 * changeValue, -10.0, 10.0) * 10.0) / 10.0;
            if(newGuideOffset >= -0.0001 && newGuideOffset <= 0.0001){
                newGuideOffset = 0.0;
            }

            guideOffset60fps = newGuideOffset;
            realGuideOffset = SDL_round(guideOffset60fps / 60.0 * gameTicksPerSecond);
            break;
        default:
            break;
    }
}

static void renderGameplayOptions(float leftX, float rightX, float y, int i, int* changeUnavailable){
    char numberInfoChar[8];
    switch(i){
        case 0:;
            separateNumberWithCorrectDecimal(numberInfoChar, 8, tapSpeed, 2);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(tapSpeed <= 1.0 + 0.1){
                *changeUnavailable = -1;
            } else if(tapSpeed >= 10.0 - 0.1){
                *changeUnavailable = 1;
            }
            break;
        case 1:
            separateNumberWithCorrectDecimal(numberInfoChar, 8, touchSpeed, 2);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(touchSpeed <= 1.0 + 0.1){
                *changeUnavailable = -1;
            } else if(touchSpeed >= 10.0 - 0.1){
                *changeUnavailable = 1;
            }

            break;
        case 2:
            renderOffOn(leftX, y, rotateStarNote);
            *changeUnavailable = boolToInt(rotateStarNote);
            break;
        case 3:
            if(mirrorType == 0){
                renderOffOn(leftX, y, false);
                *changeUnavailable = -1;
            } else {
                renderWithRightX(mirrorTexts[mirrorType - 1], leftX, y);
                *changeUnavailable = mirrorType >= stackArraySizeInt(mirrorTexts);
            }
            break;
        case 4:
            if(trackSkip == 0){
                renderOffOn(leftX, y, false);
                *changeUnavailable = -1;
            } else {
                renderWithRightX(trackSkipTexts[trackSkip - 1], leftX, y);
                *changeUnavailable = trackSkip >= stackArraySizeInt(trackSkipTexts);
            }
            break;
        default:
            break;
    }
}

static void renderDisplayOptions(float leftX, float rightX, float y, int i, int* changeUnavailable){
    char numberInfoChar[8];
    switch(i){
        case 0:;
            separatePercentageWithCorrectDecimal(numberInfoChar, 8, backgroundBrightness * 100.0, 0);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(backgroundBrightness < 0.005){
                *changeUnavailable = -1;
            } else if(backgroundBrightness > 0.995){
                *changeUnavailable = 1;
            }
            break;
        case 1:
            if(!useAlternateStarNote){
                *changeUnavailable = -1;
            } else {
                *changeUnavailable = 1;
            }

            renderWithRightX(slideTapNoteNames[useAlternateStarNote], leftX, y);
            break;
        case 2:
            /*renderOffOn(leftX, y, showSensors);
            *changeUnavailable = boolToInt(showSensors);*/
            if(showSensors == 0){
                renderOffOn(leftX, y, false);
                *changeUnavailable = -1;
            } else {
                renderWithRightX(sensorTypes[showSensors - 1], leftX, y);
                *changeUnavailable = showSensors >= stackArraySizeInt(sensorTypes);
            }
            break;
        case 3:
            if(displayType == 0){
                renderOffOn(leftX, y, false);
                *changeUnavailable = -1;
            } else {
                renderWithRightX(achievementTexts[displayType - 1], leftX, y);
                *changeUnavailable = displayType >= stackArraySizeInt(achievementTexts);
            }
            break;
        default:
            break;
    }
}

static void renderJudgeOptions(float leftX, float rightX, float y, int i, int* changeUnavailable){
    char numberInfoChar[8];
    switch(i){
        case 0:
            if(showFastLate == 0){
                renderOffOn(leftX, y, false);
                *changeUnavailable = -1;
            } else {
                renderWithRightX(fastLateTypes[showFastLate - 1], leftX, y);
                *changeUnavailable = showFastLate >= stackArraySizeInt(fastLateTypes);
            }
            break;
        case 1:
            if(showCriticalPerfect == 0){
                renderOffOn(leftX, y, false);
                *changeUnavailable = -1;
            } else {
                renderWithRightX(criticalPerfectTypes[showCriticalPerfect - 1], leftX, y);
                *changeUnavailable = showCriticalPerfect >= stackArraySizeInt(criticalPerfectTypes);
            }
            break;
        case 2:
            separateNumberWithCorrectDecimal(numberInfoChar, 8, chartOffset60fps, 1);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(chartOffset60fps <= -10.0 + 0.01){
                *changeUnavailable = -1;
            } else if(chartOffset60fps >= 10.0 - 0.01){
                *changeUnavailable = 1;
            }
            break;
        case 3:
            separateNumberWithCorrectDecimal(numberInfoChar, 8, inputOffset60fps, 1);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(inputOffset60fps <= -10.0 + 0.01){
                *changeUnavailable = -1;
            } else if(inputOffset60fps >= 10.0 - 0.01){
                *changeUnavailable = 1;
            }
            break;
        default:
            break;
    }
}

static void renderAudioOptions(float leftX, float rightX, float y, int i, int* changeUnavailable){
    char numberInfoChar[8];
    switch(i){
        case 0:
            separatePercentageWithCorrectDecimal(numberInfoChar, 8, gameplaySfxVolume * 100.0, 0);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(gameplaySfxVolume < 0.005){
                *changeUnavailable = -1;
            } else if(gameplaySfxVolume > 0.995){
                *changeUnavailable = 1;
            }
            break;
        case 1:
            separatePercentageWithCorrectDecimal(numberInfoChar, 8, userMusicVolume * 100.0, 0);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(userMusicVolume < 0.005){
                *changeUnavailable = -1;
            } else if(userMusicVolume > 0.995){
                *changeUnavailable = 1;
            }
            break;
        case 2:
            renderOffOn(leftX, y, enableFireworksSFX);
            *changeUnavailable = boolToInt(enableFireworksSFX);
            break;
        case 3:
            renderOffOn(leftX, y, enableSlideSFX);
            *changeUnavailable = boolToInt(enableSlideSFX);
            break;
        case 4:
            separateNumberWithCorrectDecimal(numberInfoChar, 8, guideOffset60fps, 1);
            renderNumbers(numberInfoChar, rightX, y, rightAlign, false);
            if(guideOffset60fps <= -10.0 + 0.01){
                *changeUnavailable = -1;
            } else if(guideOffset60fps >= 10.0 - 0.01){
                *changeUnavailable = 1;
            }
        default:
            break;
    }
}

static Uint64 timeSinceBackOrNextPressed = 0;

#define gameOptionsTextsLen ((int)(groupOptionSelected == 0 ? stackArraySize(gameplayOptions) : groupOptionSelected == 1 ? stackArraySize(displayOptions) : groupOptionSelected == 2 ? stackArraySize(judgeOptions) : stackArraySize(audioOptions)))

static int renderGameOptions(void) {
    renderBackButtonBox();
    renderScrollButtonBoxes();
    const Uint8 fade = fadeInFadeOutAlpha(timeSinceBackOrNextPressed % gameTicksPerSecond / 2, gameTicksPerSecond / 4.0, gameTicksPerSecond / 4.0, gameTicksPerSecond / 2.0) / 2;
    if(insideGroupOption){
        renderTextureSimpler(groupOptions[groupOptionSelected], getCenterX(), 150.f * guiScale);
        const int len = gameOptionsTextsLen;

        float texH = 0;
        SDL_GetTextureSize(numbersText, NULL, &texH);
        int changeUnavailable = 0;
        for(int i = 0; i < len; i++){
            const float y = (202.f + 45.f * i) * guiScale;
            const float leftX = 70.f * guiScale + SDL_fabsf(y - getCenterY() + texH / 2.f) / 2.f;
            const float rightX = screenWidth - leftX;
            if(gameOptionSelected == i){

                SDL_FRect highlightOptionFRect = xywh(leftX, y, rightX - leftX, texH);
                changeFRectSizeBy(SDL_ceilf(2.f * guiScale), &highlightOptionFRect);
                setRenderDrawColor(fade + 96, 0, 0, SDL_ALPHA_OPAQUE / 2);
                renderFillRectF(&highlightOptionFRect);
            }

            #define gameOptionsTexts (groupOptionSelected == 0 ? gameplayOptions : groupOptionSelected == 1 ? displayOptions : groupOptionSelected == 2 ? judgeOptions : audioOptions)
            renderTextureSimple(gameOptionsTexts[i], leftX, y, 1.f, false);
            #undef gameOptionsTexts

            const int oldChangeUnavailable = changeUnavailable;

            switch(groupOptionSelected){
                case 0:
                    renderGameplayOptions(leftX, rightX, y, i, &changeUnavailable);
                    break;
                case 1:
                    renderDisplayOptions(leftX, rightX, y, i, &changeUnavailable);
                    break;
                case 2:
                    renderJudgeOptions(leftX, rightX, y, i, &changeUnavailable);
                    break;
                case 3:
                    renderAudioOptions(leftX, rightX, y, i, &changeUnavailable);
                    break;
                default:
                    printf("illegal group option %d\n", groupOptionSelected);
                    break;
            }

            if(gameOptionSelected != i){
                changeUnavailable = oldChangeUnavailable;
            }
        }

        if(changeUnavailable != -1){
            renderButtonTextBox(changeMinusI, changeMinus, 255, 0, 0);
        }

        if(changeUnavailable != 1){
            renderButtonTextBox(changePlusI, changePlus, 0, 255, 0);
        }
    } else {
        renderNextButtonBox();
        for(int i = 0; i < stackArraySizeInt(groupOptions); i++){
            if(groupOptionSelected == i){
                SDL_FRect highlightOptionFRect = xywh(getCenterX(), getCenterY() + 55.f * (i - 1.5f) * guiScale, 0.f, 0.f);
                SDL_GetTextureSize(groupOptions[i], &(highlightOptionFRect.w), &(highlightOptionFRect.h));
                highlightOptionFRect.x -= highlightOptionFRect.w / 2.f;
                highlightOptionFRect.y -= highlightOptionFRect.h / 2.f;
                changeFRectSizeBy(SDL_ceilf(2.f * guiScale), &highlightOptionFRect);
                setRenderDrawColor(fade + 96, 0, 0, SDL_ALPHA_OPAQUE / 2);
                renderFillRectF(&highlightOptionFRect);
            }

            renderTextureSimpler(groupOptions[i], getCenterX(), getCenterY() + 55.f * (i - 1.5f) * guiScale);
        }
    }
    return 0;
}

Uint64 gameOptions(Uint64 delta){
    static Uint64 optionMenuHeldTime = 0;
    static int optionMenuHeldType = 0;
    timeSinceBackOrNextPressed += delta;

    const Uint64 optionsMenuARR = (Uint64)(SDL_max(gameTicksPerSecond / 10, 1));
    const Uint64 optionsMenuDAS = (Uint64)gameTicksPerSecond / 2;
    if(optionMenuHeldType != 0){
        optionMenuHeldTime += delta;
        if(optionMenuHeldTime > optionsMenuDAS + optionsMenuARR){
            pressedButtons[optionMenuHeldType - 1] = true;
            pressedTouchSensors[optionMenuHeldType - 1] = true; //real moment
            optionMenuHeldTime -= optionsMenuARR;
        }
    }

    for(int i = 0; i < 8; i++){
        if(isButtonOrTouchPressed(i + 1) && (optionMenuHeldType <= 0 || !isButtonOrTouchPressed(optionMenuHeldType))){
            optionMenuHeldTime = 0;
            optionMenuHeldType = i + 1;
        }
    }

    if(optionMenuHeldType > 0 && isButtonAndTouchReleased(optionMenuHeldType)){
        optionMenuHeldTime = 0;
        optionMenuHeldType = 0;
    }


    if(isButtonOrTouchPressed(scrollUpButtonBoxI)){
        if(insideGroupOption){
            gameOptionSelected = SDL_max(0, gameOptionSelected - 1);
        } else {
            groupOptionSelected = SDL_max(0, groupOptionSelected - 1);
        }
    }

    if(isButtonOrTouchPressed(scrollDownButtonBoxI)){
        if(insideGroupOption){
            gameOptionSelected = SDL_min(gameOptionsTextsLen - 1, gameOptionSelected + 1);
        } else {
            groupOptionSelected = SDL_min(stackArraySizeInt(groupOptions) - 1, groupOptionSelected + 1);
        }
    }

    if(isButtonOrTouchPressed(backButtonBoxI) && timeSinceBackOrNextPressed > gameTicksPerSecond / 4){
        timeSinceBackOrNextPressed = 0;
        if(insideGroupOption){
            insideGroupOption = false;
            gameOptionSelected = 0;
        } else {
            optionMenuHeldTime = 0;
            optionMenuHeldType = 0;
            if(numberOfProfiles > 0){
                updateProfile(false);
            }
            runLoadFunction(loadSongSelect, songSelectSlot);
            return 0;
        }
    }

    if(isButtonOrTouchPressed(nextButtonBoxI) && !insideGroupOption && timeSinceBackOrNextPressed > gameTicksPerSecond / 4){
        timeSinceBackOrNextPressed = 0;
        insideGroupOption = true;
    }

    int changeValue = isButtonOrTouchPressed(changePlusI) - isButtonOrTouchPressed(changeMinusI);

    if(changeValue != 0 && insideGroupOption){
        switch(groupOptionSelected){
            case 0:
                changeGameplayOption(gameOptionSelected, changeValue);
                break;
            case 1:
                changeDisplayOption(gameOptionSelected, changeValue);
                break;
            case 2:
                changeJudgeOption(gameOptionSelected, changeValue);
                break;
            case 3:
                changeAudioOption(gameOptionSelected, changeValue);
                break;
            default:
                printf("illegal group option %d\n", groupOptionSelected);
                break;
        }
    }

    renderFunction = renderGameOptions;
    subscreenRenderFunction = renderUserInfoOnSubscreen;
    return 0;
}
#undef gameOptionsTextsLen

static int optionsHandler(void* user, const char* section, const char* name, const char* value){
    UNUSED(user);
    //printf("section %s name %s\n", section, name);
    if(SDL_strcmp(section, "generic") == 0){
        if(SDL_strcmp(name, "language") == 0){
            currentLocaleName = SDL_strdup(value);
        } else if(SDL_strcmp(name, "judgelanguage") == 0){
            currentJudgeLocaleName = SDL_strdup(value);
        } else {
            return 0;
        }
    } else if(SDL_strcmp(section, "window") == 0){
        if(SDL_strcmp(name, "fullscreenmode") == 0){
            fullscreenMode = clampInt(SDL_atoi(value), 0, 2);
        } else if(SDL_strcmp(name, "windowwidth") == 0){
            windowWidth = SDL_atoi(value);
        } else if(SDL_strcmp(name, "windowheight") == 0){
            windowHeight = SDL_atoi(value);
        } else if(SDL_strcmp(name, "exclusivewidth") == 0){
            exclusiveWidth = SDL_atoi(value);
        } else if(SDL_strcmp(name, "exclusiveheight") == 0){
            exclusiveHeight = SDL_atoi(value);
        } else if(SDL_strcmp(name, "exclusivehz") == 0){
            exclusiveHz = SDL_atof(value);
        } else {
            return 0;
        }
    } else if(SDL_strcmp(section, "graphics") == 0){
        if(SDL_strcmp(name, "vsync") == 0){
            optionVSync = SDL_atoi(value);
        } else if(SDL_strcmp(name, "ingamevsync") == 0){
            optionIngameVSync = SDL_atoi(value);
        } else if(SDL_strcmp(name, "rotateby") == 0){
            rotateScreenBy = SDL_strcmp(value, "90") == 0 ? 90 :
                             SDL_strcmp(value, "180") == 0 ? 180 :
                             SDL_strcmp(value, "270") == 0 ? 270 : 0;
        } else {
            return 0;
        }
    } else if(SDL_strcmp(section, "video") == 0){
        #ifdef Circular8_ENABLE_FFmpeg
        if(SDL_strcmp(name, "enablemovie") == 0){
            isVideoEnabled = SDL_strcmp(value, "1") == 0;
        } else if(SDL_strcmp(name, "framebuffersize") == 0){
            numberOfFrameBuffers = clampInt(SDL_atoi(value), 2, stackArraySizeInt(frameBuffers));
        } else {
            return 0;
        }
        #endif // Circular8_ENABLE_FFmpeg
    } else if(SDL_strcmp(section, "audio") == 0){
        if(SDL_strcmp(name, "machinemastervolume") == 0){
            machineMasterVolume = clampDouble(SDL_atof(value), 0.0, 1.0);
        } else {
            return 0;
        }
    } else if(SDL_strcmp(section, "game") == 0){
        if(SDL_strcmp(name, "usesubscreen") == 0){
            useSubscreen = SDL_strcmp(value, "1") == 0;
        } else if(SDL_strcmp(name, "enablerating") == 0){
            enableRating = SDL_strcmp(value, "1") == 0;
        } else if(SDL_strcmp(name, "enablebginsongselect") == 0){
            enableSongSelectBGs = SDL_strcmp(value, "1") == 0;
        } else {
            return 0;
        }
    } else if(SDL_strcmp(section, "controller") == 0){
        #ifdef Circular8_ENABLE_CONTROLLER
        if(SDL_strcmp(name, "enablecontroller") == 0){
            isUsingSerialController = SDL_strcmp(value, "1") == 0;
        } else if(SDL_strcmp(name, "enablelights") == 0){
            isUsingSerialLightController = SDL_strcmp(value, "1") == 0;
        } else if(SDL_strcmp(name, "usecustomcontrollerports") == 0){
            enableCustomControllerPorts = SDL_strcmp(value, "1") == 0;
        } else if(SDL_strcmp(name, "controllertouchport") == 0){
            touchSerialPort = SDL_strdup(value);
        } else if(SDL_strcmp(name, "controllerlightsport") == 0){
            lightsSerialPort = SDL_strdup(value);
        } else if(SDL_strcmp(name, "buttondebouncedelay") == 0){
            buttonDebounceDelay = clampDouble(SDL_ceil(SDL_atof(value) * gameTicksPerSecond), 0, gameTicksPerSecond / 5);
        } else if(SDL_strcmp(name, "player") == 0){
            controllerPlayer = 1 + (SDL_strcmp(value, "2") == 0);
        } else {
            return 0;
        }
        #endif // Circular8_ENABLE_CONTROLLER
    } else {
        return 0;
    }

    return 1;
}

void loadOptionsINI(const char* optionsName){
    char* realFileName = joinTwoCharArraysToOne(optionsName, ".ini");
    if(ini_parse(realFileName, optionsHandler, NULL) < 0){
        printf("BAD INI\n");
    }
    SDL_free(realFileName);
}
