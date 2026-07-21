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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "main.h"

#include "gameinit.h"
#include "load.h"
#include "render.h"
#include "texture.h"
#include "miscutils.h"
#include "language.h"

#ifdef Circular8_ENABLE_FFmpeg
#include "video.h"
#endif // Circular8_ENABLE_FFmpeg

#include "game/mainmenu.h"
#include "game/ingame.h"
#include "game/rendersong.h"
#include "game/chart.h"
#include "game/songselect.h"
#include "game/difficultyselect.h"
#include "game/scorescreen.h"
#include "game/options.h"
#include "game/profileselect.h"

#include "thirdparty/inih/ini.h"



void loadMainMenuGui(int slot){
    (void)slot;
    if(isLoadImagePart){
        //do random stuff here
        loadTextSimple(&mainMenuTitle, bigFont, "Circular8", white, false);
        loadTextLocalized(&mainMenuStartButton, bigFont, "buttonstart", white, false);

        char infoStr[64] = {0};
        simple_snprintf(infoStr, "Build date: %s at %s", __DATE__, __TIME__);

        loadTextSimple(&buildDate, mediumFont, infoStr, white, false);


        const int SDLRuntimeVer = SDL_GetVersion();
        const int SDL_imageRuntimeVer = IMG_Version();
        const int SDL_mixerRuntimeVer = MIX_Version();
        const int SDL_ttfRuntimeVer = TTF_Version();

        //SDL
        simple_snprintf(infoStr, "SDL: %d.%d.%d (%d.%d.%d at build time)",
                                            SDL_VERSIONNUM_MAJOR(SDLRuntimeVer),
                                            SDL_VERSIONNUM_MINOR(SDLRuntimeVer),
                                            SDL_VERSIONNUM_MICRO(SDLRuntimeVer),
                                            SDL_MAJOR_VERSION,
                                            SDL_MINOR_VERSION,
                                            SDL_MICRO_VERSION);
        loadTextSimple(&(SDLVersions[0]), smallFont, infoStr, white, false);

        //SDL_image
        simple_snprintf(infoStr, "SDL_image: %d.%d.%d (%d.%d.%d at build time)",
                                    SDL_VERSIONNUM_MAJOR(SDL_imageRuntimeVer),
                                    SDL_VERSIONNUM_MINOR(SDL_imageRuntimeVer),
                                    SDL_VERSIONNUM_MICRO(SDL_imageRuntimeVer),
                                    SDL_IMAGE_MAJOR_VERSION,
                                    SDL_IMAGE_MINOR_VERSION,
                                    SDL_IMAGE_MICRO_VERSION);
        loadTextSimple(&(SDLVersions[1]), smallFont, infoStr, white, false);

        //SDL_mixer
        simple_snprintf(infoStr, "SDL_mixer: %d.%d.%d (%d.%d.%d at build time)",
                                    SDL_VERSIONNUM_MAJOR(SDL_mixerRuntimeVer),
                                    SDL_VERSIONNUM_MINOR(SDL_mixerRuntimeVer),
                                    SDL_VERSIONNUM_MICRO(SDL_mixerRuntimeVer),
                                    SDL_MIXER_MAJOR_VERSION,
                                    SDL_MIXER_MINOR_VERSION,
                                    SDL_MIXER_MICRO_VERSION);
        loadTextSimple(&(SDLVersions[2]), smallFont, infoStr, white, false);

        //SDL_ttf
        simple_snprintf(infoStr, "SDL_ttf: %d.%d.%d (%d.%d.%d at build time)",
                                    SDL_VERSIONNUM_MAJOR(SDL_ttfRuntimeVer),
                                    SDL_VERSIONNUM_MINOR(SDL_ttfRuntimeVer),
                                    SDL_VERSIONNUM_MICRO(SDL_ttfRuntimeVer),
                                    SDL_TTF_MAJOR_VERSION,
                                    SDL_TTF_MINOR_VERSION,
                                    SDL_TTF_MICRO_VERSION);
        loadTextSimple(&(SDLVersions[3]), smallFont, infoStr, white, false);
    } else {
        addGameFunctionToCurrentSlot(mainMenu);
    }
}

static void createSensorHelper(char* sensorSVGText, int sensor, SDL_FRect* minMaxFRect, float decreasePosBy){
    const float minX = minMaxFRect->x;
    const float minY = minMaxFRect->y;
    const float width = minMaxFRect->w * guiScale;
    const float height = minMaxFRect->h * guiScale;
    sensorFRects[sensor] = xywh(minX - decreasePosBy, minY - decreasePosBy, width * 360.f, height * 360.f);

    loadRawSVGSimple(&sensors[sensor], sensorSVGText, wh(SDL_roundf(width * 360.f), SDL_roundf(height * 360.f)));
}

static int loadNoteImage(SDL_Texture** note, char* notePath, SDL_Rect rect){
    char* path = joinTwoCharArraysToOne("gfx/notes/", notePath);
    const int returnValue = loadImageSimple(note, path, rect);
    SDL_free(path);
    return returnValue; //average
}

static void destroyChartGraphics(void){
    destroyTextureIfNotNull(gameBorder);
    //destroyTextureIfNotNull(fireworksTexture);
    destroyTextureIfNotNull(gameBackground);

    destroyStackArrayOfTextures(sensors);

    destroyStackArrayOfTextures(FCMedals);
}

