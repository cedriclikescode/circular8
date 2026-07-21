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
#include <assert.h>

#include "events.h"
#include "render.h"
#include "load.h"
#include "miscutils.h"
#ifdef Circular8_ENABLE_CONTROLLER
#include "controller/led.h"
#endif // Circular8_ENABLE_CONTROLLER
#include "game/chart.h"
#include "game/ingame.h"
#include "game/rendersong.h"
#include "game/options.h"
#include "game/scorescreen.h"

//three tap notes: normal, BREAK, and EACH
//also EX notes
//tap
/*SDL_Texture* normalTapNote = NULL;
SDL_Texture* breakTapNote = NULL;
SDL_Texture* eachTapNote = NULL;
SDL_Texture* EXNoteBlur = NULL;

//slide tap
SDL_Texture* normalSlideNote = NULL;
SDL_Texture* breakSlideNote = NULL;
SDL_Texture* eachSlideNote = NULL;
//multi slide tap
SDL_Texture* normalMultiSlideNote = NULL;
SDL_Texture* breakMultiSlideNote = NULL;
SDL_Texture* eachMultiSlideNote = NULL;

//hold
SDL_Texture* normalHoldNoteSize0 = NULL; //the bane of my existence 1
SDL_Texture* breakHoldNoteSize0 = NULL; //the bane of my existence 2
SDL_Texture* eachHoldNoteSize0 = NULL; //the bane of my existence 3

SDL_Texture* normalHoldNoteSize1 = NULL; //the bane of my existence 4
SDL_Texture* breakHoldNoteSize1 = NULL; //the bane of my existence 5
SDL_Texture* eachHoldNoteSize1 = NULL; //the bane of my existence 6
SDL_Texture* unheldHoldNoteSize1 = NULL; //the bane of my existence 7

SDL_Texture* holdNoteHitMarker = NULL; //the bane of my existence 8

//slide arrows
SDL_Texture* normalSlideArrow = NULL; //the actual bane of my existence 1
SDL_Texture* breakSlideArrow = NULL; //the actual bane of my existence 2
SDL_Texture* eachSlideArrow = NULL; //the actual bane of my existence 3

//the finale of the slides
SDL_Texture* normalFanSlideArrows[11] = {0};
SDL_Texture* breakFanSlideArrows[11] = {0};
SDL_Texture* eachFanSlideArrows[11] = {0};

//touch notes
SDL_Texture* normalTouchPoint = NULL;
SDL_Texture* eachTouchPoint = NULL;

SDL_Texture* normalTouchArrow = NULL;
SDL_Texture* eachTouchArrow = NULL;
//when there are multiple touches in same area
SDL_Texture* multiNormalTouchBox = NULL;
SDL_Texture* multiEachTouchBox = NULL;*/

//fireworks
//SDL_Texture* fireworksTexture = NULL;
int fireworksTouchLocation = 0;
double fireworksStartTime = -99999;
/*
//hold touch note
SDL_Texture* holdTouchTriangles[4] = {0};
SDL_Texture* holdTouchProgression = NULL;
SDL_Texture* holdTouchProgressionUnheld = NULL;

//judgements
SDL_Texture* judgements[5] = {0}; //miss, good, great, perfect, critical perfect
SDL_Texture* alternativeJudgements[5] = {0}; //miss, good, great, perfect, critical perfect
SDL_Texture* fastLateJudgements[2] = {0}; //fast, late

SDL_Texture* slideBigJudgements[3] = {0}; //line, arc, fan
SDL_Texture* slideInfoJudgements[2] = {0}; //Too late, Just, Critical is in criticalJudge and fast/late is used in fastLateJudgements
SDL_Texture* criticalJudge = NULL;*/

//texture atlas

int normalTapNoteID = 0;
int breakTapNoteID = 0;
int eachTapNoteID = 0;
int EXNoteBlurID = 0;

//slide tap
int normalSlideNoteID = 0;
int breakSlideNoteID = 0;
int eachSlideNoteID = 0;
//multi slide tap
int normalMultiSlideNoteID = 0;
int breakMultiSlideNoteID = 0;
int eachMultiSlideNoteID = 0;

//hold
int normalHoldNoteSize0ID = 0; //the bane of my existence 1
int breakHoldNoteSize0ID = 0; //the bane of my existence 2
int eachHoldNoteSize0ID = 0; //the bane of my existence 3

int normalHoldNoteSize1ID = 0; //the bane of my existence 4
int breakHoldNoteSize1ID = 0; //the bane of my existence 5
int eachHoldNoteSize1ID = 0; //the bane of my existence 6
int unheldHoldNoteSize1ID = 0; //the bane of my existence 7

int holdNoteHitMarkerID = 0; //the bane of my existence 8

//slide arrows
int normalSlideArrowID = 0; //the actual bane of my existence 1
int breakSlideArrowID = 0; //the actual bane of my existence 2
int eachSlideArrowID = 0; //the actual bane of my existence 3

//the finale of the slides
int normalFanSlideArrowsStartID = 0;
int breakFanSlideArrowsStartID = 0;
int eachFanSlideArrowsStartID = 0;

//touch notes
int normalTouchPointID = 0;
int eachTouchPointID = 0;

int normalTouchArrowID = 0;
int eachTouchArrowID = 0;
//when there are multiple touches in same area
int multiNormalTouchBoxID = 0;
int multiEachTouchBoxID = 0;

//fireworks
SDL_Texture* fireworksTexture = NULL; //not atlas right now

//hold touch note
int holdTouchTrianglesStartID = 0;
int holdTouchProgressionID = 0;
int holdTouchProgressionUnheldID = 0;

//judgements
int judgementsStartID = 0; //miss, good, great, perfect, critical perfect
int alternativeJudgementsStartID = 0; //miss, good, great, perfect, critical perfect
int fastLateJudgementsStartID = 0; //fast, late

int slideBigJudgementsStartID = 0; //line, arc, fan
int slideInfoJudgementsStartID = 0; //Too late, Just, Critical is in criticalJudge and fast/late is used in fastLateJudgements
int criticalJudgeID = 0;



//sensors
SDL_Texture* sensors[5] = {0};
SDL_FRect sensorFRects[5] = {0};

SDL_FPoint renderingSensorFPoints[34] = {0};

//other
SDL_Texture* gameBorder = NULL;
SDL_Texture* FCMedals[5] = {0};

int minIndexToRender = 0;

static int judgeToJudgeIndex(int judge){
    int judgeIndex = -1;
    switch(judge){
        case judgeMiss:
            judgeIndex = 0; //Miss...
            break;
        case judgeGood:
            judgeIndex = 1; //Good
            break;
        case judgeGreatLow:
        case judgeGreatMid:
        case judgeGreatHigh:
            judgeIndex = 2; //Great
            break;
        case judgePerfectLow:
        case judgePerfectHigh:
            judgeIndex = 3; //Perfect
            break;
        case judgePerfectCritical:
            judgeIndex = 4; //Critical Perfect
            break;
        default:
            setError("invalid judge type: %d", judge);
            break;
    }
    return judgeIndex;
}

static double getTouchDist(double farDist, double currentNoteTime, double movementTime){
    double touchDist = farDist;
    if(currentNoteTime >= movementTime) {
        const double progress = (currentNoteTime - movementTime) / movementTime;
        touchDist = SDL_max(0, touchDist - touchDist * (progress * progress));
    }
    return touchDist;
}