void loadChartGraphics(void){
    int maxTextureID = 0;

    loadImageSimple(&gameBorder, "gfx/ingame/border.svg", whScaled(660, 660));
    setAtlas(0, 4);
    if(chartNoteFeatures[featureTapNormal]){
        normalTapNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "tap/normaltap.svg", whScaled(70, 70)); //average
    }

    if(chartNoteFeatures[featureTapBreak]){
        breakTapNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "tap/breaktap.svg", whScaled(70, 70)); //gonna get that 101% SSS+ score
    }

    if(chartNoteFeatures[featureTapEach]){
        eachTapNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "tap/eachtap.svg", whScaled(70, 70));
    }

    if(chartNoteFeatures[featureEX]){
        EXNoteBlurID = maxTextureID++;
        loadNoteImage(&nullTexture, "exnoteblur.svg", whScaled(70, 70));
    }

    //actual hell for hold notes
    const bool chartHasHold = chartNoteFeatures[featureHoldNormal] || chartNoteFeatures[featureHoldBreak] || chartNoteFeatures[featureHoldEach];

    if(chartHasHold){
        holdNoteHitMarkerID = maxTextureID++;
        unheldHoldNoteSize1ID = maxTextureID++;
        loadNoteImage(&nullTexture, "hold/holdhitmarker.svg", whScaled(44, 44));
        loadNoteImage(&nullTexture, "hold/unheldholdsize1.svg", whScaled(70, 330));
    }

    if(chartNoteFeatures[featureHoldNormal]){
        normalHoldNoteSize0ID = maxTextureID++;
        normalHoldNoteSize1ID = maxTextureID++;
        loadNoteImage(&nullTexture, "hold/normalholdsize0.svg", whScaled(70, 70));
        loadNoteImage(&nullTexture, "hold/normalholdsize1.svg", whScaled(70, 330));
    }

    if(chartNoteFeatures[featureHoldBreak]){
        breakHoldNoteSize0ID = maxTextureID++;
        breakHoldNoteSize1ID = maxTextureID++;
        loadNoteImage(&nullTexture, "hold/breakholdsize0.svg", whScaled(70, 70));
        loadNoteImage(&nullTexture, "hold/breakholdsize1.svg", whScaled(70, 330));
    }

    if(chartNoteFeatures[featureHoldEach]){
        eachHoldNoteSize0ID = maxTextureID++;
        eachHoldNoteSize1ID = maxTextureID++;
        loadNoteImage(&nullTexture, "hold/eachholdsize0.svg", whScaled(70, 70));
        loadNoteImage(&nullTexture, "hold/eachholdsize1.svg", whScaled(70, 330));
    }

    if(chartNoteFeatures[featureSlideNormal]){
        normalSlideArrowID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/normalslidearrow.svg", whScaled(52, 36));
    }

    if(chartNoteFeatures[featureSlideBreak]){
        breakSlideArrowID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/breakslidearrow.svg", whScaled(52, 36));
    }

    if(chartNoteFeatures[featureSlideEach]){
        eachSlideArrowID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/eachslidearrow.svg", whScaled(52, 36));
    }

    //what on earth am i supposed to do for fan
    const SDL_Rect fanSlideSizes[11] = {whScaled(84, 32),
                                        whScaled(118, 39),
                                        whScaled(156, 48),
                                        whScaled(194, 59),
                                        whScaled(228, 66),
                                        whScaled(274, 75),
                                        whScaled(318, 88),
                                        whScaled(362, 97),
                                        whScaled(404, 105),
                                        whScaled(446, 113),
                                        whScaled(496, 124)
    };

    char info[1024] = {0};

    if(chartNoteFeatures[featureFanSlideNormal]){
        normalFanSlideArrowsStartID = maxTextureID;
        for(unsigned int i = 0; i < stackArraySize(fanSlideSizes); i++){
            simple_snprintf(info, "slide/fan/normal/fanslidearrow%d.svg", i + 1);
            loadNoteImage(&nullTexture, info, fanSlideSizes[i]);
            maxTextureID++;
        }
    }

    if(chartNoteFeatures[featureFanSlideEach]){
        eachFanSlideArrowsStartID = maxTextureID;
        for(unsigned int i = 0; i < stackArraySize(fanSlideSizes); i++){
            simple_snprintf(info, "slide/fan/each/fanslidearrow%d.svg", i + 1);
            loadNoteImage(&nullTexture, info, fanSlideSizes[i]);
            maxTextureID++;
        }
    }

    if(chartNoteFeatures[featureFanSlideBreak]){
        breakFanSlideArrowsStartID = maxTextureID;
        for(unsigned int i = 0; i < stackArraySize(fanSlideSizes); i++){
            simple_snprintf(info, "slide/fan/break/fanslidearrow%d.svg", i + 1);
            loadNoteImage(&nullTexture, info, fanSlideSizes[i]);
            maxTextureID++;
        }
    }



    if(chartNoteFeatures[featureSlideTapNormal] || chartNoteFeatures[featureSlideNormal] || chartNoteFeatures[featureFanSlideNormal]){
        normalSlideNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, useAlternateStarNote ? "slide/altslidenote.svg" : "slide/normalslidenote.svg", whScaled(70, 70));
    }

    if(chartNoteFeatures[featureSlideTapBreak] || chartNoteFeatures[featureSlideBreak] || chartNoteFeatures[featureFanSlideBreak]){
        breakSlideNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/breakslidenote.svg", whScaled(70, 70));
    }

    if(chartNoteFeatures[featureSlideTapEach] || chartNoteFeatures[featureSlideEach] || chartNoteFeatures[featureFanSlideEach]){
        eachSlideNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/eachslidenote.svg", whScaled(70, 70));
    }

    if(chartNoteFeatures[featureMultiSlideTapNormal]){
        normalMultiSlideNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, useAlternateStarNote ? "slide/altmultislidenote.svg" : "slide/normalmultislidenote.svg", whScaled(70, 70));
    }

    if(chartNoteFeatures[featureMultiSlideTapBreak]){
        breakMultiSlideNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/breakmultislidenote.svg", whScaled(70, 70));
    }

    if(chartNoteFeatures[featureMultiSlideTapEach]){
        eachMultiSlideNoteID = maxTextureID++;
        loadNoteImage(&nullTexture, "slide/eachmultislidenote.svg", whScaled(70, 70));
    }

    //touch notes

    if(chartNoteFeatures[featureTouchNormal] || chartNoteFeatures[featureTouchHold]){
        normalTouchPointID = maxTextureID++;
        normalTouchArrowID = maxTextureID++;
        multiNormalTouchBoxID = maxTextureID++;
        loadNoteImage(&nullTexture, "touch/normaltouchpoint.svg", whScaled(16, 16));
        loadNoteImage(&nullTexture, "touch/normaltoucharrow.svg", whScaled(62, 34));
        loadNoteImage(&nullTexture, "touch/multinormaltouchbox.svg", whScaled(80, 80));
    }

    if(chartNoteFeatures[featureTouchEach] || chartNoteFeatures[featureTouchHold]){
        eachTouchPointID = maxTextureID++;
        eachTouchArrowID = maxTextureID++;
        multiEachTouchBoxID = maxTextureID++;
        loadNoteImage(&nullTexture, "touch/eachtouchpoint.svg", whScaled(16, 16));
        loadNoteImage(&nullTexture, "touch/eachtoucharrow.svg", whScaled(62, 34));
        loadNoteImage(&nullTexture, "touch/multieachtouchbox.svg", whScaled(80, 80));
    }

    //hold touch note

    if(chartNoteFeatures[featureTouchHold]){
        holdTouchTrianglesStartID = maxTextureID;
        loadNoteImage(&nullTexture, "touch/hold/touchhold1.svg", whScaled(54, 54));
        loadNoteImage(&nullTexture, "touch/hold/touchhold2.svg", whScaled(54, 54));
        loadNoteImage(&nullTexture, "touch/hold/touchhold3.svg", whScaled(54, 54));
        loadNoteImage(&nullTexture, "touch/hold/touchhold4.svg", whScaled(54, 54));
        maxTextureID += 4;

        holdTouchProgressionID = maxTextureID++;
        holdTouchProgressionUnheldID = maxTextureID++;
        loadNoteImage(&nullTexture, "touch/hold/touchprogression.svg", whScaled(125, 125));
        loadNoteImage(&nullTexture, "touch/hold/touchprogressionunheld.svg", whScaled(125, 125));
    }

    //todo: add more ids

    const bool chartHasSlides = chartNoteFeatures[featureSlideNormal] || chartNoteFeatures[featureSlideBreak] || chartNoteFeatures[featureSlideEach];
    const bool chartHasFanSlides = chartNoteFeatures[featureFanSlideNormal] || chartNoteFeatures[featureFanSlideBreak] || chartNoteFeatures[featureFanSlideEach];

    judgementsStartID = maxTextureID;
    if(SDL_strncmp(currentJudgeLocaleName, "french", 2) == 0){
        loadImageSimple(&nullTexture, "gfx/ingame/judges/french/rate.svg", whScaled(61.686, 40));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/french/bon.svg", whScaled(63.205, 40));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/french/super.svg", whScaled(99.483, 40));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/french/parfait.svg", whScaled(130.187, 40));
        if(showCriticalPerfect){
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/maximum.svg", whScaled(165.955, 40));
            maxTextureID += 5;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/max.svg", whScaled(55.84 / 1.2, 30 / 1.2));
            criticalJudgeID = maxTextureID++;
        } else {
            maxTextureID += 4;
        }

        if(chartHasSlides || chartHasFanSlides){
            slideInfoJudgementsStartID = maxTextureID;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/glissade/troptard.svg", whScaled(133.626, 30));
            if(showCriticalPerfect){
                maxTextureID++;
            } else {
                maxTextureID += 2;
                loadImageSimple(&nullTexture, "gfx/ingame/judges/french/glissade/juste.svg", whScaled(74.194, 30));
            }

            fastLateJudgementsStartID = maxTextureID;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/rapide.svg", whScaled(83.96, 30));
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/tard.svg", whScaled(60.97, 30));
            maxTextureID += 2;
        } else if(showFastLate){
            fastLateJudgementsStartID = maxTextureID;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/rapide.svg", whScaled(83.96, 30));
            loadImageSimple(&nullTexture, "gfx/ingame/judges/french/tard.svg", whScaled(60.97, 30));
            maxTextureID += 2;
        }
    } else {
        loadImageSimple(&nullTexture, "gfx/ingame/judges/english/miss.svg", whScaled(80.534, 40));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/english/good.svg", whScaled(83.371, 40));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/english/great.svg", whScaled(97.457, 40));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/english/perfect.svg", whScaled(132.822, 40));
        if(showCriticalPerfect){
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/cperfect.svg", whScaled(132.822, 40));
            maxTextureID += 5;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/critical.svg", whScaled(106.304 / 1.2, 30 / 1.2));
            criticalJudgeID = maxTextureID++;
        } else {
            maxTextureID += 4;
        }

        if(chartHasSlides || chartHasFanSlides){
            slideInfoJudgementsStartID = maxTextureID;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/slide/toolate.svg", whScaled(112.118, 30));
            if(showCriticalPerfect){
                maxTextureID++;
            } else {
                loadImageSimple(&nullTexture, "gfx/ingame/judges/english/slide/just.svg", whScaled(61.388, 30));
                maxTextureID += 2;
            }

            fastLateJudgementsStartID = maxTextureID;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/fast.svg", whScaled(58.044, 30));
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/late.svg", whScaled(54.7, 30));
            maxTextureID += 2;
        } else if(showFastLate){
            fastLateJudgementsStartID = maxTextureID;
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/fast.svg", whScaled(58.044, 30));
            loadImageSimple(&nullTexture, "gfx/ingame/judges/english/late.svg", whScaled(54.7, 30));
            maxTextureID += 2;
        }
    }

    //more slides
    slideBigJudgementsStartID = -1; //hack
    if(chartHasSlides){
        slideBigJudgementsStartID = maxTextureID;
        loadImageSimple(&nullTexture, "gfx/ingame/judges/slideshape/lineslidejudge.svg", whScaled(201.375, 51));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/slideshape/arcslidejudge.svg", whScaled(232.21871, 65.87015));
        maxTextureID += 2;
    }

    if(chartHasFanSlides){
        if(slideBigJudgementsStartID <= -1){
            slideBigJudgementsStartID = maxTextureID - 2; //todo: check if that works
        }
        maxTextureID++;
        loadImageSimple(&nullTexture, "gfx/ingame/judges/slideshape/fanslidejudge.svg", whScaled(343.0436, 91.80488));
    }

    if(chartHasHold || chartNoteFeatures[featureTouchHold]){
        alternativeJudgementsStartID = maxTextureID;
        loadImageSimple(&nullTexture, "gfx/ingame/judges/headhold/headmiss.svg", whScaled(92, 92));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/headhold/headgood.svg", whScaled(92, 92));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/headhold/headgreat.svg", whScaled(92, 92));
        loadImageSimple(&nullTexture, "gfx/ingame/judges/headhold/headperfect.svg", whScaled(92, 92));
        if(showCriticalPerfect){
            loadImageSimple(&nullTexture, "gfx/ingame/judges/headhold/headcperfect.svg", whScaled(92, 92));
            maxTextureID += 5;
        } else {
            maxTextureID += 4;
        }
    }

    char* topInfoStr = NULL;
    bool isDXStars = false;


    switch(displayType){
        case achievementCombo:
            topInfoStr = "displayatcentercombo";
            break;
        case achievementPlus:
        case achievementAverage:
        case achievementMinus1:
        case achievementMinus2:
            topInfoStr = "genericscore";
            break;
        case achievementDXScorePlus:
        case achievementDXScoreMinus:
            topInfoStr = "genericdxscore";
            break;
        case achievementDXScoreStars:
            isDXStars = true;
            break;
        default:
            break;
    }

    if(topInfoStr == NULL){
        destroyTextureIfNotNull(centerDisplayTopInfo);

        const char* DXStarsStr[] = {"1dxstar", "2dxstars", "3dxstars", "4dxstars", "5dxstars", "6dxstars"};

        for(unsigned int i = 0; i < stackArraySize(DXStarsStr) - 1; i++){ //todo: enable option for 6*
            if(!isDXStars){
                destroyTextureIfNotNull(centerDisplayTopDXStarsInfo[i]);
            } else {
                loadTextLocalized(&centerDisplayTopDXStarsInfo[i], mediumFont, DXStarsStr[i], white, false);
            }
        }

    } else {
        loadTextLocalized(&centerDisplayTopInfo, mediumFont, topInfoStr, white, false);
        for(unsigned int i = 0; i < stackArraySize(centerDisplayTopDXStarsInfo); i++){
            destroyTextureIfNotNull(centerDisplayTopDXStarsInfo[i]);
        }
    }

    if(trackSkip == 0){
        destroyTextureIfNotNull(trackSkippingText);
    } else {
        loadTextLocalized(&trackSkippingText, mediumFont, "trackskipping", white, false);
    }

    setAtlas(-1, 0);

    //fireworks
    if(chartNoteFeatures[featureFireworks]){
        //fireworksTextureID = maxTextureID++;
        loadImageSimple(&fireworksTexture, "gfx/ingame/fireworks.svg", whScaled(720, 720));
    }

    //sensor hell


    //boilerplate moment
    SDL_FRect minMaxFRect[5] = {
        {32000, 32000, -32000, -32000},
        {32000, 32000, -32000, -32000},
        {32000, 32000, -32000, -32000},
        {32000, 32000, -32000, -32000},
        {32000, 32000, -32000, -32000}
    };

    for(int i = 0; i < 9; i++){
        minMaxFRect[0].x = SDL_min(minMaxFRect[0].x, ASensorPoints[i].x);
        minMaxFRect[0].y = SDL_min(minMaxFRect[0].y, -ASensorPoints[i].y);
        minMaxFRect[0].w = SDL_max(minMaxFRect[0].w, ASensorPoints[i].x);
        minMaxFRect[0].h = SDL_max(minMaxFRect[0].h, -ASensorPoints[i].y);
    }

    for(int i = 0; i < 7; i++){
        minMaxFRect[1].x = SDL_min(minMaxFRect[1].x, BSensorPoints[i].x);
        minMaxFRect[1].y = SDL_min(minMaxFRect[1].y, -BSensorPoints[i].y);
        minMaxFRect[1].w = SDL_max(minMaxFRect[1].w, BSensorPoints[i].x);
        minMaxFRect[1].h = SDL_max(minMaxFRect[1].h, -BSensorPoints[i].y);
    }

    for(int i = 0; i < 6; i++){
        minMaxFRect[2].x = SDL_min(minMaxFRect[2].x, CSensorPoints[i].x);
        minMaxFRect[2].y = SDL_min(minMaxFRect[2].y, -CSensorPoints[i].y);
        minMaxFRect[2].w = SDL_max(minMaxFRect[2].w, CSensorPoints[i].x);
        minMaxFRect[2].h = SDL_max(minMaxFRect[2].h, -CSensorPoints[i].y);
    }

    for(int i = 0; i < 6; i++){
        minMaxFRect[3].x = SDL_min(minMaxFRect[3].x, DSensorPoints[i].x);
        minMaxFRect[3].y = SDL_min(minMaxFRect[3].y, -DSensorPoints[i].y);
        minMaxFRect[3].w = SDL_max(minMaxFRect[3].w, DSensorPoints[i].x);
        minMaxFRect[3].h = SDL_max(minMaxFRect[3].h, -DSensorPoints[i].y);
    }

    for(int i = 0; i < 4; i++){
        minMaxFRect[4].x = SDL_min(minMaxFRect[4].x, ESensorPoints[i].x);
        minMaxFRect[4].y = SDL_min(minMaxFRect[4].y, -ESensorPoints[i].y);
        minMaxFRect[4].w = SDL_max(minMaxFRect[4].w, ESensorPoints[i].x);
        minMaxFRect[4].h = SDL_max(minMaxFRect[4].h, -ESensorPoints[i].y);
    }


    const float increaseSizeBy = 0.01f;
    const float decreasePosBy = 0.005f;


    minMaxFRect[0].w -= minMaxFRect[0].x - increaseSizeBy;
    minMaxFRect[1].w -= minMaxFRect[1].x - increaseSizeBy;
    minMaxFRect[2].w -= minMaxFRect[2].x - increaseSizeBy;
    minMaxFRect[3].w -= minMaxFRect[3].x - increaseSizeBy;
    minMaxFRect[4].w -= minMaxFRect[4].x - increaseSizeBy;

    minMaxFRect[0].h -= minMaxFRect[0].y - increaseSizeBy;
    minMaxFRect[1].h -= minMaxFRect[1].y - increaseSizeBy;
    minMaxFRect[2].h -= minMaxFRect[2].y - increaseSizeBy;
    minMaxFRect[3].h -= minMaxFRect[3].y - increaseSizeBy;
    minMaxFRect[4].h -= minMaxFRect[4].y - increaseSizeBy;

    simple_snprintf(info,
    "<svg viewBox=\"%.6f,%.6f,%.6f,%.6f\"><polygon points=\"%.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f\" fill=\"#fff\" stroke=\"#fff\" fill-opacity=\".1\" stroke-opacity=\".25\" stroke-width=\".005\"/></svg>",
                    minMaxFRect[0].x - decreasePosBy, minMaxFRect[0].y - decreasePosBy, minMaxFRect[0].w, minMaxFRect[0].h,
                    ASensorPoints[0].x, -ASensorPoints[0].y,
                    ASensorPoints[1].x, -ASensorPoints[1].y,
                    ASensorPoints[2].x, -ASensorPoints[2].y,
                    ASensorPoints[3].x, -ASensorPoints[3].y,
                    ASensorPoints[4].x, -ASensorPoints[4].y,
                    ASensorPoints[5].x, -ASensorPoints[5].y,
                    ASensorPoints[6].x, -ASensorPoints[6].y,
                    ASensorPoints[7].x, -ASensorPoints[7].y,
                    ASensorPoints[8].x, -ASensorPoints[8].y
                    );

    createSensorHelper(info, 0, &(minMaxFRect[0]), decreasePosBy);

    simple_snprintf(info,
    "<svg viewBox=\"%.6f,%.6f,%.6f,%.6f\"><polygon points=\"%.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f\" fill=\"#fff\" stroke=\"#fff\" fill-opacity=\".1\" stroke-opacity=\".25\" stroke-width=\".005\"/></svg>",
                    minMaxFRect[1].x - decreasePosBy, minMaxFRect[1].y - decreasePosBy, minMaxFRect[1].w, minMaxFRect[1].h,
                    BSensorPoints[0].x, -BSensorPoints[0].y,
                    BSensorPoints[1].x, -BSensorPoints[1].y,
                    BSensorPoints[2].x, -BSensorPoints[2].y,
                    BSensorPoints[3].x, -BSensorPoints[3].y,
                    BSensorPoints[4].x, -BSensorPoints[4].y,
                    BSensorPoints[5].x, -BSensorPoints[5].y,
                    BSensorPoints[6].x, -BSensorPoints[6].y
                    );

    createSensorHelper(info, 1, &(minMaxFRect[1]), decreasePosBy);

    simple_snprintf(info,
    "<svg viewBox=\"%.6f,%.6f,%.6f,%.6f\"><polygon points=\"%.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f\" fill=\"#fff\" stroke=\"#fff\" fill-opacity=\".1\" stroke-opacity=\".25\" stroke-width=\".005\"/></svg>",
                    minMaxFRect[2].x - decreasePosBy, minMaxFRect[2].y - decreasePosBy, minMaxFRect[2].w, minMaxFRect[2].h,
                    CSensorPoints[0].x, -CSensorPoints[0].y,
                    CSensorPoints[1].x, -CSensorPoints[1].y,
                    CSensorPoints[2].x, -CSensorPoints[2].y,
                    CSensorPoints[3].x, -CSensorPoints[3].y,
                    CSensorPoints[4].x, -CSensorPoints[4].y,
                    CSensorPoints[5].x, -CSensorPoints[5].y
                    );

    createSensorHelper(info, 2, &(minMaxFRect[2]), decreasePosBy);

    simple_snprintf(info,
    "<svg viewBox=\"%.6f,%.6f,%.6f,%.6f\"><polygon points=\"%.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f\" fill=\"#fff\" stroke=\"#fff\" fill-opacity=\".1\" stroke-opacity=\".25\" stroke-width=\".005\"/></svg>",
                    minMaxFRect[3].x - decreasePosBy, minMaxFRect[3].y - decreasePosBy, minMaxFRect[3].w, minMaxFRect[3].h,
                    DSensorPoints[0].x, -DSensorPoints[0].y,
                    DSensorPoints[1].x, -DSensorPoints[1].y,
                    DSensorPoints[2].x, -DSensorPoints[2].y,
                    DSensorPoints[3].x, -DSensorPoints[3].y,
                    DSensorPoints[4].x, -DSensorPoints[4].y,
                    DSensorPoints[5].x, -DSensorPoints[5].y
                    );

    createSensorHelper(info, 3, &(minMaxFRect[3]), decreasePosBy);

    simple_snprintf(info,
    "<svg viewBox=\"%.6f,%.6f,%.6f,%.6f\"><polygon points=\"%.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f\" fill=\"#fff\" stroke=\"#fff\" fill-opacity=\".1\" stroke-opacity=\".25\" stroke-width=\".005\"/></svg>",
                    minMaxFRect[4].x - decreasePosBy, minMaxFRect[4].y - decreasePosBy, minMaxFRect[4].w, minMaxFRect[4].h,
                    ESensorPoints[0].x, -ESensorPoints[0].y,
                    ESensorPoints[1].x, -ESensorPoints[1].y,
                    ESensorPoints[2].x, -ESensorPoints[2].y,
                    ESensorPoints[3].x, -ESensorPoints[3].y
                    );

    createSensorHelper(info, 4, &(minMaxFRect[4]), decreasePosBy);

    int total = 0;
    for(int i = 0; i < 5; i++){
        const float dirChange = i == 2 ? 180.f : 45.f; //C sensor moment
        const int maxN = i == 2 ? 2 : 8;
        for(int n = 0; n < maxN; n++){
            const SDL_FPoint oldXY = (SDL_FPoint){sensorFRects[i].x, sensorFRects[i].y};
            const SDL_FPoint newXY = rotateFPointBy(dirChange * n, oldXY);
            renderingSensorFPoints[total].x = (newXY.x + 1) * screenSize / 2.f;
            renderingSensorFPoints[total].y = (newXY.y + 1) * screenSize / 2.f;
            total++;
        }
    }
}

static void loadChartSoundEffects(void){
    if(tapNoteSfx == NULL){
        loadSound(&tapNoteSfx, "sfx/notes/tap.wav");
    }

    if(holdNoteSfx == NULL){
        loadSound(&holdNoteSfx, "sfx/notes/hold.wav");
    }

    if(slideStartSfx == NULL && enableSlideSFX){
        loadSound(&slideStartSfx, "sfx/notes/slidestart.wav");
    }

    if(breakNoteSfx == NULL){
        loadSound(&breakNoteSfx, "sfx/notes/break.wav");
    }

    if(touchNoteSfx == NULL){
        loadSound(&touchNoteSfx, "sfx/notes/touch.wav");
    }

    if(EXNoteSfx == NULL){
        loadSound(&EXNoteSfx, "sfx/notes/exnote.wav");
    }

    if(fireworkSfx == NULL && enableFireworksSFX){
        loadSound(&fireworkSfx, "sfx/notes/fireworks.wav");
    }

    if(fullComboSfx == NULL){
        loadSound(&fullComboSfx, "sfx/ingame/fullcombo.wav");
    }
}

static void loadNextButtonBox(void){
    loadImageSimple(&buttonBox, "gfx/buttonbox.svg", whScaled(buttonBoxWH.x, buttonBoxWH.y));
    loadTextLocalized(&buttonBoxTexts[3], mediumFont, "buttonnext", white, true);
}