static void renderTouchHold(double currentNoteTime, double movementTime, const SDL_FPoint* touchFPoint, int touchLocation, double endLength, bool isUnheld){
    const Uint8 touchAlpha = convertTimerAndDurationToBrightness(currentNoteTime, movementTime);
    const float touchDist = getTouchDist(16.0, currentNoteTime, movementTime);
    for(int n = 0; n < 4; n++){
        const SDL_FPoint triangleFPoint = moveXYWithDirAndDistFloat(touchFPoint->x, touchFPoint->y, n * 90.f + 45.f, (39.f + touchDist) * guiScale);
        setAlphaOfAtlas(0, holdTouchTrianglesStartID + n, touchAlpha);
        renderSprite(0, holdTouchTrianglesStartID + n, triangleFPoint.x, triangleFPoint.y, true);
        setAlphaOfAtlas(0, holdTouchTrianglesStartID + n, 255);
        setAlphaOfAtlas(0, holdTouchTrianglesStartID + n, 255);
    }


    const float progression = (currentNoteTime - movementTime * 2.0) / endLength;
    if(progression <= 0.f){
        return;
    }

    float texW = 0.f;
    float texH = 0.f;
    SDL_Texture* texture = NULL;
    SDL_FRect src = {0.f, 0.f, 0.f, 0.f};
    getAtlasTextureAndSrc(0, isUnheld ? holdTouchProgressionUnheldID : holdTouchProgressionID, &texture, &src);
    texW = src.w;
    texH = src.w;

    SDL_FRect dstPart1 = {screenXOffset + touchLocations[touchLocation].x * screenSize,
                          screenYOffset + touchLocations[touchLocation].y * screenSize - texH / 2.f,
                          texW * SDL_min(progression, 0.25f) * 2.f, texH / 2.f};

    if(progression > 0.25f){
        dstPart1.h = texH * SDL_min(1.f, progression * 2.f);
    }

    //danger
    SDL_FRect srcPart1 = {src.x + texW / 2.f, src.y,
                          dstPart1.w, dstPart1.h};

    renderTexture(texture, &srcPart1, &dstPart1);

    if(progression <= 0.5f){
        return;
    }

    const float texW2 = SDL_min(1.f, (progression - 0.5f) * 4.f) * texW / 2.f;
    SDL_FRect dstPart2 = {dstPart1.x - texW2,
                          dstPart1.y + texH / 2.f,
                          texW2, texH / 2.f};

    float increaseHeightBy = 0.f;
    if(progression > 0.75f){
        increaseHeightBy = SDL_min(1.f, (progression - 0.75f) * 4.f) * dstPart2.h;
        dstPart2.y -= increaseHeightBy;
        dstPart2.h += increaseHeightBy;
    }

    //danger
    SDL_FRect srcPart2 = {SDL_max(src.x, srcPart1.x - texW2), src.y + texH / 2.f - increaseHeightBy,
                          texW2, dstPart2.h};

    renderTexture(texture, &srcPart2, &dstPart2);
    /*if(progression > 0.f){
        float texW = 0.f;
        float texH = 0.f;
        SDL_Texture* texture;
        SDL_FRect src;
        getAtlasTextureAndSrc(0, isUnheld ? holdTouchProgressionUnheldID : holdTouchProgressionID, &texture, &src);
        texW = src.w;
        texH = src.w;

        SDL_FRect dstPart1 = {screenXOffset + touchLocations[touchLocation].x * screenSize,
                              screenYOffset + touchLocations[touchLocation].y * screenSize - texH / 2.f,
                              texW * SDL_min(progression, 0.25f) * 2.f, texH / 2.f};

        if(progression > 0.25f){
            dstPart1.h = texH * SDL_min(1.f, progression * 2.f);
        }

        //danger
        SDL_FRect srcPart1 = {src.x + texW / 2.f, src.y,
             dstPart1.w,
             dstPart1.h};

        renderTexture(texture, &srcPart1, &dstPart1);

        if(progression > 0.5f){
            const float texW2 = SDL_min(1.f, (progression - 0.5f) * 4.f) * texW / 2.f;
            SDL_FRect dstPart2 = {dstPart1.x - texW2,
                                  dstPart1.y + texH / 2.f,
                                  texW2, texH / 2.f};

            float increaseHeightBy = 0.f;
            if(progression > 0.75f){
                increaseHeightBy = SDL_min(1.f, (progression - 0.75f) * 4.f) * dstPart2.h;
                dstPart2.y -= increaseHeightBy;
                dstPart2.h += increaseHeightBy;
            }

            //danger
            SDL_FRect srcPart2 = {SDL_max(src.x, srcPart1.x - texW2), src.y + texH / 2.f - increaseHeightBy,
                 texW2,
                 dstPart2.h};

            renderTexture(texture, &srcPart2, &dstPart2);
        }
    }*/
}

static void renderTouch(double currentNoteTime, double movementTime, const SDL_FPoint *touchFPoint, bool isEach){
    const Uint8 touchAlpha = convertTimerAndDurationToBrightness(currentNoteTime, movementTime);
    const float touchDist = getTouchDist(18.0, currentNoteTime, movementTime);


    const int touchArrowID = isEach ? eachTouchArrowID : normalTouchArrowID;
    setAlphaOfAtlas(0, touchArrowID, touchAlpha);
    for(int n = 0; n < 4; n++){
        const SDL_FPoint newFPoint = moveXYWithDirAndDistFloat(touchFPoint->x, touchFPoint->y, n * 90.f, (18.f + touchDist) * guiScale);
        renderSpriteEX(0, touchArrowID, newFPoint.x, newFPoint.y, 1.f, true, n * 90.0, SDL_FLIP_NONE);
    }
    setAlphaOfAtlas(0, touchArrowID, 255);
}

static void renderTouchBox(int touchLocation, float scale, bool isEach){
    renderSpriteEX(0, isEach ? multiEachTouchBoxID : multiNormalTouchBoxID,
                   screenXOffset + touchLocations[touchLocation].x * screenSize,
                   screenYOffset + touchLocations[touchLocation].y * screenSize,
                   scale, true, 0.0, SDL_FLIP_NONE);
}

static void renderFireworks(double currentTime, double startTime, int touch){
    const double elapsedTime = (double)currentTime - (double)startTime;
    const Uint8 fireworksAlpha = fadeInFadeOutAlpha(elapsedTime, 0.05, 0.30, 0.5);
    if(fireworksAlpha <= 0){
        return;
    }
    const float sizeOfFireworks = 0.5f + 0.5f * ((float)SDL_min(elapsedTime, 0.15) / 0.15f);

    const SDL_FPoint location = {touchLocations[touch].x, touchLocations[touch].y};

    const float mainX = screenXOffset + location.x * screenSize;
    const float mainY = screenYOffset + location.y * screenSize;
    const SDL_FRect baseFRect = xywh(mainX, mainY, screenSize, screenSize);

    const SDL_FRect fireworksFRect = FRectSimple(&baseFRect, sizeOfFireworks, true);
    const double direction = elapsedTime * 450.0;

    setTextureAlpha(fireworksTexture, fireworksAlpha);
    renderTextureRotated(fireworksTexture, NULL, &fireworksFRect, direction, NULL, SDL_FLIP_NONE);
}

//precomputed tables for note anchoring

static const float sinNoteTable[8] = {0.38268343236508977,
                                      0.92387953251128676,
                                      0.92387953251128676,
                                      0.38268343236508977,
                                     -0.38268343236508977,
                                     -0.92387953251128676,
                                     -0.92387953251128676,
                                     -0.38268343236508977
                                  };

static const float cosNoteTable[8] = {0.92387953251128676,
                                      0.38268343236508977,
                                     -0.38268343236508977,
                                     -0.92387953251128676,
                                     -0.92387953251128676,
                                     -0.38268343236508977,
                                      0.38268343236508977,
                                      0.92387953251128676
};

SDL_FRect moveNoteObject(SDL_Texture *texture, float x, float y, float scale, int noteLocation, float distance){
    float texW = 0;
    float texH = 0;
    if(texture != NULL){
        SDL_GetTextureSize(texture, &texW, &texH);
    } else {
        texW = 1;
        texH = 1;
    }

    SDL_FRect resultRect = {x + sinNoteTable[noteLocation - 1] * distance, y - cosNoteTable[noteLocation - 1] * distance, texW, texH};
    resultRect.w *= scale;
    resultRect.h *= scale;

    resultRect.x -= resultRect.w / 2.f;
    resultRect.y -= resultRect.h / 2.f;

    return resultRect;
}

SDL_FPoint moveNoteXY(float x, float y, int noteLocation, float distance){
    if_unlikely(noteLocation <= 0 || noteLocation > 8){
        return ((SDL_FPoint){-1, -1});
    }
    SDL_FPoint resultFPoint = {x + sinNoteTable[noteLocation - 1] * distance, y - cosNoteTable[noteLocation - 1] * distance};

    return resultFPoint;
}