static void loadButtonBoxes(void){
    loadNextButtonBox();
    loadTextLocalized(&buttonBoxTexts[0], mediumFont, "buttonoptions", white, true);
    loadTextLocalized(&buttonBoxTexts[4], mediumFont, "buttonscrolldown", white, true);
    loadTextLocalized(&buttonBoxTexts[5], mediumFont, "buttonback", white, true);
    loadTextLocalized(&buttonBoxTexts[7], mediumFont, "buttonscrollup", white, true);
}

static void loadBackgroundSimple(SDL_Texture **imageToLoad, const char *file, const char* extension, SDL_Rect rect){
    char *backgroundPath = joinPrefixMiddleEndCharArrays(file, "/bg.", extension);
    loadImageSimple(imageToLoad, backgroundPath, rect);

    SDL_free(backgroundPath);
}

static void loadSongInfo(void){
    loadTextLocalized(&userScreenScore, bigFont, "genericscore", white, false);
    loadTextSimple(&subScreenSongTitle, mediumFont, songs[currentSongIndex].songTitle, white, false);
    const char* diffLevel = songs[currentSongIndex].difficultyLevels[selectedDifficulty];
    if(diffLevel != NULL && SDL_strnlen(diffLevel, 1)){
        char* realLevel = SDL_strdup(songs[currentSongIndex].difficultyLevels[selectedDifficulty]);
        char* levelName = joinPrefixMiddleEndCharArrays(langGetStr("difficultylv"), " ", realLevel);
        loadTextSimple(&subScreenDifficultyLevel, mediumFont, levelName, white, false);
        SDL_free(realLevel);
        SDL_free(levelName);
    } else {
        destroyTextureIfNotNull(subScreenDifficultyLevel);
    }

    if(songs[currentSongIndex].difficultyNames[selectedDifficulty] == NULL){
        loadTextLocalized(&subScreenDifficultyName, smallFont, selectedDifficulty == 0 ? "difficultyeasy" :
                                                               selectedDifficulty == 1 ? "difficultybasic" :
                                                               selectedDifficulty == 2 ? "difficultyadvanced" :
                                                               selectedDifficulty == 3 ? "difficultyexpert" :
                                                               selectedDifficulty == 4 ? "difficultymaster" :
                                                               selectedDifficulty == 5 ? "difficultyremaster" :
                                                               "difficultyoriginal",
                                                               white, false);
    } else {
        loadTextSimple(&subScreenDifficultyName, smallFont, songs[currentSongIndex].difficultyNames[selectedDifficulty], white, false);
    }
    loadImageSimple(&songInfo, "gfx/ingame/songinfo.svg", whScaled(414, 104));
}

static void reloadCurrentProfileGraphics(void){
    loadImageSimple(&userCard, "gfx/usercard.svg", whScaled(340, 140));
    if(profiles[currentProfileIndex].hasProfilePicture){
        char* picturePath = joinPrefixMiddleEndCharArrays("profiles/", profiles[currentProfileIndex].profileLocation, "/profile.png");
        loadImageSimple(&profiles[currentProfileIndex].profileIcon, picturePath, whScaled(110, 110));
        SDL_free(picturePath);
    }
    updateRatingTexture(currentProfileIndex);
}

void loadInGame(int slot){
    UNUSED(slot);
    if(isLoadImagePart){
        destroyTextureIfNotNull(gameBackground);
        destroyTextureIfNotNull(buttonBox);
        destroyTextureIfNotNull(difficultyCircle);
        destroyStackArrayOfTextures(buttonBoxTexts);
        loadChartGraphics();

        const bool songHasPNG = songs[currentSongIndex].hasPNG;
        const bool songHasJPG = songs[currentSongIndex].hasJPG;
        const bool songHasBackground = songHasPNG || songHasJPG;

        #ifdef Circular8_ENABLE_FFmpeg
        const bool loadBackground = songHasBackground && SDL_round(backgroundBrightness * 255.0) > 0 && !isShowingVideo;
        #else
        const bool loadBackground = songHasBackground && SDL_round(backgroundBrightness * 255.0) > 0;
        #endif // Circular8_ENABLE_FFmpeg

        if(loadBackground){ //if 0% brightness, don't load, saving some texture memory
            loadBackgroundSimple(&gameBackground, songs[currentSongIndex].chartLocation, songHasPNG ? "png" : "jpg", whScaled(720, 720));
        }

        if(SDL_strncmp(currentJudgeLocaleName, "french", 2) == 0){
            const char* FCMedalNames[5] = {"gfx/fcmedals/french/zerorate.svg",
                                           "gfx/fcmedals/french/zerorateplus.svg",
                                           "gfx/fcmedals/french/tousparfaits.svg",
                                           "gfx/fcmedals/french/tousparfaitsplus.svg",
                                           "gfx/fcmedals/french/maximumdxpoints.svg"
                                  };
            const SDL_Rect FCMedalRects[5] = {whScaled(175.151, 184),
                                              whScaled(230.885, 184),
                                              whScaled(322.895, 184),
                                              whScaled(378.628, 184),
                                              whScaled(363.935, 184)
                                             };
            for(int i = 0; i < 5; i++){
                loadImageSimple(&FCMedals[i], FCMedalNames[i], FCMedalRects[i]);
            }
        } else {
            const char* FCMedalNames[5] = {"gfx/fcmedals/english/fc.svg",
                                           "gfx/fcmedals/english/fcplus.svg",
                                           "gfx/fcmedals/english/ap.svg",
                                           "gfx/fcmedals/english/applus.svg",
                                           "gfx/fcmedals/english/acp.svg"
                                  };
            const SDL_Rect FCMedalRects[5] = {whScaled(247.301, 184),
                                              whScaled(303.034, 184),
                                              whScaled(287.025, 184),
                                              whScaled(342.756, 184),
                                              whScaled(287.025, 274)
                                             };
            for(int i = 0; i < 5; i++){
                loadImageSimple(&FCMedals[i], FCMedalNames[i], FCMedalRects[i]);
            }
        }

        if(useSubscreen){
            loadSongInfo();
            reloadCurrentProfileGraphics();
        }
    } else {
        rendererVSync = optionIngameVSync;
        highScore = getBestScoreFromChecksumAndDifficulty(songs[currentSongIndex].checksumOfFile, selectedDifficulty);
        loadChartSoundEffects();
        addGameFunctionToCurrentSlot(inGame);
    }
}