static void renderSingleSlideJudge(const SDL_FPoint bigJudge, const SDL_FPoint infoJudge,
                                   double bigJudgeDir, double infoJudgeDir,
                                   int slideType, int slideJudge,
                                   int judgeID, Uint8 fadingSlide,
                                   bool flipX){
    const SDL_Color slideJudgementColors[5] = {
        {128, 128, 128, 255}, //Too late (Miss)
        {  0, 115,  57, 255}, //Good
        {128,   0, 128, 255}, //Great
        {128,  64,   0, 255}, //Perfect
        {191, 191,   0, 255}  //Critical Perfect
    };

    assert(slideJudge >= 0 && slideJudge < 5);

    setColorOfAtlas(0, slideBigJudgementsStartID + slideType, slideJudgementColors[slideJudge].r,
                                                              slideJudgementColors[slideJudge].g,
                                                              slideJudgementColors[slideJudge].b);
    setAlphaOfAtlas(0, slideBigJudgementsStartID + slideType, fadingSlide);
    renderSpriteEX(0, slideBigJudgementsStartID + slideType, bigJudge.x, bigJudge.y, 1.f, true, bigJudgeDir, flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    setAlphaOfAtlas(0, slideBigJudgementsStartID + slideType, 255);
    setColorOfAtlas(0, slideBigJudgementsStartID + slideType, 255, 255, 255);

    setAlphaOfAtlas(0, judgeID, fadingSlide);
    renderSpriteEX(0, judgeID, infoJudge.x, infoJudge.y, 1.f, true, infoJudgeDir, SDL_FLIP_NONE);
    setAlphaOfAtlas(0, judgeID, 255);
}

static void renderSlideJudgements(Sint64 gameTime){
    const float centerX = getCenterX();
    const float centerY = getCenterY();
    for(int i = 0; i < lenOfJudgementInfoForSlideEnd; i++){
        const Sint64 slideEndJudgeDisplayTime = (gameTime - judgementInfoForSlideEnd[i].properties.judgementDisplayTimer);
        const int slideTrueJudge = judgementInfoForSlideEnd[i].properties.judgementType;

        const Sint64 maxDisplayTime = gameTicksPerSecond / 3;
        if(slideEndJudgeDisplayTime < 0 || slideEndJudgeDisplayTime > maxDisplayTime || (showCriticalPerfect == 2 && slideTrueJudge >= judgePerfectCritical)){
            continue;
        }

        const Sint64 fadeCompleteTime = gameTicksPerSecond / 12;


        const Uint8 fadingSlide = fadeInFadeOutAlpha(slideEndJudgeDisplayTime, fadeCompleteTime, maxDisplayTime - fadeCompleteTime, maxDisplayTime);
        const int location = judgementInfoForSlideEnd[i].location;
        const double buttonDegree = buttonNumberToDegrees(location);
        const SDL_FPoint buttonFPoint = moveNoteXY(centerX, centerY, location, borderRadius * guiScale);


        const int slideDirection = judgementInfoForSlideEnd[i].properties.judgementDirection;

        const int judgeID = slideTrueJudge <= judgeMiss ? slideInfoJudgementsStartID :
                            slideTrueJudge < judgePerfectCritical ?
                            (fastLateJudgementsStartID + (slideDirection >= judgeLate)) :
                            (showCriticalPerfect ? criticalJudgeID :
                             slideInfoJudgementsStartID + 1);

        const int slideJudge = SDL_min(judgeToJudgeIndex(slideTrueJudge), 3 + showCriticalPerfect);

        switch(judgementInfoForSlideEnd[i].drawType){
            case judgeSlideLine:{
                const double degree = judgementInfoForSlideEnd[i].special.angle;


                //printf("degreeFromCenter: %.2f\n", degreeFromCenter);
                double degreeOffset = degree > 90.5 && degree < 270.5 ? 180 : 0;// ? 180.0 : 0.0;

                //exceptions
                if(location >= 5 && location <= 8){
                    if(degree > 89.5 && degree < 90.5){ //example: slide 8-5
                        degreeOffset = 180;
                    }
                } else {
                    if(degree > 269.5 && degree < 270.5){ //example: slide 4-1
                        degreeOffset = 0.0;
                    }
                }

                const int lineSlideID = slideBigJudgementsStartID + judgeSlideLine;
                SDL_FRect src = {0.f, 0.f, 0.f, 0.f};
                getAtlasTextureAndSrc(0, lineSlideID, NULL, &src);
                src.w /= -1.75f;
                const SDL_FPoint bigJudgeFPoint = moveXYWithDirAndDistFloat(buttonFPoint.x, buttonFPoint.y, degree + 90.f, src.w);
                const SDL_FPoint infoJudgeFPoint = moveXYWithDirAndDistFloat(buttonFPoint.x, buttonFPoint.y, degree + 90.f, src.w + 15.f * guiScale);

                renderSingleSlideJudge(bigJudgeFPoint, infoJudgeFPoint, degree, degree + degreeOffset, judgeSlideLine, slideJudge, judgeID, fadingSlide, false);
            }
                break;
            case judgeSlideArc:{

                const bool flipX = judgementInfoForSlideEnd[i].special.arcSlideFlipHorizontally;
                const SDL_FPoint buttonLeftFPoint = moveNoteXY(centerX, centerY, offsetButton(location, flipX ? 0 : 7), borderRadius * guiScale);

                //todo: change function to be better documented (works fine already)
                const double multiplier = flipX ? 1 : -1;
                const double multiplier2 = flipX ? 1.0 / 1.175 : 1.175;
                const double multiplier3 = flipX ? 0.575 : multiplier2 * 1.04;
                const double multiplier4 = flipX ? 2.1 : 1.055;
                const double multiplier5 = flipX ? 1.9 : 1.2;

                SDL_FRect src = {0.f, 0.f, 0.f, 0.f};
                getAtlasTextureAndSrc(0, slideBigJudgementsStartID + judgeSlideArc, NULL, &src);

                const SDL_FPoint bigJudgeFPoint = moveXYWithDirAndDistFloat(buttonLeftFPoint.x, buttonLeftFPoint.y, buttonDegree + (90 + 22.5 * multiplier2) * multiplier, (src.h / 2.f + src.w) / 2.15f * multiplier);
                const SDL_FPoint infoJudgeFPoint = moveXYWithDirAndDistFloat(buttonLeftFPoint.x, buttonLeftFPoint.y, buttonDegree + (90 + 22.5 * multiplier3) * multiplier, (src.w + src.h) / 1.7f / multiplier4 * multiplier);

                renderSingleSlideJudge(bigJudgeFPoint, infoJudgeFPoint, buttonDegree + 22.5 * multiplier, buttonDegree + ((22.5 / 2) * multiplier2 * multiplier5) * multiplier, judgeSlideArc, slideJudge, judgeID, fadingSlide, flipX);
            }
                break;
            case judgeSlideFan:
                renderSingleSlideJudge(moveNoteXY(centerX, centerY, location, (borderRadius - 23.f) * guiScale),
                                       moveNoteXY(centerX, centerY, location, (borderRadius - 4.f) * guiScale),
                                       buttonDegree,
                                       buttonDegree,
                                       judgeSlideFan,
                                       slideJudge,
                                       judgeID,
                                       fadingSlide,
                                       false);
                break;
            default:
                printf("illegal slide judgement %d\n", judgementInfoForSlideEnd[i].drawType);
        }
    }
}

static bool shouldShowRegularJudgement(int judgeIndex){
    const int maxJudge = 3 + (showCriticalPerfect > 0);
    return (showCriticalPerfect != 2 || judgeIndex < 4) &&
           (showFastLate != 2 || judgeIndex >= maxJudge || judgeIndex <= judgeMiss);
}

static void renderTouchJudgements(int touch, int judgeIndex, double judgeDisplayTime){
    const float touchJudgeScale = 0.8f + 0.2f * ((float)SDL_min(judgeDisplayTime, 0.050) / 0.050f);
    const SDL_FPoint center = {0.5f, 0.5f};
    const SDL_FPoint location = {touchLocations[touch].x, touchLocations[touch].y};

    const float mainX = screenXOffset + location.x * screenSize;
    const float mainY = screenYOffset + location.y * screenSize;

    const double pointTowards = touch / 8 == 2 ? 0.f : //C sensor
                                startFPPointsTowardEndFP(&center, &location);
    const int maxJudge = 3 + (showCriticalPerfect > 0);
    if(shouldShowRegularJudgement(judgeIndex)){
        renderSpriteEX(0, judgementsStartID + judgeIndex, mainX, mainY, touchJudgeScale, true, pointTowards, SDL_FLIP_NONE);
    }

    const bool displayCritical = showCriticalPerfect == 1 && judgeIndex == 4 && (showFastLate != 2 || judgeIndex >= maxJudge);
    const int judgeSpeedIndex = judgeIndex >= maxJudge ? judgeCritical : judgeLate;
    const bool displayFastLate = judgeSpeedIndex != 0 && judgeIndex >= 1 && showFastLate;
    SDL_FRect judgementSrc = {0.f, 0.f, 0.f, 0.f};
    if(displayCritical || displayFastLate){
        getAtlasTextureAndSrc(0, judgementsStartID + judgeIndex, NULL, &judgementSrc);
    }

    if(displayCritical){
        const SDL_FPoint criticalFPoint = moveXYWithDirAndDistFloat(mainX, mainY, pointTowards, judgementSrc.h * 1.01f);
        renderSpriteEX(0, criticalJudgeID, criticalFPoint.x, criticalFPoint.y, touchJudgeScale, true, pointTowards, SDL_FLIP_NONE);
    }

    if(displayFastLate){
        const SDL_FPoint fastLateFPoint = moveXYWithDirAndDistFloat(mainX, mainY, pointTowards, judgementSrc.h * -1.02f);
        //-1 becomes 0 and 1 stays the same
        renderSpriteEX(0, fastLateJudgementsStartID + (judgeSpeedIndex + 1) / 2, fastLateFPoint.x, fastLateFPoint.y, touchJudgeScale, true, pointTowards, SDL_FLIP_NONE);
    }
}

static void renderTapJudgements(int n, int judgeIndex, double judgeDisplayTime){
    assert(n >= 1 && n <= 8);
    const float judgeScale = 0.8f + 0.2f * ((float)SDL_min(judgeDisplayTime, 0.050) / 0.050f);
    const double angleOfJudgement = buttonNumberToDegrees(n);
    const float centerX = getCenterX();
    const float centerY = getCenterY();

    const SDL_FPoint judgementFPoint = moveNoteXY(centerX, centerY, n, travelDistance * guiScale);

    const int maxJudge = 3 + (showCriticalPerfect > 0);
    if(shouldShowRegularJudgement(judgeIndex)){
        renderSpriteEX(0, judgementsStartID + judgeIndex, judgementFPoint.x, judgementFPoint.y, judgeScale, true, angleOfJudgement, SDL_FLIP_NONE);
    }

    int judgeSpeedIndex = judgementInfoForButton[n - 1].judgementDirection;

    if(judgeIndex >= maxJudge){
        judgeSpeedIndex = judgeCritical; //override it
    }
    const bool displayCritical = showCriticalPerfect == 1 && judgeIndex == 4 && (showFastLate != 2 || judgeIndex >= maxJudge);
    const bool displayFastLate = judgeSpeedIndex != 0 && judgeIndex >= 1 && showFastLate;
    SDL_FRect judgementSrc = {0.f, 0.f, 0.f, 0.f};
    if(displayCritical || displayFastLate){
        getAtlasTextureAndSrc(0, judgementsStartID + judgeIndex, NULL, &judgementSrc);
    }

    if(displayCritical){
        const SDL_FPoint criticalFPoint = moveNoteXY(centerX, centerY, n, travelDistance * guiScale + judgementSrc.h * 1.01f);
        renderSpriteEX(0, criticalJudgeID, criticalFPoint.x, criticalFPoint.y, judgeScale, true, angleOfJudgement, SDL_FLIP_NONE);
    }

    if(displayFastLate){
        const SDL_FPoint fastLateFPoint = moveNoteXY(centerX, centerY, n, travelDistance * guiScale - judgementSrc.h * 1.02f);
        judgeSpeedIndex = (judgeSpeedIndex + 1) / 2; //-1 becomes 0 and 1 stays the same
        renderSpriteEX(0, fastLateJudgementsStartID + judgeSpeedIndex, fastLateFPoint.x, fastLateFPoint.y, judgeScale, true, angleOfJudgement, SDL_FLIP_NONE);
    }
}

//try to increase fps
int generalMaxI = 0;
int touchMaxI = 0;

static void renderFanSlide(int button, int minInnerProgress, Uint8 alphaValueForFanSlide, bool isBreak, bool isEach){
    const float centerX = getCenterX();
    const float centerY = getCenterY();
    const int startingArrowRender[10] = {0, 2,
                                         3, 4,
                                         5, 6,
                                         7, 8,
                                         9, 9};
    const SDL_FPoint start = moveNoteXY(centerX, centerY, button, borderRadius * guiScale);
    const double slideDir = buttonNumberToDegrees(button + 4);

    const SDL_FPoint end = moveNoteXY(centerX, centerY, offsetButton(button, 4), borderRadius * guiScale);

    for(int n = startingArrowRender[minInnerProgress]; n < 11; n++){
        const SDL_FPoint progress = interpolateFPoint(start, end, 0.075f + (n / 10.f) * 0.8f);
        const int fanSlideArrowID = isBreak ? breakFanSlideArrowsStartID + n : isEach ? eachFanSlideArrowsStartID + n : normalFanSlideArrowsStartID + n;
        setAlphaOfAtlas(0, fanSlideArrowID, alphaValueForFanSlide);
        renderSpriteEX(0, fanSlideArrowID, progress.x, progress.y, 1.f, true, slideDir, SDL_FLIP_NONE);
        setAlphaOfAtlas(0, fanSlideArrowID, 255);
    }
}

static void renderTapNote(int button, int tapNoteSkin,
                          double noteTime, double moveTime,
                          double starNoteRotationSpeed,
                          bool isBreak, bool isEach, bool isEX){
    float sizeOfTapNote = 1.f;

    double moveTapNoteBy = (borderRadius - travelDistance);
    if(noteTime < moveTime) {
        sizeOfTapNote = noteTime / moveTime;
    } else {
        moveTapNoteBy += (noteTime - moveTime) / (moveTime / travelDistance);
    }

    moveTapNoteBy *= guiScale;

    const SDL_FPoint noteFPoint = moveNoteXY(getCenterX(), getCenterY(), button, moveTapNoteBy);


    double tapNoteDirection = buttonNumberToDegrees(button);
    if(tapNoteSkin != tapNoteNormalSkin){
        if(rotateStarNote){
            tapNoteDirection += noteTime * starNoteRotationSpeed / gameTicksPerSecond;
        }
    }

    int tapNoteID = normalTapNoteID;
    switch(tapNoteSkin){
        case tapNoteNormalSkin:
            if(isBreak){
                tapNoteID = breakTapNoteID;
            } else if(isEach){
                tapNoteID = eachTapNoteID;
            } else {
                tapNoteID = normalTapNoteID;
            }
            break;
        case tapNoteStarSkin:
            if(isBreak){
                tapNoteID = breakSlideNoteID;
            } else if(isEach){
                tapNoteID = eachSlideNoteID;
            } else {
                tapNoteID = normalSlideNoteID;
            }
            break;
        case tapNoteMultiStarSkin:
            if(isBreak){
                tapNoteID = breakMultiSlideNoteID;
            } else if(isEach){
                tapNoteID = eachMultiSlideNoteID;
            } else {
                tapNoteID = normalMultiSlideNoteID;
            }
            break;
        default:
            break;
    }

    renderSpriteEX(0, tapNoteID, noteFPoint.x, noteFPoint.y, sizeOfTapNote, true, tapNoteDirection, SDL_FLIP_NONE);
    if(isEX){
        renderSpriteEX(0, EXNoteBlurID, noteFPoint.x, noteFPoint.y, sizeOfTapNote, true, 0.0, SDL_FLIP_NONE);
    }
}

//nightmare function args
static void renderHoldNote(int button, double moveTime,
                           Sint64 gameTime, Sint64 hitTime,
                           Sint64 holdLength, Sint64 unheldOn,
                           bool isBreak, bool isEach, bool isEX,
                           bool isHeld, bool isMiss){
    const float centerX = getCenterX();
    const float centerY = getCenterY();
    const double directionAsDegrees = buttonNumberToDegrees(button);
    double noteTime = gameTime - hitTime + (moveTime * 2);
    if(noteTime - moveTime * 2 - holdLength >= 0) {
        noteTime = holdLength + moveTime * 2;
    }

    double distance = borderRadius - travelDistance;
    const bool shortTest = holdLength >= (18 * gameTicksPerSecond) / 60 || isMiss;
    const bool isNotHeld = !isHeld &&
                            gameTime >= hitTime + (6 * gameTicksPerSecond) / 60 &&
                            unheldOn <= hitTime + holdLength - (12 * gameTicksPerSecond) / 60 &&
                            (shortTest); //trying some random fixes (todo: should hit at very end when unheld count as held?)

    const int holdNoteSize0ID = (isBreak ? breakHoldNoteSize0ID : isEach ? eachHoldNoteSize0ID : normalHoldNoteSize0ID);
    const int holdNoteSize1ID = (isNotHeld ? unheldHoldNoteSize1ID : isBreak ? breakHoldNoteSize1ID : isEach ? eachHoldNoteSize1ID : normalHoldNoteSize1ID);

    float holdSize0Scale = 1.f;
    SDL_FPoint EXNoteFPoint = {0.f, 0.f};
    if(noteTime < moveTime) {
        holdSize0Scale = noteTime / moveTime;
        if(isEX){
            EXNoteFPoint = moveNoteXY(centerX, centerY, button, distance * guiScale);
        }
        const SDL_FPoint holdSize0FPoint = moveNoteXY(centerX, centerY, button, distance * guiScale);
        renderSpriteEX(0, holdNoteSize0ID, holdSize0FPoint.x, holdSize0FPoint.y, holdSize0Scale, true, directionAsDegrees, SDL_FLIP_NONE);
    } else {
        //3 times moment
        //I legit don't understand anything here but it works

        //first: data
        SDL_Texture* size1Atlas = NULL;
        SDL_FRect src = {0.f, 0.f, 0.f, 0.f};
        getAtlasTextureAndSrc(0, holdNoteSize1ID, &size1Atlas, &src);

        const float texWOfPart2 = src.w;
        const float texHOfPart2 = src.h;

        const float trueMaxDistance = clampDouble(noteTime - moveTime, 0, holdLength) / (moveTime / travelDistance);
        const float maximumDistance = SDL_min(travelDistance, trueMaxDistance);
        const float normalDistance = (noteTime - moveTime) / (moveTime / travelDistance);
        const float subtractDistance = SDL_max(0, normalDistance - SDL_max(travelDistance, trueMaxDistance));

        const float additionalDistance = trueMaxDistance >= travelDistance ? 0 : SDL_max(0, SDL_min(travelDistance, normalDistance) - trueMaxDistance);

        const SDL_FPoint holdStartHitMarkerFPoint = moveNoteXY(centerX, centerY, button, (distance + subtractDistance + additionalDistance) * guiScale);
        const SDL_FPoint holdStartDstFPoint = moveNoteXY(centerX, centerY, button, (distance + subtractDistance + additionalDistance) * guiScale);

        SDL_FRect holdNoteStartDstRect = {holdStartDstFPoint.x, holdStartDstFPoint.y, texWOfPart2, texWOfPart2};
        centerFRect(&holdNoteStartDstRect, false);

        const float heightOfStart = holdNoteStartDstRect.w / 2.f + SDL_min(normalDistance, maximumDistance - subtractDistance) * guiScale;
        SDL_FPoint holdNoteRotationPoint = {holdNoteStartDstRect.w / 2.f, holdNoteStartDstRect.w / 2.f};
        holdNoteStartDstRect.h = heightOfStart;
        //danger
        const SDL_FRect holdNoteStartSrcRect = {src.x, src.y, holdNoteStartDstRect.w, holdNoteStartDstRect.h};

        //third data

        const float texWPart2DivBy2Ceil = SDL_ceilf(texWOfPart2 / 2.f);
        //danger
        const SDL_FRect holdNoteEndSrcRect = {src.x, src.y + (texHOfPart2 - texWPart2DivBy2Ceil), texWOfPart2, texWPart2DivBy2Ceil};

        const SDL_FPoint holdEndDstFPoint = moveNoteXY(centerX, centerY, button, (distance + subtractDistance + additionalDistance) * guiScale + heightOfStart);
        SDL_FRect endHoldNoteLocation = {holdEndDstFPoint.x, holdEndDstFPoint.y, texWOfPart2, texWOfPart2};
        centerFRect(&endHoldNoteLocation, false);
        const double endMarkDist = (distance + subtractDistance + additionalDistance) * guiScale + heightOfStart - texWPart2DivBy2Ceil; //fix ex blur at wrong location
        const SDL_FPoint holdEndHitMarkerFPoint = moveNoteXY(centerX, centerY, button, endMarkDist);

        //second: rendering (for real)
        //render second hit marker first
        renderSpriteEX(0, holdNoteHitMarkerID, holdEndHitMarkerFPoint.x, holdEndHitMarkerFPoint.y, 1.f, true, directionAsDegrees, SDL_FLIP_NONE);
        //render first hit marker second
        //todo: only render if hold is fully extended or is shrinking
        renderSpriteEX(0, holdNoteHitMarkerID, holdStartHitMarkerFPoint.x, holdStartHitMarkerFPoint.y, 1.f, true, directionAsDegrees, SDL_FLIP_NONE);


        //first part of hold note
        //danger
        renderTextureRotated(size1Atlas, &holdNoteStartSrcRect, &holdNoteStartDstRect, directionAsDegrees + 180.0, &holdNoteRotationPoint, SDL_FLIP_NONE);
        //second part of hold note
        endHoldNoteLocation.h /= 2.f;
        //danger
        renderTextureRotated(size1Atlas, &holdNoteEndSrcRect, &endHoldNoteLocation, directionAsDegrees + 180.0, &holdNoteRotationPoint, SDL_FLIP_NONE);

        if(isEX){
            EXNoteFPoint = moveNoteXY(centerX, centerY, button, endMarkDist);
        }
    }

    if(isEX){
        renderSpriteEX(0, EXNoteBlurID, EXNoteFPoint.x, EXNoteFPoint.y, holdSize0Scale, true, 0.0, SDL_FLIP_NONE);
    }
}

static slideStarInfo slideStars[128] = {0};
static int lenOfSlideStars = 0;

static void addStarToRender(SDL_FPoint noteFPoint, float directionOffset, float starScale, int starType, Uint8 alphaValueForStar){
    if(lenOfSlideStars < stackArraySizeInt(slideStars)){
        slideStars[lenOfSlideStars].starFPoint = noteFPoint;
        slideStars[lenOfSlideStars].dir = directionOffset;
        slideStars[lenOfSlideStars].scale = starScale;
        slideStars[lenOfSlideStars].starType = starType;
        slideStars[lenOfSlideStars].alpha = alphaValueForStar;
        lenOfSlideStars++;
    }
}

static void renderSlideNote(int button, int endButton,
                            double noteTime, double moveTime,
                            Sint64 delayLength, Sint64 slideLength,
                            int* indexOfLocationTimes, slideTouchItem* startIndexes, int* indexOfStartIndex,
                            slidePointInfo* arrows, int lenOfArrows,
                            bool isBreak, bool isEach, bool suddenStarAppearance){
    if(noteTime < moveTime){
        return;
    }

    Uint8 alphaValueForStar = 255;
    Uint8 alphaValueForSlide = 255;
    bool shouldRenderStar = false; //true

    double directionOffset = 0.0;
    SDL_FPoint noteFPoint = {0.f, 0.f};
    float starScale = 1.f;

    const float centerX = getCenterX();
    const float centerY = getCenterY();

    if(noteTime < moveTime * 2.0) { //note approaches button
        const float slideArrowVisibility = (noteTime - moveTime) / moveTime;
        alphaValueForSlide *= slideArrowVisibility;
        if(alphaValueForSlide <= 0){
            return;
        }
    } else if(noteTime < moveTime * 2.0 + delayLength) { //star note disappears then reappears and gets even bigger
        if(!suddenStarAppearance){
            starScale = (noteTime - moveTime * 2.0) / delayLength;
            alphaValueForStar *= starScale;
            shouldRenderStar = true;
        }

        noteFPoint = moveNoteXY(centerX, centerY, button, borderRadius * guiScale);
        directionOffset = arrows[0].dir;
    } else {
        const Sint64 currentSlideTime = SDL_min(noteTime - moveTime * 2.0 - delayLength, slideLength);

        //proper progress
        const double normalizedTime = slideLength <= 0 ? 1.0 : (double)currentSlideTime / (double)slideLength;
        while(arrows[*indexOfLocationTimes].arrowTime < normalizedTime){
            (*indexOfLocationTimes) = SDL_min((*indexOfLocationTimes) + 1, lenOfArrows);
        }

        const int locationTimesIndex = (*indexOfLocationTimes);

        const double nextTime = arrows[locationTimesIndex].arrowTime;
        const double previousTime = locationTimesIndex == 0 ? 0.0 : arrows[locationTimesIndex - 1].arrowTime;
        const float realProgress = SDL_min((normalizedTime - previousTime) / (nextTime - previousTime), 1.f);

        const float firstDirectionOffset = arrows[locationTimesIndex].dir;
        const float secondDirectionOffset = arrows[SDL_min(lenOfArrows - 1, locationTimesIndex + 1)].dir;
        directionOffset = interpolateDirections(firstDirectionOffset, secondDirectionOffset, realProgress);

        SDL_FPoint start = {0, 0};
        SDL_FPoint end = {0, 0};

        if(locationTimesIndex == 0){
            start = moveNoteXY(centerX, centerY, button, borderRadius * guiScale);
        } else {
            start.x = screenXOffset + arrows[locationTimesIndex - 1].x * screenSize;
            start.y = screenYOffset + arrows[locationTimesIndex - 1].y * screenSize;
        }

        if(locationTimesIndex == lenOfArrows){
            end = moveNoteXY(centerX, centerY, endButton, borderRadius * guiScale);
        } else {
            end.x = screenXOffset + arrows[locationTimesIndex].x * screenSize;
            end.y = screenYOffset + arrows[locationTimesIndex].y * screenSize;
        }
        noteFPoint = interpolateFPoint(start, end, realProgress);

        shouldRenderStar = true;
    }

    const int slideArrowID = isBreak ? breakSlideArrowID : isEach ? eachSlideArrowID : normalSlideArrowID;

    //render the arrows first
    const int index = (*indexOfStartIndex);
    int endLoop = startIndexes[index].newArrowIndex; //what on earth is this
    if(index > 0 && endLoop <= 0){
        endLoop = startIndexes[index - 1].newArrowIndex;
    }

    setAlphaOfAtlas(0, slideArrowID, alphaValueForSlide);

    for(int n = lenOfArrows - 1; n >= endLoop; n--) { //render in reverse
        const slidePointInfo currentArrow = arrows[n];
        if(currentArrow.isVisible){
            renderSpriteEX(0, slideArrowID,
                           screenXOffset + currentArrow.x * screenSize,
                           screenYOffset + currentArrow.y * screenSize,
                           1.f, true, currentArrow.dir, SDL_FLIP_NONE);
        }
    }
    setAlphaOfAtlas(0, slideArrowID, 255);

    if(shouldRenderStar){
        addStarToRender(noteFPoint,
                        directionOffset,
                        1.f + starScale / 2.f,
                        isBreak ? 2 : isEach ? 1 : 0,
                        alphaValueForStar);
    }
}

static void renderFanSlideNote(int button, int slideIndex,
                               double noteTime, double moveTime,
                               Sint64 delayLength, Sint64 slideLength,
                               double* locationTimes, int* indexOfLocationTimes,
                               int totalFanSlides, int leftProgress, int middleProgress, int rightProgress,
                               bool isBreak, bool isEach, bool suddenStarAppearance){
    if(noteTime < moveTime){
        return;
    }

    const Sint64 currentSlideTime = SDL_min(noteTime - moveTime * 2 - delayLength, slideLength);
    int slidesPassed = 0;
    float progressFloat = 0.f;
    if(slideLength <= 0){
        slidesPassed = currentSlideTime >= 0 ? totalFanSlides - 1 : 0;
        progressFloat = 0.f;
    } else if(locationTimes == NULL){
        if(currentSlideTime >= slideLength){
            progressFloat = 1.f;
        } else {
            progressFloat = ((currentSlideTime * totalFanSlides) % slideLength) / (double)slideLength;
        }
        slidesPassed = clampInt(currentSlideTime * totalFanSlides / slideLength, 0, totalFanSlides - 1);
    } else {
        const double normalizedTime = (double)currentSlideTime / (double)gameTicksPerSecond;
        while((*indexOfLocationTimes) < totalFanSlides && locationTimes[*indexOfLocationTimes] < normalizedTime){
            (*indexOfLocationTimes)++;
        }
        slidesPassed = (*indexOfLocationTimes);
        const double startTime = slidesPassed == 0 ? 0.0 :
                                 locationTimes[slidesPassed - 1];
        const double endTime = locationTimes[slidesPassed];
        progressFloat = (normalizedTime - startTime) / (endTime - startTime);
        progressFloat = SDL_min(progressFloat, 1.f);
    }



    Uint8 alphaValueForFanStars = 255;
    Uint8 alphaValueForFanSlide = 255;
    bool shouldRenderFanStars = false;
    float fanStarScale = 1.f;

    const int realButton = offsetButton(button, (slidesPassed % 2 == 1) * 4);
    const double startDir = buttonNumberToDegrees(realButton);
    SDL_FPoint fanStarFPoints[3] = {0};

    if(noteTime < moveTime * 2) { //note approaches button
        const float slideArrowVisibility = (noteTime - moveTime) / moveTime;
        alphaValueForFanSlide *= slideArrowVisibility;
        if(alphaValueForFanSlide <= 0){
            return;
        }
    } else {
        const float centerX = getCenterX();
        const float centerY = getCenterY();
        const bool isStarGrowing = noteTime < moveTime * 2 + delayLength;
        fanStarFPoints[0] = moveXYWithDirAndDistFloat(centerX, centerY, startDir - 3.f, borderRadius * guiScale);
        fanStarFPoints[1] = moveNoteXY(centerX, centerY, realButton, borderRadius * guiScale);
        fanStarFPoints[2] = moveXYWithDirAndDistFloat(centerX, centerY, startDir + 3.f, borderRadius * guiScale);

        if(isStarGrowing){ //star note disappears then reappears and gets even bigger
            if(!suddenStarAppearance){
                fanStarScale = (noteTime - moveTime * 2.0) / delayLength;
                alphaValueForFanStars *= fanStarScale;
                shouldRenderFanStars = true;
            }
        } else {
            for(int n = 0; n < 3; n++){
                const SDL_FPoint currentSide = fanStarFPoints[n];
                const SDL_FPoint oppositeSide = moveNoteXY(centerX, centerY, offsetButton(realButton, 5 - n), borderRadius * guiScale);

                //divide by 0 moment
                const SDL_FPoint progress = slideLength <= 0 ? oppositeSide : interpolateFPoint(currentSide, oppositeSide, progressFloat);
                fanStarFPoints[n].x = progress.x;
                fanStarFPoints[n].y = progress.y;
            }

            shouldRenderFanStars = true;
        }
    }

    const int minOuterProgress = SDL_min(leftProgress, rightProgress);
    const int minInnerProgress = SDL_min(minOuterProgress, middleProgress);
    const int minN = slideIndex + 1;
    for(int n = totalFanSlides; n >= minN; n--){
        renderFanSlide(offsetButton(button, (n % 2 == 0) * 4),
                       n == minN ? minInnerProgress : 0,
                       alphaValueForFanSlide,
                       isBreak,
                       isEach);
    }

    if(!shouldRenderFanStars){
        return;
    }

    const int renderOrder[3] = {0, 2, 1}; //middle star renders last

    for(unsigned int n = 0; n < stackArraySize(renderOrder); n++){
        addStarToRender(fanStarFPoints[renderOrder[n]],
                        startDir + 180.0 + 22.5 - renderOrder[n] * 22.5,
                        1.f + fanStarScale / 2.f,
                        isBreak ? 2 : isEach ? 1 : 0,
                        alphaValueForFanStars);
    }
}

int renderBeatMap(double generalMoveTime, double touchMoveTime, Sint64 gameTime, float offsetX, float offsetY) {
    //oh god double moment

    lenOfSlideStars = 0;
    if(chartNoteFeatures[featureFireworks]){
        renderFireworks((double)gameTime / gameTicksPerSecond, fireworksStartTime, fireworksTouchLocation);
    }

    const float centerX = getCenterX();
    const float centerY = getCenterY();
    renderTextureSimpler(gameBorder, centerX + offsetX * guiScale, centerY + offsetY * guiScale);

    //render slide in front first, no matter what
    while(generalMaxI < chartLength && gameTime - chartNotes[generalMaxI].noteHitTime + (generalMoveTime * 2.0) >= 0){
        generalMaxI++;
    }

    while(touchMaxI < chartLength && gameTime - chartNotes[touchMaxI].noteHitTime + (touchMoveTime * 2.0) >= 0){
        touchMaxI++;
    }

    int newMinIndex = -1; //more bugs

    for(int i = generalMaxI - 1; i >= minIndexToRender; i--) { //reverse order for a reason
        if(chartNotes[i].hasHit){
            continue;
        }
        newMinIndex = i; //only get min index in this loop

        const double currentNoteTime = (gameTime - chartNotes[i].noteHitTime) + (generalMoveTime * 2.0);

        if(currentNoteTime < generalMoveTime) {
            continue;
        }

        const int currentNoteDirection = chartNotes[i].location;

        switch(chartNotes[i].noteType) {
            case noteTypeTap:
            case noteTypeHold:
            case noteTypeTouch:
            case noteTypeTouchHold:
                break;
            case noteTypeSlide:
                renderSlideNote(currentNoteDirection, chartNotes[i].info.slide.endButton,
                                currentNoteTime, generalMoveTime,
                                chartNotes[i].info.slide.delayLength, chartNotes[i].info.slide.slideLength,
                                &(chartNotes[i].info.slide.indexOfLocationTimes),
                                chartNotes[i].info.slide.startIndexes, &(chartNotes[i].info.slide.indexOfStartIndex),
                                chartNotes[i].info.slide.arrows, chartNotes[i].info.slide.lenOfArrows,
                                chartNotes[i].isBreak, chartNotes[i].isEach, chartNotes[i].info.slide.suddenStarAppearance);
                break;
            case noteTypeFanSlide:
                renderFanSlideNote(currentNoteDirection, chartNotes[i].info.fanSlide.slideIndex,
                                   currentNoteTime, generalMoveTime,
                                   chartNotes[i].info.fanSlide.delayLength, chartNotes[i].info.fanSlide.slideLength,
                                   chartNotes[i].info.fanSlide.locationTimes, &(chartNotes[i].info.fanSlide.indexOfLocationTimes),
                                   chartNotes[i].info.fanSlide.totalFanSlides, chartNotes[i].info.fanSlide.leftProgress,
                                   chartNotes[i].info.fanSlide.middleProgress, chartNotes[i].info.fanSlide.rightProgress,
                                   chartNotes[i].isBreak, chartNotes[i].isEach, chartNotes[i].info.fanSlide.suddenStarAppearance);
                /*if(currentNoteTime < generalMoveTime){
                    break;
                }
                const Sint64 endLen = chartNotes[i].info.fanSlide.slideLength;
                const Sint64 currentSlideTime = SDL_min(currentNoteTime - generalMoveTime * 2 - chartNotes[i].info.fanSlide.delayLength, endLen);
                const int totalFanSlides = chartNotes[i].info.fanSlide.totalFanSlides;
                int slidesPassed = 0;
                float progressFloat = 0.f;
                if(endLen <= 0){
                    slidesPassed = currentSlideTime >= 0 ? totalFanSlides - 1 : 0;
                    progressFloat = 0.f;
                } else if(chartNotes[i].info.fanSlide.locationTimes == NULL){
                    if(currentSlideTime >= endLen){
                        progressFloat = 1.f;
                    } else {
                        progressFloat = ((currentSlideTime * totalFanSlides) % endLen) / (double)endLen;
                    }
                    slidesPassed = clampInt(currentSlideTime * totalFanSlides / endLen, 0, totalFanSlides - 1);
                } else {
                    const double normalizedTime = (double)currentSlideTime / (double)gameTicksPerSecond;
                    while(chartNotes[i].info.fanSlide.indexOfLocationTimes < totalFanSlides &&
                          chartNotes[i].info.fanSlide.locationTimes[chartNotes[i].info.fanSlide.indexOfLocationTimes] < normalizedTime){
                        chartNotes[i].info.fanSlide.indexOfLocationTimes++;
                    }
                    slidesPassed = chartNotes[i].info.fanSlide.indexOfLocationTimes;
                    const double startTime = slidesPassed == 0 ? 0.0 :
                                             chartNotes[i].info.fanSlide.locationTimes[slidesPassed - 1];
                    const double endTime = chartNotes[i].info.fanSlide.locationTimes[slidesPassed];
                    progressFloat = (normalizedTime - startTime) / (endTime - startTime);
                    progressFloat = SDL_min(progressFloat, 1.f);
                }



                Uint8 alphaValueForFanStars = 255;
                Uint8 alphaValueForFanSlide = 0;
                bool shouldRenderFanStars = false;
                float fanStarScale = 1.f;

                const int realButton = offsetButton(currentNoteDirection, (slidesPassed % 2 == 1) * 4);
                const double startDir = buttonNumberToDegrees(realButton);
                SDL_FPoint fanStarFPoints[3] = {0};

                alphaValueForFanSlide = 255;
                if(currentNoteTime < generalMoveTime * 2) { //note approaches button
                    const float slideArrowVisibility = (currentNoteTime - generalMoveTime) / generalMoveTime;
                    alphaValueForFanSlide *= slideArrowVisibility;
                } else {
                    const bool isStarGrowing = currentNoteTime < generalMoveTime * 2 + chartNotes[i].info.fanSlide.delayLength;
                    fanStarFPoints[0] = moveXYWithDirAndDistFloat(centerX, centerY, startDir - 3.f, borderRadius * guiScale);
                    fanStarFPoints[1] = moveNoteXY(centerX, centerY, realButton, borderRadius * guiScale);
                    fanStarFPoints[2] = moveXYWithDirAndDistFloat(centerX, centerY, startDir + 3.f, borderRadius * guiScale);

                    if(isStarGrowing){ //star note disappears then reappears and gets even bigger
                        fanStarScale = (currentNoteTime - generalMoveTime * 2.f) / chartNotes[i].info.fanSlide.delayLength;
                        if(!chartNotes[i].info.fanSlide.suddenStarAppearance){
                            alphaValueForFanStars *= fanStarScale;
                            shouldRenderFanStars = true;
                        }
                    } else {
                        for(int n = 0; n < 3; n++){
                            const SDL_FPoint currentSide = fanStarFPoints[n];
                            const SDL_FPoint oppositeSide = moveNoteXY(centerX, centerY, offsetButton(realButton, 5 - n), borderRadius * guiScale);

                            //divide by 0 moment
                            const SDL_FPoint progress = endLen <= 0 ? oppositeSide : interpolateFPoint(currentSide, oppositeSide, progressFloat);
                            fanStarFPoints[n].x = progress.x;
                            fanStarFPoints[n].y = progress.y;
                        }

                        shouldRenderFanStars = true;
                    }
                }


                if(alphaValueForFanSlide <= 0){
                    break;
                }

                const int minOuterProgress = SDL_min(chartNotes[i].info.fanSlide.leftProgress, chartNotes[i].info.fanSlide.rightProgress);
                const int minInnerProgress = SDL_min(minOuterProgress, chartNotes[i].info.fanSlide.middleProgress);
                const int minN = chartNotes[i].info.fanSlide.slideIndex + 1;
                for(int n = totalFanSlides; n >= minN; n--){
                    renderFanSlide(offsetButton(currentNoteDirection, (n % 2 == 0) * 4),
                                                n == minN ? minInnerProgress : 0,
                                                alphaValueForFanSlide,
                                                chartNotes[i].isBreak,
                                                chartNotes[i].isEach);
                }

                if(!shouldRenderFanStars){
                    break;
                }

                const int renderOrder[3] = {0, 2, 1}; //middle star renders last

                for(unsigned int n = 0; n < stackArraySize(renderOrder); n++){
                    if(lenOfSlideStars < stackArraySizeInt(slideStars)){
                        slideStars[lenOfSlideStars].starFPoint = fanStarFPoints[renderOrder[n]];
                        slideStars[lenOfSlideStars].dir = startDir + 180 + 22.5 - renderOrder[n] * 22.5;
                        slideStars[lenOfSlideStars].scale = 1.f + fanStarScale / 2.f;
                        slideStars[lenOfSlideStars].starType = chartNotes[i].isBreak ? 2 : chartNotes[i].isEach ? 1 : 0;
                        slideStars[lenOfSlideStars].alpha = alphaValueForFanStars;
                        lenOfSlideStars++;
                    }
                }*/
                break;
            default:
                setError("Invalid note type while rendering: %d\n", chartNotes[i].noteType);
        }
    }

    for(int i = 0; i < lenOfSlideStars; i++){
        const int starID = slideStars[i].starType == 2 ? breakSlideNoteID : slideStars[i].starType == 1 ? eachSlideNoteID : normalSlideNoteID;
        setAlphaOfAtlas(0, starID, slideStars[i].alpha);
        renderSpriteEX(0, starID, slideStars[i].starFPoint.x, slideStars[i].starFPoint.y, slideStars[i].scale, true, slideStars[i].dir, SDL_FLIP_NONE);
        setAlphaOfAtlas(0, starID, 255);
    }
    renderSlideJudgements(gameTime);

    //render tap and hold later
    for(int i = generalMaxI - 1; i >= minIndexToRender; i--) { //reverse order for a reason
        if(chartNotes[i].hasHit){
            continue;
        }

        if(newMinIndex == -1){
            newMinIndex = i;
        } else {
            newMinIndex = SDL_min(newMinIndex, i);
        }

        const double currentNoteTime = gameTime - chartNotes[i].noteHitTime + (generalMoveTime * 2.0);


        if(currentNoteTime < 0.0) {
            continue;
        }

        const int currentNoteDirection = chartNotes[i].location;

        switch(chartNotes[i].noteType) {
            case noteTypeTap:
                renderTapNote(currentNoteDirection,  chartNotes[i].info.tap.tapNoteSkin,
                              currentNoteTime,       generalMoveTime, chartNotes[i].info.tap.starNoteRotationSpeed,
                              chartNotes[i].isBreak, chartNotes[i].isEach, chartNotes[i].isEX);
                break;
            case noteTypeHold: //top 5 nightmares
                renderHoldNote(currentNoteDirection, generalMoveTime,
                               gameTime, chartNotes[i].noteHitTime,
                               chartNotes[i].info.hold.holdLength, chartNotes[i].info.hold.unheldOn,
                               chartNotes[i].isBreak,              chartNotes[i].isEach, chartNotes[i].isEX,
                               chartNotes[i].info.hold.isHeld,     chartNotes[i].info.hold.headJudgeType <= judgeMiss);
                break;
            case noteTypeSlide:
            case noteTypeFanSlide:
            case noteTypeTouch:
            case noteTypeTouchHold:
                break;
            default:
                setError("Invalid note type while rendering: %d\n", chartNotes[i].noteType);
        }
    }

    //touch moment
    unsigned int numberOfVisibleTouches[34] = {0};
    bool isFirstTouchBoxEach[34] = {0};
    bool isSecondTouchBoxEach[34] = {0};
    for(int i = minIndexToRender; i < touchMaxI; i++){
        if(chartNotes[i].hasHit || chartNotes[i].noteType != noteTypeTouch){
            continue;
        }

        const int index = chartNotes[i].location;
        numberOfVisibleTouches[index]++;
        if(numberOfVisibleTouches[index] == 2){
            isFirstTouchBoxEach[index] = chartNotes[i].isEach;
        } else if(numberOfVisibleTouches[index] == 3){
            isSecondTouchBoxEach[index] = chartNotes[i].isEach;
        }
    }

    for(int i = 0; i < stackArraySizeInt(numberOfVisibleTouches) ; i++){
        if(numberOfVisibleTouches[i] >= 3){
            renderTouchBox(i, 1.25f, isSecondTouchBoxEach[i]);
            renderTouchBox(i, 1.f, isFirstTouchBoxEach[i]);
        } else if(numberOfVisibleTouches[i] >= 2){
            renderTouchBox(i, 1.f, isFirstTouchBoxEach[i]);
        }
    }

    for(int i = touchMaxI - 1; i >= minIndexToRender; i--) { //reverse order for a reason
        if(chartNotes[i].hasHit){
            continue;
        }
        if(newMinIndex == -1){
            newMinIndex = i;
        } else {
            newMinIndex = SDL_min(newMinIndex, i);
        }

        const Sint64 hitTime = chartNotes[i].noteHitTime;
        const double currentNoteTime = gameTime - hitTime + (touchMoveTime * 2.0);

        if(currentNoteTime < 0.0) {
            continue;
        }

        const int touchLocation = chartNotes[i].location;
        const int touchPointID = chartNotes[i].isEach ? eachTouchPointID : normalTouchPointID;
        const SDL_FPoint touchFPoint = {screenXOffset + touchLocations[touchLocation].x * screenSize,
                                        screenYOffset + touchLocations[touchLocation].y * screenSize};

        switch(chartNotes[i].noteType) {
            case noteTypeTap:
            case noteTypeHold:
            case noteTypeSlide:
            case noteTypeFanSlide:
                break;
            case noteTypeTouch:
                renderTouch(currentNoteTime, touchMoveTime, &touchFPoint, chartNotes[i].isEach);
                renderSprite(0, touchPointID, touchFPoint.x, touchFPoint.y, true);
                break;
            case noteTypeTouchHold:;
                const bool isNotHeld = !(chartNotes[i].info.touchHold.isHeld) &&
                        gameTime >= hitTime + (15 * gameTicksPerSecond) / 60 &&
                        chartNotes[i].info.touchHold.unheldOn <= hitTime + chartNotes[i].info.touchHold.holdLength - (12 * gameTicksPerSecond) / 60;
                renderTouchHold(currentNoteTime, touchMoveTime, &touchFPoint, touchLocation, chartNotes[i].info.touchHold.holdLength, isNotHeld);
                renderSprite(0, touchPointID, touchFPoint.x, touchFPoint.y, true);
                break;

            default:
                setError("Invalid note type while rendering: %d\n", chartNotes[i].noteType);
        }
    }

    minIndexToRender = generalMaxI >= chartLength && touchMaxI >= chartLength && newMinIndex == -1 ? chartLength : SDL_max(minIndexToRender, newMinIndex);

    if(showSensors){ //todo: make it more visible with high background brightness
        int total = 0;
        for(int i = 0; i < 5; i++){
            const float dirChange = i == 2 ? 180.f : 45.f; //C sensor moment
            const int maxN = i == 2 ? 2 : 8;
            for(int n = 0; n < maxN; n++){
                const SDL_FRect newFRect = {screenXOffset + renderingSensorFPoints[total].x, screenYOffset + renderingSensorFPoints[total].y, sensorFRects[i].w, sensorFRects[i].h};
                //what am i doing
                const SDL_FPoint topLeftFPoint = {0.f, 0.f};
                if(heldTouchSensors[total]){
                    if(showSensors != 1){
                        setTextureColor(sensors[i], 64, 255, 64);
                    } else {
                        setTextureColor(sensors[i], 255, 255, 255);
                    }
                    renderTextureRotated(sensors[i], NULL, &newFRect, dirChange * n, &topLeftFPoint, SDL_FLIP_NONE);
                } else if(showSensors != 3){
                    setTextureColor(sensors[i], 255, 255, 255);
                    renderTextureRotated(sensors[i], NULL, &newFRect, dirChange * n, &topLeftFPoint, SDL_FLIP_NONE);
                }
                total++;
            }
        }
    }

    const int maxJudge = 3 + showCriticalPerfect;

    for(int n = 1; n <= 8; n++){
        const double judgeDisplayTime = (gameTime - judgementInfoForButton[n - 1].judgementDisplayTimer);

        if(judgeDisplayTime < 0.0 || judgeDisplayTime >= gameTicksPerSecond / 4.0){
            continue;
        }

        const int judgeIndex = SDL_min(judgeToJudgeIndex(judgementInfoForButton[n - 1].judgementType), maxJudge);

        if(judgeIndex < 0){
            continue;
        }
        renderTapJudgements(n, judgeIndex, judgeDisplayTime / gameTicksPerSecond);
    }

    for(int n = 1; n <= 8; n++){
        const Sint64 headJudgeDisplayTime = (gameTime - judgementInfoForHeadHold[n - 1].judgementDisplayTimer);
        const Sint64 headJudgeEndTime = judgementInfoForHeadHold[n - 1].endTime;

        if(headJudgeDisplayTime < 0 || headJudgeDisplayTime >= headJudgeEndTime){
            continue;
        }

        const int headJudgeIndex = SDL_min(judgeToJudgeIndex(judgementInfoForHeadHold[n - 1].judgementType), maxJudge); //most insane bug ever
        if(headJudgeIndex < 0){
            continue;
        }

        const SDL_FPoint headJudgementFPoint = moveNoteXY(centerX, centerY, n, borderRadius * guiScale);

        //most insane fade in and fade out code ever
        const Sint64 fadeDurationInt = gameTicksPerSecond / 8;
        const Sint64 fadeTimer = (Sint64)(headJudgeDisplayTime) % (fadeDurationInt * 2);

        const Uint8 alphaValue = fadeInFadeOutAlpha(fadeTimer, fadeDurationInt, fadeDurationInt, fadeDurationInt * 2);
        setAlphaOfAtlas(0, alternativeJudgementsStartID + headJudgeIndex, alphaValue);
        renderSprite(0, alternativeJudgementsStartID + headJudgeIndex, headJudgementFPoint.x, headJudgementFPoint.y, true);
        setAlphaOfAtlas(0, alternativeJudgementsStartID + headJudgeIndex, 255);
    }

    #ifdef Circular8_ENABLE_CONTROLLER
    if(isUsingSerialLightController && FCType <= medalTypeNone /* prevent race condition */){
        for(int i = 0; i < 8; i++){ //get new led
            const Sint64 buttonLEDJudgeDisplayTime = (gameTime - judgeButtonLED[i].judgementDisplayTimer);
            SDL_Color newLEDColor = {128, 128, 128, 255};
            if(buttonLEDJudgeDisplayTime >= 0 && buttonLEDJudgeDisplayTime < gameTicksPerSecond / 2){
                const int buttonLEDJudgeIndex = SDL_min(judgeToJudgeIndex(judgeButtonLED[i].judgementType), 3); //led moment
                newLEDColor = buttonLEDColors[buttonLEDJudgeIndex];
            }

            setVirtualLightForOneButton(i + 1, newLEDColor.r, newLEDColor.g, newLEDColor.b);
        }
        tryToMakeVirtualLightsReal();
    }
    #endif // Circular8_ENABLE_CONTROLLER


    if(!chartNoteFeatures[featureTouchNormal] && !chartNoteFeatures[featureTouchEach] && !chartNoteFeatures[featureTouchHold]){ //improves performance slightly on charts without touch notes
        return 0;
    }

    for(int n = 0; n < 8 * 4 + 2; n++){
        const double touchJudgeDisplayTime = (gameTime - judgementInfoForTouch[n].judgementDisplayTimer);
        if(touchJudgeDisplayTime < 0.0 || touchJudgeDisplayTime >= gameTicksPerSecond / 4.0){
            continue;
        }

        const int touchJudgeIndex = SDL_min(judgeToJudgeIndex(judgementInfoForTouch[n].judgementType), maxJudge);
        if(touchJudgeIndex < 0){
            continue;
        }
        renderTouchJudgements(n, touchJudgeIndex, touchJudgeDisplayTime / gameTicksPerSecond);
    }

    for(int n = 0; n < 8 * 4 + 2; n++){
        const Sint64 touchHoldJudgeDisplayTime = (gameTime - judgementInfoForTouchHold[n].judgementDisplayTimer);
        const Sint64 touchHoldJudgeEndTime = judgementInfoForTouchHold[n].endTime;

        if(touchHoldJudgeDisplayTime < 0 || touchHoldJudgeDisplayTime >= touchHoldJudgeEndTime){
            continue;
        }

        const int touchHoldJudgeIndex = SDL_min(judgeToJudgeIndex(judgementInfoForTouchHold[n].judgementType), maxJudge);
        if(touchHoldJudgeIndex < 0){
            continue;
        }

        const double fadeInAndFadeOutDuration = 0.125;
        const double fadeTimer = SDL_fmod((double)touchHoldJudgeDisplayTime / gameTicksPerSecond, fadeInAndFadeOutDuration * 2);
        const Uint8 alphaValue = fadeInFadeOutAlpha(fadeTimer, fadeInAndFadeOutDuration, fadeInAndFadeOutDuration, fadeInAndFadeOutDuration * 2.0);

        const SDL_FPoint location = {touchLocations[n].x, touchLocations[n].y};
        const float mainX = screenXOffset + location.x * screenSize;
        const float mainY = screenYOffset + location.y * screenSize;

        setAlphaOfAtlas(0, alternativeJudgementsStartID + touchHoldJudgeIndex, alphaValue);
        renderSprite(0, alternativeJudgementsStartID + touchHoldJudgeIndex, mainX, mainY, true);
        setAlphaOfAtlas(0, alternativeJudgementsStartID + touchHoldJudgeIndex, 255);
    }
    return 0;
}