static void loadJacketsAndTitlesForAllSongs(void){
    for(int i = 0; i < lengthOfSongs; i++){
        songs[i].titleText = loadText(smallFont, songs[i].songTitle, white);
        if(!enableSongSelectBGs){
            continue;
        }

        if(songs[i].hasPNG || songs[i].hasJPG){
            if(songs[i].background == NULL){
                loadBackgroundSimple(&songs[i].background, songs[i].chartLocation, songs[i].hasPNG ? "png" : "jpg", whScaled(112, 112));
            }
        }
    }

    loadButtonBoxes();
}

static SDL_EnumerationResult checkSongFeaturesCallback(void* userdata, const char* directoryName, const char* fileName){
    UNUSED(userdata);

    //inefficient but whatever
    char* fullPath = joinPrefixMiddleEndCharArrays(directoryName, "/", fileName);
    SDL_PathInfo fileInfo;
    SDL_GetPathInfo(fullPath, &fileInfo);
    const bool isDirectory = fileInfo.type == SDL_PATHTYPE_DIRECTORY;
    SDL_free(fullPath);

    if(isDirectory){
        return SDL_ENUM_CONTINUE;
    }

    if(SDL_strcmp(fileName, "bg.png") == 0){
        songs[lengthOfSongs].hasPNG = true;
    } else if(SDL_strcmp(fileName, "bg.jpg") == 0){
        songs[lengthOfSongs].hasJPG = true;
    } else if(SDL_strcmp(fileName, "track.mp3") == 0){
        songs[lengthOfSongs].hasMP3 = true;
    } else if(SDL_strcmp(fileName, "track.ogg") == 0){
        songs[lengthOfSongs].hasOGG = true;
    } else if(SDL_strcmp(fileName, "pv.mp4") == 0){
        songs[lengthOfSongs].hasMP4filePV = true;
    } else if(SDL_strcmp(fileName, "mv.mp4") == 0){
        songs[lengthOfSongs].hasMP4fileMV = true;
    } else if(SDL_strcmp(fileName, "bg.mp4") == 0){
        songs[lengthOfSongs].hasMP4fileBG = true;
    } else if(SDL_strcmp(fileName, "pv.webm") == 0){
        songs[lengthOfSongs].hasWebMfilePV = true;
    } else if(SDL_strcmp(fileName, "mv.webm") == 0){
        songs[lengthOfSongs].hasWebMfileMV = true;
    } else if(SDL_strcmp(fileName, "bg.webm") == 0){
        songs[lengthOfSongs].hasWebMfileBG = true;
    }
    return SDL_ENUM_CONTINUE;
}

static SDL_EnumerationResult loadSongsCallback(void* userdata, const char* directoryName, const char* fileName){
    UNUSED(userdata);
    SDL_PathInfo fileInfo;
    char* fullPath = joinPrefixMiddleEndCharArrays(directoryName, "/", fileName);
    SDL_GetPathInfo(fullPath, &fileInfo);
    if(fileInfo.type == SDL_PATHTYPE_DIRECTORY){
        SDL_EnumerateDirectory(fullPath, loadSongsCallback, NULL);
    } else {
        if(fileInfo.type == SDL_PATHTYPE_FILE && SDL_strcmp(fileName, "maidata.txt") == 0){
            SDL_free(fullPath);

            if(lengthOfSongs % 8 == 0){
                songFile* newPointer = (songFile*)reallocfTypeSDL((songs), sizeof(songFile) * (lengthOfSongs + 8));
                songs = newPointer;
            }
            songs[lengthOfSongs].chartLocation = SDL_strdup(directoryName);

            songs[lengthOfSongs].songTitle = "Song title not found"; //should it have strdup? //SDL_strdup("Song title not found");

            if(!getInfoAboutMaidatatxt(&songs[lengthOfSongs])){
                SDL_free(songs[lengthOfSongs].chartLocation);
                return SDL_ENUM_SUCCESS;
            }

            //initalize stuff
            songs[lengthOfSongs].hasPNG = false;
            songs[lengthOfSongs].hasJPG = false;
            songs[lengthOfSongs].hasMP3 = false;
            songs[lengthOfSongs].hasOGG = false;
            songs[lengthOfSongs].hasMP4filePV = false;
            songs[lengthOfSongs].hasMP4fileMV = false;
            songs[lengthOfSongs].hasMP4fileBG = false;
            songs[lengthOfSongs].hasWebMfilePV = false;
            songs[lengthOfSongs].hasWebMfileMV = false;
            songs[lengthOfSongs].hasWebMfileBG = false;
            songs[lengthOfSongs].background = NULL;

            SDL_EnumerateDirectory(directoryName, checkSongFeaturesCallback, NULL);

            lengthOfSongs++;
            return SDL_ENUM_SUCCESS;
        }
    }

    SDL_free(fullPath);
    return SDL_ENUM_CONTINUE;
}

static void loadMenuBGAndBorder(void){
    loadImageSimple(&menuBackground, "gfx/bg.jpg", whScaled(720, 720));
    loadImageSimple(&songBorder, "gfx/songborder.svg", whScaled(480, 128));
}

static int SDLCALL songSortingCompare(const void *a, const void *b) {
    const songFile* A = (const songFile*)a;
    const songFile* B = (const songFile*)b;
    return SDL_strcmp(A->songTitle, B->songTitle);
}

int SDLCALL profileSortingCompare(const void *a, const void *b) {
    const profileInfo* A = (const profileInfo*)a;
    const profileInfo* B = (const profileInfo*)b;
    return SDL_strcmp(A->name, B->name);
}

void loadSongSelect(int slot){
    UNUSED(slot);
    if(isLoadImagePart){
        if(lengthOfSongs <= 0){
            SDL_EnumerateDirectory("levels", loadSongsCallback, NULL);
            if(lengthOfSongs > 0){
                SDL_qsort(songs, lengthOfSongs, sizeof(songs[0]), songSortingCompare);
            }
        }
        loadJacketsAndTitlesForAllSongs();
        loadMenuBGAndBorder();
        if(useSubscreen){
            reloadCurrentProfileGraphics();
        }
    } else {
        if(numberOfProfiles > 0){
            //garbage workaround
            char* realFileName = joinTwoCharArraysToOne(profiles[currentProfileIndex].fullPath, "/user.ini");
            if(ini_parse(realFileName, profileHandler, NULL) < 0){
                printf("BAD INI\n");
            }
            SDL_free(realFileName);
            loadScoreFile();
        }

        //throw away useless textures to save some vram
        //options
        destroyStackArrayOfTextures(optionOffOnTexts);
        destroyStackArrayOfTextures(groupOptions);
        destroyStackArrayOfTextures(gameplayOptions);
        destroyStackArrayOfTextures(displayOptions);
        destroyStackArrayOfTextures(judgeOptions);
        destroyStackArrayOfTextures(audioOptions);

        //difficulty select
        destroyTextureIfNotNull(textLV);
        destroyTextureIfNotNull(decreaseDifficultyText);
        destroyTextureIfNotNull(increaseDifficultyText);

        addGameFunctionToCurrentSlot(songSelect);
    }
}

static void loadDiffTextures(void){
    loadButtonBoxes();

    loadImageSimple(&difficultyCircle, "gfx/difficultycircle.svg", whScaled(77, 77));
    loadImageSimple(&difficultyHighlight, "gfx/difficultyhighlight.svg", whScaled(90, 90));

    loadTextLocalized(&textLV, smallFont, "difficultylv", white, false);
    loadTextLocalized(&decreaseDifficultyText, mediumFont, "difficultydecrease", white, true);
    loadTextLocalized(&increaseDifficultyText, mediumFont, "difficultyincrease", white, true);
}

void loadDifficultySelect(int slot){
    (void)slot;
    if(isLoadImagePart){
        loadDiffTextures();
        loadMenuBGAndBorder();
        const int len = stackArraySizeInt(difficultyTextures);
        for(int i = 0; i < len; i++){
            destroyTextureIfNotNull(difficultyTextures[i]);
        }

        if(useSubscreen){
            reloadCurrentProfileGraphics();
        }
        printf("songs: %d\n", lengthOfSongs);
    } else {
        addGameFunctionToCurrentSlot(difficultySelect);
    }
}

void updateRatingTexture(int i){
    loadTextSimple(&profiles[i].nameTexture, mediumFont, profiles[i].name == NULL ? "(no name)" : profiles[i].name, white, false);
    char rating[32] = {0};
    char plays[32] = {0};

    separateNumberWithCorrectDecimal(rating, 32, profiles[i].rating, 2);
    separateNumberByThousands(plays, 32, profiles[i].playCount);
    char totalRating[128] = {0};
    if(enableRating){
        simple_snprintf(totalRating, "%s: %s\n%s: %s %s", langGetStr("genericrating"), rating,
                                                          langGetStr("profileplays"), plays,
                                                          profiles[i].playCount == 1 ? langGetStr("profiletimessingular") : langGetStr("profiletimesplural"));
    } else {
        simple_snprintf(totalRating, "%s: %s %s", langGetStr("profileplays"), plays,
                                                  profiles[i].playCount == 1 ? langGetStr("profiletimessingular") : langGetStr("profiletimesplural"));
    }

    loadTextSimple(&profiles[i].ratingTexture, smallFont, totalRating, white, false);
}

void loadProfileSelect(int slot){
    (void)slot;
    if(isLoadImagePart){
        loadImageSimple(&userCard, "gfx/usercard.svg", whScaled(340, 140));
        for(int i = 0; i < numberOfProfiles; i++){
            if(profiles[i].hasProfilePicture){
                char* picturePath = joinPrefixMiddleEndCharArrays("profiles/", profiles[i].profileLocation, "/profile.png");
                loadImageSimple(&profiles[i].profileIcon, picturePath, whScaled(110, 110));
                SDL_free(picturePath);
            }
            updateRatingTexture(i);
        }
        loadButtonBoxes();
    } else {
        addGameFunctionToCurrentSlot(profileSelect);
    }
}

void loadScoreScreen(int slot){
    (void)slot;
    if(isLoadImagePart){
        destroyChartGraphics();
        loadNextButtonBox();
        const char* ranks[] = {"SSS+", "SSS",
                               "SS+",  "SS",
                               "S+",   "S",
                               "AAA",  "AA", "A",
                               "BBB",  "BB", "B",
                               "C",    "D"};



        const bool hasPNG = songs[currentSongIndex].hasPNG;
        const bool hasJPG = songs[currentSongIndex].hasJPG;
        if(hasPNG || hasJPG){
            loadBackgroundSimple(&scoreScreenSongBG, songs[currentSongIndex].chartLocation, hasPNG ? "png" : "jpg", whScaled(200.f, 200.f));
        }

        char infoStr[64];
        char extraStr[16];

        const char* genericScore = langGetStr("genericscore");
        const char* genericDXScore = langGetStr("genericdxscore");

        separatePercentageWithCorrectDecimal(extraStr, 16, calculateScore(currentBaseScore, absoluteMaxBaseScore, currentBreakBonus, absoluteMaxBreakBonus), 4);
        simple_snprintf(infoStr, "%s: %s", genericScore, extraStr);

        loadTextSimple(&scoreScreenScore, bigFont, infoStr, white, false);

        simple_snprintf(infoStr, "%s: %s", langGetStr("scorerank"), ranks[rankIndex]);
        loadTextSimple(&scoreScreenRank, bigFont, infoStr, white, false);

        const SDL_Point DXStars = getMaxDXStarsAndHP(currentDXScore, absoluteMaxDXScore, 6);
        if(DXStars.x >= 6){
            simple_snprintf(extraStr, "%s%u", langGetStr("genericmaxminus"), absoluteMaxDXScore - currentDXScore);
        } else {
            separatePercentageWithCorrectDecimal(extraStr, 16, SDL_trunc(currentDXScore * 1000000.0 / SDL_max(1, absoluteMaxDXScore)) / 10000.0, 4);
        }
        simple_snprintf(infoStr, "%s: %u/%u (%s", genericDXScore, currentDXScore, absoluteMaxDXScore, extraStr);
        calculateDXStars(infoStr, stackArraySize(infoStr), currentDXScore, absoluteMaxDXScore);
        loadTextSimple(&scoreScreenDXScore, smallFont, infoStr, white, false);


        if(currentMaxDXScore == absoluteMaxDXScore){
             const int FCBadgeI = getFCMedal();

            if(FCBadgeI >= 0){
                const char* FCBadges[] = {"genericfc",
                                          "genericfcplus", "genericap",
                                          "genericapplus", "genericacp"};
                loadTextLocalized(&scoreScreenFCBadge, mediumFont, FCBadges[FCBadgeI], white, false);
            } else {
                destroyTextureIfNotNull(scoreScreenFCBadge);
            }
        } else { //track skipped
            //texture naming moment
            loadTextLocalized(&scoreScreenFCBadge, mediumFont, "trackskipped", white, false);
        }


        if(numberOfProfiles > 0){
            if(!inScoreScreen){
                inScoreScreen = true;
                const double oldRating = SDL_floor(profiles[currentProfileIndex].rating * 100);
                updateScoreFile(); //todo: figure out rating issues
                addedRating = SDL_floor(profiles[currentProfileIndex].rating * 100) - oldRating;
                updateProfile(true);
            }

            if(addedRating >= 1){
                simple_snprintf(infoStr, "+%.2g %s!", addedRating / 100.0, langGetStr("genericrating"));
                loadTextSimple(&addedRatingTexture, smallFont, infoStr, white, false);
            } else {
                destroyTextureIfNotNull(addedRatingTexture);
            }
        }
        loadSongInfo();
        if(useSubscreen){
            reloadCurrentProfileGraphics();
        }
    } else {
        addGameFunctionToCurrentSlot(scoreScreen);
        rendererVSync = optionVSync;
        #ifdef Circular8_ENABLE_FFmpeg
        endVideoThread();
        #endif // Circular8_ENABLE_FFmpeg
    }
}

void loadGameOptions(int slot){
    //living hell
    (void)slot;
    if(isLoadImagePart){
        loadButtonBoxes();
        loadTextLocalized(&changeMinus, mediumFont, "optiondecrease", white, true);
        loadTextLocalized(&changePlus, mediumFont, "optionincrease", white, true);

        loadTextLocalized(&optionOffOnTexts[0], mediumFont, "genericoff", white, false);
        loadTextLocalized(&optionOffOnTexts[1], mediumFont, "genericon", white, false);

        loadTextLocalized(&groupOptions[0], mediumFont, "groupoptiongameplay", white, false);
        loadTextLocalized(&groupOptions[1], mediumFont, "groupoptiondisplay", white, false);
        loadTextLocalized(&groupOptions[2], mediumFont, "groupoptionjudge", white, false);
        loadTextLocalized(&groupOptions[3], mediumFont, "groupoptionaudio", white, false);

        loadTextLocalized(&gameplayOptions[0], mediumFont, "optiontapspeed", white, false);
        loadTextLocalized(&gameplayOptions[1], mediumFont, "optiontouchspeed", white, false);
        loadTextLocalized(&gameplayOptions[2], mediumFont, "optionsliderotation", white, false);
        loadTextLocalized(&gameplayOptions[3], mediumFont, "optionmirror", white, false);
        loadTextLocalized(&gameplayOptions[4], mediumFont, "optiontrackskip", white, false);

        //mirror
        loadTextLocalized(&mirrorTexts[0], mediumFont, "mirrorfliphorizontally", white, false);
        loadTextLocalized(&mirrorTexts[1], mediumFont, "mirrorflipvertically", white, false);
        loadTextLocalized(&mirrorTexts[2], mediumFont, "mirrorflip180", white, false);

        //track skip
        loadTextLocalized(&trackSkipTexts[0], mediumFont, "trackskipbuttons", white, false);
        loadTextLocalized(&trackSkipTexts[1], mediumFont, "trackskipnotouch", white, false);
        loadTextLocalized(&trackSkipTexts[2], mediumFont, "trackskipeither", white, false);

        //display options
        loadTextLocalized(&displayOptions[0], mediumFont, "optionbgbrightness", white, false);
        loadTextLocalized(&displayOptions[1], mediumFont, "optionaltstarnote", white, false);
        loadTextLocalized(&displayOptions[2], mediumFont, "optionsensors", white, false);
        loadTextLocalized(&displayOptions[3], mediumFont, "optioncenterdisplay", white, false);

        //alt star note
        loadTextLocalized(&slideTapNoteNames[0], mediumFont, "normalstarnotecolor", white, false);
        loadTextLocalized(&slideTapNoteNames[1], mediumFont, "tapstarnotecolor", white, false);

        //sensor types
        loadTextLocalized(&sensorTypes[0], mediumFont, "optionsensorsstatic", white, false);
        loadTextLocalized(&sensorTypes[1], mediumFont, "optionsensorsinteractive", white, false);
        loadTextLocalized(&sensorTypes[2], mediumFont, "optionsensorsontouch", white, false);

        const char* displayTypeChars[] = {
            "displayatcentercombo",
            "displayatcenterscoreplus", //"Achievement +",
            "displayatcenterscoreavg", //"Achievement Avg",
            "displayatcenterscorecomeback",//"Achievement -1",
            "displayatcenter101minusmistakes", //"Achievement -2",
            "displayatcenterborders",
            "displayatcenterborderss",
            "displayatcenterbordersss",
            "displayatcenterbordermybest",
            "displayatcenterdxscoreplus",
            "displayatcenterdxscoreminus",
            "displayatcenterdxscorestars"
        };

        for(unsigned int i = 0; i < stackArraySize(displayTypeChars); i++){
            loadTextLocalized(&achievementTexts[i], mediumFont, displayTypeChars[i], white, false);
        }

        loadTextLocalized(&judgeOptions[0], mediumFont, "optionshowfastlate", white, false);
        loadTextLocalized(&judgeOptions[1], mediumFont, "optionshowcritperf", white, false);
        loadTextLocalized(&judgeOptions[2], mediumFont, "optionchartoffset", white, false);
        loadTextLocalized(&judgeOptions[3], mediumFont, "optioninputoffset", white, false);

        //judge display speed
        loadTextLocalized(&judgeDisplaySpeedTypes[0], mediumFont, "judgedisplayslowest", white, false);
        loadTextLocalized(&judgeDisplaySpeedTypes[1], mediumFont, "judgedisplayslower", white, false);
        loadTextLocalized(&judgeDisplaySpeedTypes[2], mediumFont, "judgedisplaynormal", white, false);
        loadTextLocalized(&judgeDisplaySpeedTypes[3], mediumFont, "judgedisplayfaster", white, false);
        loadTextLocalized(&judgeDisplaySpeedTypes[4], mediumFont, "judgedisplayfastest", white, false);


        //fast late options:
        loadTextLocalized(&fastLateTypes[0], mediumFont, "fastlatenormal", white, false);
        loadTextLocalized(&fastLateTypes[1], mediumFont, "fastlateexclusive", white, false);
        loadTextLocalized(&fastLateTypes[2], mediumFont, "fastlatebreakonly", white, false);

        //critical perfect
        loadTextLocalized(&criticalPerfectTypes[0], mediumFont, "critperfnormal", white, false);
        loadTextLocalized(&criticalPerfectTypes[1], mediumFont, "critperfinvisible", white, false);

        loadTextLocalized(&audioOptions[0], mediumFont, "optionsfxvolume", white, false);
        loadTextLocalized(&audioOptions[1], mediumFont, "optionbgmvolume", white, false);
        loadTextLocalized(&audioOptions[2], mediumFont, "optionplayfireworkssfx", white, false);
        loadTextLocalized(&audioOptions[3], mediumFont, "optionplayslidesfx", white, false);
        loadTextLocalized(&audioOptions[4], mediumFont, "optionguideoffset", white, false);

        if(useSubscreen){
            reloadCurrentProfileGraphics();
        }
    } else {
        addGameFunctionToCurrentSlot(gameOptions);
    }
}
