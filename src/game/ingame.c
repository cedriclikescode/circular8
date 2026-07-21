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
#include <assert.h>
#include <limits.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "main.h"
#include "gameinit.h"
#include "render.h"
#include "load.h"
#include "events.h"
#include "miscutils.h"
#include "language.h"
#ifdef Circular8_ENABLE_CONTROLLER
#include "controller/touch.h"
#include "controller/led.h"
#endif // Circular8_ENABLE_CONTROLLER

#ifdef Circular8_ENABLE_FFmpeg
#include "video.h"
#endif // Circular8_ENABLE_FFmpeg

#include "game/ingame.h"
#include "game/chart.h"
#include "game/rendersong.h"
#include "game/scorescreen.h"
#include "game/options.h"
#include "game/profileselect.h"
#include "game/difficultyselect.h"

Sint64 firstOffset = 0;

#define frames(f) ((f) * gameTicksPerSecond / 60)
static const Sint64 EXNoteTimingWindow = frames(9);

static const Sint64 tapNoteTimingWindows[7] = {
    frames(1),          //Critical Perfect
    frames(2),          //Perfect High
    frames(3),          //Perfect low
    frames(4),          //Great High
    frames(5),          //Great Mid
    frames(6),          //Great Low
    EXNoteTimingWindow  //Good (frames(9))
};

//https://www.bilibili.com/opus/970660281077202961

static const Sint64 earlySlideTimingWindows[4] = {
   frames(-17), //Critical Perfect
   frames(-21), //Fast Great Low
   frames(-25), //Fast Great Mid
   frames(-29), //Fast Great High
   //Anything worse is Fast Good
};

static const Sint64 lateSlideTimingWindows[5] = {
    frames(14), //Critical Perfect
    frames(-9), //Late Great High (impossible to obtain)
    frames(19), //Late Great Mid
    frames(23), //Late Great Low
    frames(36)  //Late Good
};

static const Sint64 tooLateSlideTiming = frames(33); //used to expand late good, don't confuse with lateSlideTimingWindows[4]

static const Sint64 touchTimingWindows[7] = {
    frames(9),                     //Critical Perfect
    21 * gameTicksPerSecond / 120, //Perfect High (10.5 frames)
    frames(12),                    //Perfect low
    frames(13),                    //Great High
    frames(14),                    //Great Mid
    frames(15),                    //Great Low
    frames(18)                     //Good
};

#undef frames

MIX_Audio* tapNoteSfx = NULL;
MIX_Audio* holdNoteSfx = NULL;
MIX_Audio* slideStartSfx = NULL;
MIX_Audio* touchNoteSfx = NULL;
MIX_Audio* breakNoteSfx = NULL;
MIX_Audio* EXNoteSfx = NULL;
MIX_Audio* fireworkSfx = NULL;
MIX_Audio* fullComboSfx = NULL;
MIX_Track* ingameTracks[32] = {0};
int indexOfIngameTracks = 0;

Uint64 inGameTime = 0;
Uint64 inGameDelta = 0;
generalJudgementProperties judgementInfoForButton[8] = {0};
#ifdef Circular8_ENABLE_CONTROLLER
//button led for some reason is non standard (#ff0 gives visible green instead of yellow)
generalJudgementProperties judgeButtonLED[8] = {0}; //default color, fixes brief black leds on first song start

const SDL_Color buttonLEDColors[4] = {
    {128, 128, 128, 255}, //Miss is probably not displayed
    {  0, 255,   0, 255},
    {255,   0,   0, 255},
    {255, 128,   0, 255}, //Critical perfect and perfect uses same led color
};
#endif // Circular8_ENABLE_CONTROLLER
holdJudgementProperties judgementInfoForHeadHold[8] = {0};
slideJudgementProperties judgementInfoForSlideEnd[64] = {0};
int lenOfJudgementInfoForSlideEnd = 0;
touchJudgementProperties judgementInfoForTouch[8 * 4 + 2] = {0};
touchHoldJudgementProperties judgementInfoForTouchHold[8 * 4 + 2] = {0}; //new level 15 chart says hi

unsigned int comboCount = 0;

static bool didAnswerSfxPlay[7] = {0};

static void resetAnswerSoundsPlayed(void){
    SDL_memset(didAnswerSfxPlay, 0, sizeof(didAnswerSfxPlay));
}

static void playIngameSound(MIX_Audio* audio){
    MIX_StopTrack(ingameTracks[indexOfIngameTracks], 0);
    MIX_SetTrackAudio(ingameTracks[indexOfIngameTracks], audio);
    MIX_PlayTrack(ingameTracks[indexOfIngameTracks], 0);
    indexOfIngameTracks++;
    if(indexOfIngameTracks >= stackArraySizeInt(ingameTracks)){
        indexOfIngameTracks = 0;
    }
}

static void playAnswerSound(int sfxNumber){
    assertStackArrayBounds(didAnswerSfxPlay, sfxNumber);
    if(!didAnswerSfxPlay[sfxNumber]){
        didAnswerSfxPlay[sfxNumber] = true;
        switch(sfxNumber){
            case tapNoteSfxNum:
                playIngameSound(tapNoteSfx);
                break;
            case holdNoteSfxNum:
                playIngameSound(holdNoteSfx);
                break;
            case slideStartSfxNum:
                if(enableSlideSFX){
                    playIngameSound(slideStartSfx);
                }
                break;
            case breakNoteSfxNum:
                playIngameSound(breakNoteSfx);
                break;
            case touchNoteSfxNum:
                playIngameSound(touchNoteSfx);
                break;
            case EXNoteSfxNum:
                playIngameSound(EXNoteSfx);
                break;
            case fireworkSfxNum:
                if(enableFireworksSFX){
                    playIngameSound(fireworkSfx);
                }
                break;
            default:
                printf("illegal answer sound %d\n", sfxNumber);
                break;
        }
    }
}

answerSfxInfo* answerPlayTimes = NULL;
int lenOfAnswerPlayTimes = 0;
int minIndexToPlaySounds = 0;

static void playAnswerSoundsAsRequired(void) {
    const Sint64 currentChartTime = ((Sint64)inGameTime - (gameTicksPerSecond * 2) - firstOffset + realChartOffset + realGuideOffset);
    static Sint64 oldTime = -99999999999;
    if(oldTime != currentChartTime){
        oldTime = currentChartTime;
        resetAnswerSoundsPlayed();
    }

    while(minIndexToPlaySounds < lenOfAnswerPlayTimes && currentChartTime >= answerPlayTimes[minIndexToPlaySounds].playSoundAt){
        playAnswerSound(answerPlayTimes[minIndexToPlaySounds].soundType);
        minIndexToPlaySounds++;
    }
}

static bool hasInputSinceMiss = true; //for no touch track skip

static void addScore(int noteType, int judgementType, bool isBreak) {
    assertStackArrayBounds(judgementCounts, judgementType);
    assertStackArrayBounds(DXScoreTable, judgementType);

    judgementCounts[judgementType]++;
    currentDXScore += DXScoreTable[judgementType];
    currentMaxDXScore += DXScoreTable[judgePerfectCritical];
    const unsigned int baseScore = pointsTable[isBreak ? 3 : noteType][judgementType];
    const unsigned int bestBaseScore = pointsTable[isBreak ? 3 : noteType][judgePerfectCritical];
    currentBaseScore += baseScore;
    currentMaxBaseScore += bestBaseScore;
    if(isBreak) {
        const unsigned int breakBonus = pointsTable[4][judgementType];
        const unsigned int bestBreakBonus = pointsTable[4][judgePerfectCritical];
        currentBreakBonus += breakBonus;
        currentMaxBreakBonus += bestBreakBonus;
    }

    if(judgementType <= judgeMiss){
        comboCount = 0;
        hasInputSinceMiss = false;
    } else {
        comboCount++;
    }
}

static void setJudgementToTapNote(int note, Sint64 judgementDisplayTimer, int judgementDirection, int judgementType, bool isBreak, bool isHold) {
    assertStackArrayBounds(judgementInfoForButton, note - 1);
    judgementInfoForButton[note - 1].judgementDisplayTimer = judgementDisplayTimer;
    judgementInfoForButton[note - 1].judgementDirection = judgementType < judgePerfectCritical &&
                                                          judgementDirection == judgeCritical ? judgeLate : judgementDirection; //hack for hold notes
    if(showFastLate == 3 && !isBreak){
        judgementInfoForButton[note - 1].judgementDirection = judgeCritical;
    }
    judgementInfoForButton[note - 1].judgementType = judgementType;
    #ifdef Circular8_ENABLE_CONTROLLER
    assertStackArrayBounds(judgeButtonLED, note - 1);
    if(isUsingSerialLightController && judgementType > judgeMiss){
        judgeButtonLED[note - 1].judgementDisplayTimer = judgementDisplayTimer;
        judgeButtonLED[note - 1].judgementDirection = judgementDirection;
        judgeButtonLED[note - 1].judgementType = judgementType;
    }
    #endif // Circular8_ENABLE_CONTROLLER
    addScore(isHold ? noteTypeHold : noteTypeTap, judgementType, isBreak);
}

static void setJudgementToTouchNote(int note, Sint64 judgementDisplayTimer, int judgementType, bool isHold, bool fireworksEnabled){
    assertStackArrayBounds(judgementInfoForTouch, note);
    judgementInfoForTouch[note].judgementDisplayTimer = judgementDisplayTimer;
    judgementInfoForTouch[note].judgementType = judgementType;
    addScore(isHold ? noteTypeHold : noteTypeTap, judgementType, false);
    if(fireworksEnabled && judgementType > judgeMiss){
        playAnswerSound(fireworkSfxNum);
        fireworksTouchLocation = note;
        fireworksStartTime = (double)judgementDisplayTimer / (double)gameTicksPerSecond;
    }
}

static void setHeadTouchHoldJudgement(int touch, Sint64 hitTime, Sint64 endTime, int judgementType) {
    assertStackArrayBounds(judgementInfoForTouchHold, touch);
    judgementInfoForTouchHold[touch].judgementDisplayTimer = hitTime;
    judgementInfoForTouchHold[touch].endTime = endTime;
    judgementInfoForTouchHold[touch].judgementType = judgementType;
}

static void setHeadHoldJudgement(int note, Sint64 hitTime, Sint64 endTime, int judgementDirection, int judgementType) {
    assertStackArrayBounds(judgementInfoForHeadHold, note - 1);
    judgementInfoForHeadHold[note - 1].judgementDisplayTimer = hitTime;
    judgementInfoForHeadHold[note - 1].endTime = endTime;
    judgementInfoForHeadHold[note - 1].judgementDirection = judgementDirection;
    judgementInfoForHeadHold[note - 1].judgementType = judgementType;
}

static void cleanupSlideJudgments(Sint64 currentSongTime){
    //faster way
    int moveBy = 0;
    for(int i = 0; i < lenOfJudgementInfoForSlideEnd; i++){
        if((currentSongTime - judgementInfoForSlideEnd[i].properties.judgementDisplayTimer) > gameTicksPerSecond / 3){
            moveBy++;
        } else if(moveBy > 0){
            judgementInfoForSlideEnd[i - moveBy] = judgementInfoForSlideEnd[i];
        }
    }

    lenOfJudgementInfoForSlideEnd -= moveBy;
}

static void addSlideJudgement(int endLocation, int drawType, double angle, Sint64 judgementDisplayTimer, int judgementDirection, int judgementType, bool isBreak, bool flipX){
    cleanupSlideJudgments(judgementDisplayTimer);
    const int currentLen = lenOfJudgementInfoForSlideEnd;
    const int maxSizeOfSlideJudgements = stackArraySizeInt(judgementInfoForSlideEnd);
    if(currentLen < maxSizeOfSlideJudgements && (showCriticalPerfect != 2 || judgementType < judgePerfectCritical)){
        judgementInfoForSlideEnd[currentLen].drawType = drawType;
        if(drawType == judgeSlideLine){
            judgementInfoForSlideEnd[currentLen].special.angle = makeAngleInRange(angle);
        } else if(drawType == judgeSlideArc){
            judgementInfoForSlideEnd[currentLen].special.arcSlideFlipHorizontally = flipX;
        }

        judgementInfoForSlideEnd[currentLen].location = endLocation;
        judgementInfoForSlideEnd[currentLen].properties.judgementDisplayTimer = judgementDisplayTimer;
        judgementInfoForSlideEnd[currentLen].properties.judgementDirection = judgementDirection;
        judgementInfoForSlideEnd[currentLen].properties.judgementType = judgementType;
        lenOfJudgementInfoForSlideEnd++;
    }
    addScore(noteTypeSlide, judgementType, isBreak);
}

static int getJudgeFromTapNote(Sint64 hitTime) {
    const Sint64 noteHitOffset = absMacro(hitTime);
    if(noteHitOffset <= tapNoteTimingWindows[0]) {
        return judgePerfectCritical;
    } else if(noteHitOffset <= tapNoteTimingWindows[1]) {
        return judgePerfectHigh;
    } else if(noteHitOffset <= tapNoteTimingWindows[2]) {
        return judgePerfectLow;
    } else if(noteHitOffset <= tapNoteTimingWindows[3]) {
        return judgeGreatHigh;
    } else if(noteHitOffset <= tapNoteTimingWindows[4]) {
        return judgeGreatMid;
    } else if(noteHitOffset <= tapNoteTimingWindows[5]) {
        return judgeGreatLow;
    } else if(noteHitOffset <= tapNoteTimingWindows[6]) {
        return judgeGood;
    }
    return judgeMiss;
}

static int getJudgeFromTouchNote(Sint64 hitTime) {
    if(hitTime < 0){ //no early hits
        if(hitTime >= -touchTimingWindows[0]){
            return judgePerfectCritical;
        }
        return judgeMiss;
    }

    if(hitTime <= touchTimingWindows[0]) {
        return judgePerfectCritical;
    } else if(hitTime <= touchTimingWindows[1]) {
        return judgePerfectHigh;
    } else if(hitTime <= touchTimingWindows[2]) {
        return judgePerfectLow;
    } else if(hitTime <= touchTimingWindows[3]) {
        return judgeGreatHigh;
    } else if(hitTime <= touchTimingWindows[4]) {
        return judgeGreatMid;
    } else if(hitTime <= touchTimingWindows[5]) {
        return judgeGreatLow;
    } else if(hitTime <= touchTimingWindows[6]) {
        return judgeGood;
    }
    return judgeMiss;
}

static int getJudgeFromSlide(Sint64 hitTime, double variableT) {
    const Sint64 maxWindow = gameTicksPerSecond * 3 / 5; //simplified factor of 36 / 60
    const Sint64 critWindow = SDL_min(maxWindow, lateSlideTimingWindows[0] + (variableT / 4.0) * gameTicksPerSecond);
    hitTime += variableT * gameTicksPerSecond;
    if(hitTime < 0) {
        if(-hitTime <= critWindow || hitTime >= earlySlideTimingWindows[0]) {
            return judgePerfectCritical;
        } else if(hitTime >= earlySlideTimingWindows[1]) {
            return judgeGreatHigh;
        } else if(hitTime >= earlySlideTimingWindows[2]) {
            return judgeGreatMid;
        } else if(hitTime >= earlySlideTimingWindows[3]) {
            return judgeGreatLow;
        } else {
            return judgeGood;
        }
    } else {
        const Sint64 maxLateGood = tooLateSlideTiming + variableT * gameTicksPerSecond;
        if(hitTime <= critWindow) {
            return judgePerfectCritical;
        } else if(hitTime <= lateSlideTimingWindows[1]) {
            return judgeGreatHigh;
        } else if(hitTime <= lateSlideTimingWindows[2]) {
            return judgeGreatMid;
        } else if(hitTime <= lateSlideTimingWindows[3]) {
            return judgeGreatLow;
        } else if(hitTime <= lateSlideTimingWindows[4] || hitTime <= maxLateGood) {
            return judgeGood;
        }
    }

    return judgeMiss;
}

static void handleTapAndHold(int button, int heldBy, Sint64 currentChartTime) {
    for(int noteI = minIndexToRender; noteI < chartLength; noteI++) {
        if(chartNotes[noteI].hasHit){
            continue;
        }

        Sint64 noteHitOffset = currentChartTime - chartNotes[noteI].noteHitTime;

        if(noteHitOffset < -EXNoteTimingWindow){ //faster
            break;
        }

        if(chartNotes[noteI].location == button &&
                (chartNotes[noteI].noteType == noteTypeTap || chartNotes[noteI].noteType == noteTypeHold)) {
            //jackpot
            //note: touch and slide note are handled differently
             if(noteHitOffset < 0){
                noteHitOffset *= -1; //flip sign to emulate absolute
            }
            int noteJudgeDirection = (currentChartTime - chartNotes[noteI].noteHitTime) < 0 ? judgeEarly : judgeLate;
            int noteJudgeType = judgeMiss;

            if(chartNotes[noteI].noteType == noteTypeHold) { //weird special case
                const bool continueJudging = chartNotes[noteI].info.hold.isHeld; //for utage
                chartNotes[noteI].info.hold.isHeld = true;
                switch(chartNotes[noteI].info.hold.inputType){
                    case heldByNeither:
                        chartNotes[noteI].info.hold.inputType = heldBy; //worlds weirdest bug
                        break;
                    case heldByButton:
                        if(heldBy == heldByTouchscreen){
                            chartNotes[noteI].info.hold.inputType = heldByBoth;
                        } else {
                            chartNotes[noteI].info.hold.inputType = heldBy;
                        }
                        break;
                    case heldByTouchscreen:
                        if(heldBy == heldByButton){
                            chartNotes[noteI].info.hold.inputType = heldByBoth;
                        } else {
                            chartNotes[noteI].info.hold.inputType = heldBy;
                        }
                        break;
                    default:
                        break;
                }

                if(!chartNotes[noteI].info.hold.hasAcknowledgedHeld && chartNotes[noteI].info.hold.headJudgeType <= judgeMiss) {
                    int headJudgeType = getJudgeFromTapNote(noteHitOffset);
                    if(chartNotes[noteI].isEX && headJudgeType != judgeMiss){
                        headJudgeType = judgePerfectCritical;
                    }
                    const int headJudgeDirection = headJudgeType == judgePerfectCritical ? judgeCritical : noteJudgeDirection;
                    chartNotes[noteI].info.hold.headJudgeType = headJudgeType;
                    chartNotes[noteI].info.hold.headJudgeDirection = headJudgeDirection;
                    setHeadHoldJudgement(button, chartNotes[noteI].noteHitTime, chartNotes[noteI].info.hold.holdLength, headJudgeDirection, headJudgeType);
                }

                if(continueJudging){ //utage moment
                    continue;
                } else {
                    break; //break is here to stop
                }
            } else if(chartNotes[noteI].isEX && noteHitOffset <= EXNoteTimingWindow) {
                noteJudgeDirection = judgeCritical;
                noteJudgeType = judgePerfectCritical;
            } else if(chartNotes[noteI].noteType == noteTypeTap) {
                noteJudgeType = getJudgeFromTapNote(noteHitOffset);
                if(noteJudgeType == judgeMiss){
                    //edge cases with note being late good one frame before
                    continue;
                }

                if(noteJudgeType == judgePerfectCritical) {
                    noteJudgeDirection = judgeCritical;
                }
            }

            setJudgementToTapNote(chartNotes[noteI].location, currentChartTime + realInputOffset, noteJudgeDirection, noteJudgeType,
                                  chartNotes[noteI].isBreak, false); //dumb bugs with impossible fixes if you don't know what to look for
            chartNotes[noteI].hasHit = true;
            break;
        }
    }
}

static void apply51PercentRule(int noteI, int noteJudgeType, Sint64 currentChartTime){
    const bool isTouch = chartNotes[noteI].noteType == noteTypeTouch;
    const int len = isTouch ?
                    chartNotes[noteI].info.touch.lenOfConnectedNotes :
                    chartNotes[noteI].info.touchHold.lenOfConnectedNotes;
    const int* connectedNotes = isTouch ?
                                chartNotes[noteI].info.touch.connectedNotes :
                                chartNotes[noteI].info.touchHold.connectedNotes;
    int notesHit = 1;

    for(int i = 0; i < len; i++){
        if(!chartNotes[connectedNotes[i]].hasHit ||
           (chartNotes[connectedNotes[i]].noteType == noteTypeTouchHold &&
            chartNotes[connectedNotes[i]].info.touchHold.headJudgeType == -1)){
            continue;
        }

        notesHit++;

        if(notesHit <= (len + 1) / 2){ //51% logic, + 1 to include self
            continue;
        }

        for(int n = 0; n < len; n++){
            const int note = connectedNotes[n];
            if(chartNotes[note].hasHit ||
              (chartNotes[note].noteType == noteTypeTouchHold &&
               chartNotes[connectedNotes[i]].info.touchHold.headJudgeType != -1)){
                continue;
            }

            if(chartNotes[note].noteType == noteTypeTouch){
                setJudgementToTouchNote(chartNotes[note].location,
                                        currentChartTime,
                                        noteJudgeType, false, chartNotes[note].info.touch.fireworksEnabled);
                chartNotes[note].hasHit = true;
                continue;
            }

            chartNotes[note].info.touchHold.headJudgeType = noteJudgeType;
            setHeadTouchHoldJudgement(chartNotes[note].location, chartNotes[noteI].noteHitTime, chartNotes[noteI].info.touchHold.holdLength, noteJudgeType);
        }
        break;
    }
}

static void handleTouchAndTouchHold(int touch, Sint64 currentChartTime){
    for(int noteI = minIndexToRender; noteI < chartLength; noteI++) {
        if(chartNotes[noteI].hasHit){
            continue;
        }

        const Sint64 noteHitOffset = currentChartTime - chartNotes[noteI].noteHitTime;

        if(noteHitOffset < -touchTimingWindows[0]){ //no early hits for touch
            break;
        }

        const bool isTouch = chartNotes[noteI].noteType == noteTypeTouch;
        const bool isTouchHold = chartNotes[noteI].noteType == noteTypeTouchHold;
        const int location = chartNotes[noteI].location;
        const bool isCSensor = (isTouch || isTouchHold) && (location == 8 * 2 || location == 8 * 2 + 1);

        if(!(isTouch || isTouchHold) || (isCSensor ? (touch != 8 * 2 && touch != 8 * 2 + 1) : location != touch)){
            continue;
        }


        if(isTouch){
            const int noteJudgeType = getJudgeFromTouchNote(noteHitOffset);
            if(noteJudgeType > judgeMiss){ //fast miss is impossible
                setJudgementToTouchNote(isCSensor ? 8 * 2 : touch,
                                       currentChartTime,
                                       noteJudgeType, false, chartNotes[noteI].info.touch.fireworksEnabled);
                chartNotes[noteI].hasHit = true;
                apply51PercentRule(noteI, noteJudgeType, currentChartTime);
            }
        } else if(isTouchHold){
            chartNotes[noteI].info.touchHold.isHeld = true;
            chartNotes[noteI].info.touchHold.isC1Held = isCSensor && touch % 8 + 1 == 1;
            chartNotes[noteI].info.touchHold.isC2Held = isCSensor && touch % 8 + 1 == 2;

            if(!chartNotes[noteI].info.touchHold.hasAcknowledgedHeld) {
                //display judge if fail
                if(chartNotes[noteI].info.touchHold.headJudgeType == -1){
                    const int headJudgeType = getJudgeFromTouchNote(noteHitOffset);
                    chartNotes[noteI].info.touchHold.headJudgeType = headJudgeType;
                    setHeadTouchHoldJudgement(isCSensor ? 2 * 8 : touch, chartNotes[noteI].noteHitTime, chartNotes[noteI].info.touchHold.holdLength, headJudgeType);
                    apply51PercentRule(noteI, headJudgeType, currentChartTime);
                } else if(chartNotes[noteI].info.touchHold.headJudgeType == judgeMiss){
                    setHeadTouchHoldJudgement(isCSensor ? 2 * 8 : touch, chartNotes[noteI].noteHitTime, chartNotes[noteI].info.touchHold.holdLength, judgeMiss);
                    apply51PercentRule(noteI, judgeMiss, currentChartTime);
                }
            }


        }
        break;
    }
}

static void handleButtonRelease(int button, int heldBy, Sint64 gameTime) {
    for(int noteI = minIndexToRender; noteI < chartLength; noteI++) {
        const Sint64 timeBeforeHit = chartNotes[noteI].noteHitTime - gameTime;
        if(timeBeforeHit > EXNoteTimingWindow){
            break; //easy performance optimization
        }
        const bool isHeldByBoth = chartNotes[noteI].info.hold.inputType == heldByBoth;

        if(chartNotes[noteI].hasHit ||
           chartNotes[noteI].noteType != noteTypeHold ||
           chartNotes[noteI].location != button){
            continue;
        }

        if(isHeldByBoth){
            if(heldBy == heldByButton){
                chartNotes[noteI].info.hold.inputType = heldByTouchscreen;
            } else {
                chartNotes[noteI].info.hold.inputType = heldByButton;
            }
            break;
        } else if(chartNotes[noteI].info.hold.inputType == heldBy){
            chartNotes[noteI].info.hold.isHeld = false;
            chartNotes[noteI].info.hold.inputType = heldByNeither;
            chartNotes[noteI].info.hold.unheldOn = gameTime;
            break;
        }
    }
}

SDL_Texture* centerDisplayTopInfo = NULL;
SDL_Texture* centerDisplayTopDXStarsInfo[6] = {0};
SDL_Texture* gameBackground = NULL;

SDL_Texture* trackSkippingText = NULL;

static void renderBackground(Uint8 brightness){
    float bgTexW, bgTexH;
    if(gameBackground != NULL && SDL_GetTextureSize(gameBackground, &bgTexW, &bgTexH)){
        setTextureColor(gameBackground, brightness, brightness, brightness);
        const SDL_FRect bgFRect = {screenXOffset, screenYOffset, screenSize, screenSize};
        renderTexture(gameBackground, NULL, &bgFRect);
    }
}

int FCType = medalTypeNone;
static Uint64 timeSinceFCShown = 0;
static Uint64 timeSinceLastOffsetCall = 0;
static Uint64 timeBeforeTrackSkip = 4 * gameTicksPerSecond;

static void renderTopInfo(SDL_Texture* topInfo){
    float texW = 0;
    float texH = 0;

    SDL_GetTextureSize(topInfo, &texW, &texH);
    const SDL_FRect topInfoFRect = xywh(getCenterX(), getCenterY() - texH * 1.15f, texW, texH);

    setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE / 2.f);

    const SDL_FRect filterFRect = FRectSimple(&topInfoFRect, 1.05f, true);
    renderFillRectF(&filterFRect);

    renderTextureSimpler(topInfo, topInfoFRect.x, topInfoFRect.y);
}

SDL_Texture* userScreenScore = NULL;
SDL_Texture* songInfo = NULL;
SDL_Texture* subScreenDifficultyName = NULL;
SDL_Texture* subScreenDifficultyLevel = NULL;
SDL_Texture* subScreenSongTitle = NULL;

void renderDifficultyInfo(float offsetX, float offsetY){
    //render difficulty thing
    const SDL_FRect songInfoFRect = FRectWithTexSimple(songInfo, getCenterX() + offsetX * guiScale, offsetY * guiScale, 1.f, false);
    const SDL_Color diffColor = songs[currentSongIndex].difficultyColors[selectedDifficulty];
    setTextureColor(songInfo, diffColor.r, diffColor.g, diffColor.b);
    renderTextureSimpleManual(songInfo, &songInfoFRect, true);

    //render difficulty name and cap width
    SDL_FRect difficultyFRect = FRectWithTexSimple(subScreenDifficultyName, songInfoFRect.x, songInfoFRect.y - 35.f * guiScale, 1.f, false);
    difficultyFRect.w = SDL_min(difficultyFRect.w, 110.f * guiScale);

    renderTextureSimpleManual(subScreenDifficultyName, &difficultyFRect, true);

    //render song title and cap width
    SDL_FRect infoFRect = FRectWithTexSimple(subScreenSongTitle, songInfoFRect.x - 62.f * guiScale, songInfoFRect.y + 15.f * guiScale, 1.f, false);
    infoFRect.w = SDL_min(infoFRect.w, 270.f * guiScale);

    renderTextureSimpleManual(subScreenSongTitle, &infoFRect, true);

    //render level and cap width
    if(subScreenDifficultyLevel != NULL){
        infoFRect = FRectWithTexSimple(subScreenDifficultyLevel, songInfoFRect.x + 143.f * guiScale, songInfoFRect.y + 15.f * guiScale, 1.f, false);
        infoFRect.w = SDL_min(infoFRect.w, 115.f * guiScale);

        renderTextureSimpleManual(subScreenDifficultyLevel, &infoFRect, true);
    }
}

int renderInGameInfoOnSubscreen(void){
    if(numberOfProfiles > 0){
        renderUserInfoOnSubscreen();
    }

    const SDL_FRect topInfoFRect = FRectWithTexSimple(userScreenScore, getCenterX() + 105.f * guiScale, 70.f * guiScale, 1.f, false); //xywh(getCenterX() + 105.f * guiScale, 70.f * guiScale, texW, texH);

    setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE);

    const SDL_FRect filterFRect = FRectSimple(&topInfoFRect, 1.05f, true);
    renderFillRectF(&filterFRect);

    renderTextureSimpler(userScreenScore, topInfoFRect.x, topInfoFRect.y);

    char infoChars[16];


    separatePercentageWithCorrectDecimal(infoChars, 16, calculateScore(currentBaseScore, absoluteMaxBaseScore, currentBreakBonus, absoluteMaxBreakBonus), 4);
    renderNumbersEx(infoChars, topInfoFRect.x, topInfoFRect.y + 75.f * guiScale, middleAlign, true, ((SDL_Color){0, 0, 0, SDL_ALPHA_OPAQUE}), 3.f * guiScale);

    renderDifficultyInfo(-140.f, 225.f);
    return 0;
}

static int renderInGame(void) {
    const Sint64 currentChartTime = (Sint64)inGameTime - (gameTicksPerSecond * 2) - firstOffset - realChartOffset;

    const Uint8 realBackgroundBrightness = SDL_round(backgroundBrightness * 255.0);
    if(realBackgroundBrightness > 0){
        #ifdef Circular8_ENABLE_FFmpeg
        if(isShowingVideo){
            renderVideoFit(SDL_max(currentChartTime + firstOffset, 0) / (double)gameTicksPerSecond, realBackgroundBrightness);
        } else {
            renderBackground(realBackgroundBrightness);
        }
        #else
        renderBackground(realBackgroundBrightness);
        #endif // Circular8_ENABLE_FFmpeg
    }

    SDL_Point DXStarsAndHP = {0, 0};
    if(timeBeforeTrackSkip <= 3 * gameTicksPerSecond){
        renderTopInfo(trackSkippingText);
    } else if(centerDisplayTopInfo != NULL){
        renderTopInfo(centerDisplayTopInfo);
    } else if(centerDisplayTopDXStarsInfo[0] != NULL){
        DXStarsAndHP = getMaxDXStarsAndHP(absoluteMaxDXScore - (currentMaxDXScore - currentDXScore), absoluteMaxDXScore, 5);
        if(DXStarsAndHP.x >= 0){
            renderTopInfo(centerDisplayTopDXStarsInfo[DXStarsAndHP.x]);
        }
    }

    char infoChars[16] = {0};
    switch(displayType){
        case achievementCombo:
            simple_snprintf(infoChars, "%u", comboCount);
            break;
        case achievementPlus:
            separatePercentageWithCorrectDecimal(infoChars, 16, calculateScore(currentBaseScore,
                                                                absoluteMaxBaseScore,
                                                                currentBreakBonus,
                                                                absoluteMaxBreakBonus), 4);
            break;
        case achievementAverage:;
            const bool isThereBreakAndCurrentMaxIsZero = absoluteMaxBreakBonus > 0 && currentMaxBreakBonus == 0; //fixes issue

            separatePercentageWithCorrectDecimal(infoChars, 16, calculateScore(currentBaseScore,
                                                                currentMaxBaseScore,
                                                                currentBreakBonus + isThereBreakAndCurrentMaxIsZero,
                                                                currentMaxBreakBonus + isThereBreakAndCurrentMaxIsZero), 4);
            break;
        case achievementMinus1:
            separatePercentageWithCorrectDecimal(infoChars, 16, calculateScore(absoluteMaxBaseScore - (currentMaxBaseScore - currentBaseScore),
                                                                absoluteMaxBaseScore,
                                                                currentBreakBonus,
                                                                absoluteMaxBreakBonus), 4);
            break;
        case achievementMinus2:
        case achievementBorderS:
        case achievementBorderSS:
        case achievementBorderSSS:
        case achievementMyBest:;
            double maxScorePossible = calculateScore(absoluteMaxBaseScore - (currentMaxBaseScore - currentBaseScore),
                                                     absoluteMaxBaseScore,
                                                     absoluteMaxBreakBonus - (currentMaxBreakBonus - currentBreakBonus),
                                                     absoluteMaxBreakBonus);
            switch(displayType){
                case achievementBorderS:
                    maxScorePossible -= 97.0;
                    break;
                case achievementBorderSS:
                    maxScorePossible -= 99.0;
                    break;
                case achievementBorderSSS:
                    maxScorePossible -= 100.0;
                    break;
                case achievementMyBest:
                    maxScorePossible = SDL_trunc((maxScorePossible - highScore) * 10000.0) / 10000.0; //prevent rounding errors
                    break;
                default:
                    break;
            }

            if(maxScorePossible >= 0){ //TODO: should i remove it for borders?
                //simple_snprintf(infoChars, "%.4f%%", maxScorePossible);
                separatePercentageWithCorrectDecimal(infoChars, 16, maxScorePossible, 4);
            }
            break;
        case achievementDXScorePlus:
            simple_snprintf(infoChars, "+%u", currentDXScore);
            break;
        case achievementDXScoreMinus:
            simple_snprintf(infoChars, "-%u", currentMaxDXScore - currentDXScore);
            break;
        case achievementDXScoreStars:
            if(DXStarsAndHP.x >= 0){
                simple_snprintf(infoChars, "+%d", DXStarsAndHP.y); //todo: what should happen if 0 stars
            }
            break;
        default:
            break;
    }

    //track skip moment
    if(timeBeforeTrackSkip <= 3 * gameTicksPerSecond){
        simple_snprintf(infoChars, "%d", (int)(SDL_min(timeBeforeTrackSkip / gameTicksPerSecond, 2)) + 1);
    }

    renderNumbersEx(infoChars, getCenterX(), getCenterY(), middleAlign, true, ((SDL_Color){0, 0, 0, SDL_ALPHA_OPAQUE / 2.f}), 3.f * guiScale);
    const Uint64 FCMedalMovementTime = gameTicksPerSecond / 8;
    static float offsetX = 0.f;
    static float offsetY = 0.f;
    if(timeSinceLastOffsetCall >= gameTicksPerSecond / 33){ //33 chosen to try to prevent uneven shaking on 30 and 60 fps
        timeSinceLastOffsetCall = 0;
        if(timeSinceFCShown >= FCMedalMovementTime && timeSinceFCShown < FCMedalMovementTime + gameTicksPerSecond){
            offsetX = SDL_randf() * 24.f - 12.f;
            offsetY = SDL_randf() * 24.f - 12.f;
        } else {
            offsetX = 0.f;
            offsetY = 0.f;
        }
    }

    //2.0 / x is the same as:
    //60.0 / x / 30.0
    //but more accurate and more immune to overflow

    renderBeatMap((2.0 / tapSpeed) * gameTicksPerSecond, (2.0 / touchSpeed) * gameTicksPerSecond, currentChartTime, offsetX, offsetY);


    if(FCType >= medalTypeFC){
        #ifdef Circular8_ENABLE_CONTROLLER
        if(isUsingSerialLightController){
            const SDL_Color FCColor = applyFCMedalLightingEffect(FCType, timeSinceFCShown);
            setVirtualLightForMultipleButtons(1, 8, FCColor.r, FCColor.g, FCColor.b);
            tryToMakeVirtualLightsReal();
        }
        #endif // Circular8_ENABLE_CONTROLLER
        const Uint8 brightness = 128 + fadeInFadeOutAlpha(timeSinceFCShown % gameTicksPerSecond, gameTicksPerSecond / 2.0, gameTicksPerSecond / 2.0, gameTicksPerSecond) / 2;
        setTextureColor(FCMedals[FCType], brightness, brightness, brightness);
        if(timeSinceFCShown < FCMedalMovementTime){
            float texH = 0;
            SDL_GetTextureSize(FCMedals[FCType], NULL, &texH);
            renderTextureSimpler(FCMedals[FCType], getCenterX(), interpolateFloat(-texH, getCenterY(), (float)timeSinceFCShown / (float)FCMedalMovementTime));
        } else {
            renderTextureSimpler(FCMedals[FCType], getCenterX() + offsetX, getCenterY() + offsetY);
        }

    }
    return 0;
}

static void handleHoldNote(int i, Sint64 headSkipDuration, Sint64 currentChartTime){
    const Sint64 chartTimeOffset = currentChartTime - chartNotes[i].noteHitTime;
    const bool isTouchHold = chartNotes[i].noteType == noteTypeTouchHold;
    const Sint64 holdLength = isTouchHold ? chartNotes[i].info.touchHold.holdLength : chartNotes[i].info.hold.holdLength;
    const Sint64 tailSkipDuration = (12 * gameTicksPerSecond) / 60;
    const Sint64 headAndTailSkipDuration = headSkipDuration + tailSkipDuration;
    const Sint64 maxDuration = holdLength - headAndTailSkipDuration;
    const bool shouldStartCheck = chartTimeOffset > headSkipDuration;
    //macro to solve our nightmares (doesn't work with judgement direction)
    #define currentHoldInfo(field) (isTouchHold ? (chartNotes[i].info.touchHold.field) : (chartNotes[i].info.hold.field))
    if(isTouchHold){
        chartNotes[i].info.touchHold.hasStartedToCheck = shouldStartCheck;
    } else {
        chartNotes[i].info.hold.hasStartedToCheck = shouldStartCheck;
    }

    const bool hasAcknowledgedHeld = currentHoldInfo(hasAcknowledgedHeld);//isTouchHold ? chartNotes[i].info.touchHold.hasAcknowledgedHeld : chartNotes[i].info.hold.hasAcknowledgedHeld;

    //code from hell
    const bool isTooLateMissIfNotHit = isTouchHold ? chartTimeOffset > touchTimingWindows[6] :
                                                           chartTimeOffset > EXNoteTimingWindow;
    if(holdLength <= headAndTailSkipDuration){
        //hack
        if(currentHoldInfo(headJudgeType) > -1){
            if(isTouchHold){
                chartNotes[i].info.touchHold.stopChecking = true;
            } else {
                chartNotes[i].info.hold.stopChecking = true;
            }
        }

        if(chartTimeOffset >= holdLength && (currentHoldInfo(headJudgeType) > -1 || isTooLateMissIfNotHit)){
            chartNotes[i].hasHit = true;
            if(isTooLateMissIfNotHit && currentHoldInfo(headJudgeType) <= judgeMiss){
                if(isTouchHold){
                    chartNotes[i].info.touchHold.headJudgeType = judgeMiss;
                } else {
                    chartNotes[i].info.hold.headJudgeType = judgeMiss;
                    chartNotes[i].info.hold.headJudgeDirection = judgeLate;
                }
            }

            if(isTouchHold){
                setJudgementToTouchNote(chartNotes[i].location, currentChartTime, currentHoldInfo(headJudgeType), true, chartNotes[i].info.touchHold.fireworksEnabled);
            } else {
                setJudgementToTapNote(chartNotes[i].location, currentChartTime + realInputOffset, chartNotes[i].info.hold.headJudgeDirection, currentHoldInfo(headJudgeType),
                          chartNotes[i].isBreak, true);
            }
        }
    } else if(((isTouchHold || chartNotes[i].info.hold.inputType != heldByNeither) && currentHoldInfo(isHeld)) || hasAcknowledgedHeld) {
        const bool isAtMostMaxDuration = (chartTimeOffset - headSkipDuration) < maxDuration; //< or <= ???

        if(shouldStartCheck && (isAtMostMaxDuration || !currentHoldInfo(stopChecking))) {
            Sint64 maxDelta = LLONG_MAX;
            if(!isAtMostMaxDuration) {
                if(isTouchHold){
                    chartNotes[i].info.touchHold.stopChecking = true;
                } else {
                    chartNotes[i].info.hold.stopChecking = true;
                }
                maxDelta = maxDuration - (currentHoldInfo(oldChartOffset) - headSkipDuration);
            }
            if(!hasAcknowledgedHeld && !currentHoldInfo(hasStartedToCheck)) {
                if(isTouchHold){
                    chartNotes[i].info.touchHold.heldDuration = SDL_min(chartTimeOffset - headSkipDuration, maxDuration);
                } else {
                    chartNotes[i].info.hold.heldDuration = SDL_min(chartTimeOffset - headSkipDuration, maxDuration);
                }
            } else if(currentHoldInfo(isHeld)){
                const Sint64 newHeldDuration = currentHoldInfo(heldDuration) + SDL_min((Sint64)inGameDelta, maxDelta);
                if(isTouchHold){
                    chartNotes[i].info.touchHold.heldDuration = SDL_min(newHeldDuration, maxDuration);
                } else {
                    chartNotes[i].info.hold.heldDuration = SDL_min(newHeldDuration, maxDuration);
                }
            }

            if(isTouchHold){
                chartNotes[i].info.touchHold.hasAcknowledgedHeld = true;
                chartNotes[i].info.touchHold.heldDuration = SDL_max(0, currentHoldInfo(heldDuration));
            } else {
                chartNotes[i].info.hold.hasAcknowledgedHeld = true;
                chartNotes[i].info.hold.heldDuration = SDL_max(0, currentHoldInfo(heldDuration));
            }

            //setJudgementToTapNote(chartNotes[i].location, currentChartTime, judgeLate, judgeMiss);
            //chartNotes[i].hasHit = true;
        }

        if(chartTimeOffset >= holdLength) {
            chartNotes[i].hasHit = true;
            if(holdLength <= headAndTailSkipDuration) {
                if(isTouchHold){
                    setJudgementToTouchNote(chartNotes[i].location, currentChartTime, currentHoldInfo(headJudgeType), true, chartNotes[i].info.touchHold.fireworksEnabled);
                } else {
                    setJudgementToTapNote(chartNotes[i].location, currentChartTime, chartNotes[i].info.hold.headJudgeDirection, currentHoldInfo(headJudgeType),
                                          chartNotes[i].isBreak, true);
                }
            } else {
                //time for MATH
                //const Sint64 holdDivisor = holdLength - headAndTailSkipDuration;
                const Sint64 holdPercentage = currentHoldInfo(heldDuration) * 100 / maxDuration;
                //printf("held: %d\n", (int)holdPercentage);
                //printf("held duration: %d divisor: %d\n", (int)chartNotes[i].hold.heldDuration, (int)maxDuration);

                int holdJudgeType = currentHoldInfo(headJudgeType);
                if(holdJudgeType == judgePerfectCritical) {
                    if(holdPercentage < 33){
                        holdJudgeType = judgeGood;
                    } else if(holdPercentage < 67){
                        holdJudgeType = judgeGreatHigh;
                    } else if(holdPercentage < 100){
                        holdJudgeType = judgePerfectHigh;
                    }
                } else {
                    if(holdJudgeType == judgeMiss) {
                        if(holdPercentage >= 5) {
                            holdJudgeType = judgeGood;
                        }
                    } else if(holdJudgeType == judgeGood) {
                        if(holdPercentage >= 67) {
                            holdJudgeType = judgeGreatHigh;
                        }
                    } else {
                        if(holdPercentage < 33) {
                            holdJudgeType = judgeGood;
                        } else if(holdPercentage < 67 || holdJudgeType == judgeGreatLow || holdJudgeType == judgeGreatMid) {
                            holdJudgeType = judgeGreatHigh;
                        }

                        //use correct judgements
                        if(holdJudgeType == judgePerfectLow){
                            holdJudgeType = judgePerfectHigh;
                        }
                    }
                }

                //printf("head: %d real: %d\n", chartNotes[i].hold.headJudgeType, holdJudgeType);
                if(chartNotes[i].isEX && holdJudgeType == judgeGood){
                    holdJudgeType = judgeGreatHigh; //this correct?
                }

                if(isTouchHold){
                    setJudgementToTouchNote(chartNotes[i].location, currentChartTime, holdJudgeType, true, chartNotes[i].info.touchHold.fireworksEnabled);
                } else {
                    setJudgementToTapNote(chartNotes[i].location, currentChartTime, chartNotes[i].info.hold.headJudgeDirection, holdJudgeType,
                                          chartNotes[i].isBreak, true);
                }
            }
        }
    } else {
        if(isTooLateMissIfNotHit && (currentHoldInfo(headJudgeType) <= judgeMiss || chartTimeOffset >= holdLength)) {
            if(isTouchHold){
                chartNotes[i].info.touchHold.headJudgeType = clampInt(currentHoldInfo(headJudgeType), judgeMiss, judgeGood); //hit but 0% hold = miss fix
            } else {
                chartNotes[i].info.hold.headJudgeType = clampInt(currentHoldInfo(headJudgeType), judgeMiss, judgeGood); //hit but 0% hold = miss fix
                if(currentHoldInfo(headJudgeType) <= judgeMiss){
                    chartNotes[i].info.hold.headJudgeDirection = judgeLate;
                }
            }

            if(chartTimeOffset >= holdLength) {
                if(isTouchHold){
                    setJudgementToTouchNote(chartNotes[i].location, currentChartTime, currentHoldInfo(headJudgeType), true, chartNotes[i].info.touchHold.fireworksEnabled);
                } else {
                    setJudgementToTapNote(chartNotes[i].location, currentChartTime, chartNotes[i].info.hold.headJudgeDirection, currentHoldInfo(headJudgeType),
                                          chartNotes[i].isBreak, true);
                }
                chartNotes[i].hasHit = true;
            }
        }
    }

    if(isTouchHold){
        chartNotes[i].info.touchHold.oldChartOffset = chartTimeOffset;
    } else {
        chartNotes[i].info.hold.oldChartOffset = chartTimeOffset;
    }
    #undef currentHoldInfo
}

static SDL_Point nextValidTouchForFan(int stage, int side, int button){
    if(stage >= 4){
        return ((SDL_Point){-1, -1});
    }

    const SDL_Point left[4] = {{getSensor('A', offsetButton(1, button - 1)), -1},
                               {getSensor('B', offsetButton(8, button - 1)), -1},
                               {getSensor('B', offsetButton(7, button - 1)), -1},
                               {getSensor('A', offsetButton(6, button - 1)), getSensor('D', offsetButton(6, button - 1))}
    };

    const SDL_Point middle[4] = {{getSensor('A', offsetButton(1, button - 1)), -1},
                                 {getSensor('B', offsetButton(1, button - 1)), -1},
                                 {getSensor('C', 1), getSensor('C', 2)},
                                 {getSensor('B', offsetButton(5, button - 1)), getSensor('A', offsetButton(5, button - 1))}
    };

    const SDL_Point right[4] = {{getSensor('A', offsetButton(1, button - 1)), -1},
                                {getSensor('B', offsetButton(2, button - 1)), -1},
                                {getSensor('B', offsetButton(3, button - 1)), -1},
                                {getSensor('A', offsetButton(4, button - 1)), getSensor('D', offsetButton(5, button - 1))}
    };

    return side == -1 ? left[stage] : side == 0 ? middle[stage] : side == 1 ? right[stage] : ((SDL_Point){-2, -2});
}

static void handleFanSlide(int note, int touch, Sint64 currentChartTime, int eventType){
    int progressions[3] = {chartNotes[note].info.fanSlide.leftProgress, chartNotes[note].info.fanSlide.middleProgress, chartNotes[note].info.fanSlide.rightProgress};

    for(int i = 0; i < 3; i++){
        for(int n = 0; n < 3; n++){
            const int slideIndex = chartNotes[note].info.fanSlide.slideIndex;
            const SDL_Point nextTouch = nextValidTouchForFan((progressions[i] + n) / 2, i - 1, offsetButton(chartNotes[note].location, (slideIndex % 2 == 1) ? 4 : 0));
            if(!((nextTouch.x == touch || nextTouch.y == touch) && (progressions[i] + n) % 2 == eventType)){
                continue;
            }

            progressions[i] += 1 + n;
            if(progressions[0] < 7 || progressions[1] < 7 || progressions[2] < 7){
                break;
            }

            const int totalFanSlides = chartNotes[note].info.fanSlide.totalFanSlides;
            if(slideIndex < totalFanSlides - 1){
                chartNotes[note].info.fanSlide.slideIndex++;
                progressions[0] = 0;
                progressions[1] = 0;
                progressions[2] = 0;
                break;
            }

            const Sint64 slideLength = chartNotes[note].info.fanSlide.slideLength;
            const Sint64 slideEndHitOffset = currentChartTime - chartNotes[note].noteHitTime - chartNotes[note].info.fanSlide.delayLength - slideLength;
            int slideJudge = judgeMiss;
            double variableT = 0.0;
            //todo: try to reduce divisions/precision issues?
            const double* locationTimes = chartNotes[note].info.fanSlide.locationTimes;
            const double lastSlideTime = locationTimes == NULL ?
                                         (slideLength / (double)totalFanSlides / gameTicksPerSecond) :
                                         (locationTimes[totalFanSlides - 1]);
            if(i == 1){ //inner slide
                variableT = lastSlideTime * 0.1629;
                slideJudge = getJudgeFromSlide(slideEndHitOffset, variableT);
            } else { //outer slide
                variableT = lastSlideTime * 0.1793;
                slideJudge = getJudgeFromSlide(slideEndHitOffset, variableT);
            }

            const int slideJudgeDirection = slideJudge == judgePerfectCritical ? judgeCritical :
                                           (slideEndHitOffset + variableT * gameTicksPerSecond) < 0 ? judgeEarly : judgeLate;
            addSlideJudgement(offsetButton(chartNotes[note].location, (totalFanSlides % 2 == 1) * 4),
                              judgeSlideFan,
                              0,
                              currentChartTime,
                              slideJudgeDirection,
                              slideJudge,
                              chartNotes[note].isBreak,
                              false);
            chartNotes[note].hasHit = true;
            i = 9999; //outerloop moment
            break;
        }
    }
    chartNotes[note].info.fanSlide.leftProgress = progressions[0];
    chartNotes[note].info.fanSlide.middleProgress = progressions[1];
    chartNotes[note].info.fanSlide.rightProgress = progressions[2];
}

bool isValidSlideTouch(int noteI, int touch, int index, int eventType){
    const int touch1 = chartNotes[noteI].info.slide.startIndexes[index].touchLocation1;
    const int touch2 = chartNotes[noteI].info.slide.startIndexes[index].touchLocation2;
    if((touch == 8 * 2 || touch == 8 * 2 + 1) && (touch1 == 8 * 2 || touch1 == 8 * 2 + 1 || touch2 == 8 * 2 || touch2 == 8 * 2 + 1)){ //c sensor
        return chartNotes[noteI].info.slide.startIndexes[index].eventType == eventType;
    }
    return (touch1 == touch || touch2 == touch) && chartNotes[noteI].info.slide.startIndexes[index].eventType == eventType;
}

static void handleRegularSlide(int noteI, int touch, Sint64 currentChartTime, int eventType){
    const int lenOfStartIndex = chartNotes[noteI].info.slide.lenOfStartIndex;
    const bool isTransitionSlide = chartNotes[noteI].info.slide.isTransitionSlide;
    const int noSkip1 = (lenOfStartIndex <= 3) ? 0 : ((lenOfStartIndex <= 5 || isTransitionSlide) ? 2 : -1);
    const int noSkip2 = (isTransitionSlide && lenOfStartIndex == 9) ? 6 : -1;
    const int firstValidIndex = chartNotes[noteI].info.slide.indexOfStartIndex;
    int secondValidIndex = firstValidIndex + 1;
    int thirdValidIndex = secondValidIndex + 1;
    if(secondValidIndex >= lenOfStartIndex || firstValidIndex == noSkip1 || firstValidIndex == noSkip2) {
        secondValidIndex = firstValidIndex;
    }

    if(thirdValidIndex >= lenOfStartIndex || firstValidIndex == noSkip1 || firstValidIndex == noSkip2) {
        thirdValidIndex = secondValidIndex;
    }

    if(isValidSlideTouch(noteI, touch, firstValidIndex, eventType)) {
        chartNotes[noteI].info.slide.indexOfStartIndex = firstValidIndex + 1;
    } else if(isValidSlideTouch(noteI, touch, secondValidIndex, eventType)) {
        chartNotes[noteI].info.slide.indexOfStartIndex = secondValidIndex + 1;
    } else if(isValidSlideTouch(noteI, touch, thirdValidIndex, eventType)) {
        chartNotes[noteI].info.slide.indexOfStartIndex = thirdValidIndex + 1;
    }

    if(chartNotes[noteI].info.slide.indexOfStartIndex >= lenOfStartIndex) {
        const Sint64 slideEndHitOffset = currentChartTime - chartNotes[noteI].noteHitTime - chartNotes[noteI].info.slide.delayLength - chartNotes[noteI].info.slide.slideLength;
        const double variableT = chartNotes[noteI].info.slide.variableT;
        const int slideJudge = getJudgeFromSlide(slideEndHitOffset, variableT);
        const int slideJudgeDirection = slideJudge == judgePerfectCritical ? judgeCritical :
                                       (slideEndHitOffset + variableT * gameTicksPerSecond) < 0 ? judgeEarly : judgeLate;

        addSlideJudgement(chartNotes[noteI].info.slide.endButton,
                          chartNotes[noteI].info.slide.drawType,
                          chartNotes[noteI].info.slide.arrows[chartNotes[noteI].info.slide.lenOfArrows - 1].dir - 90,
                          currentChartTime,
                          slideJudgeDirection,
                          slideJudge,
                          chartNotes[noteI].isBreak,
                          chartNotes[noteI].info.slide.flipBigJudgeX);
        chartNotes[noteI].hasHit = true;
    }
}

Uint64 inGame(Uint64 delta) {
    renderFunction = renderInGame;
    subscreenRenderFunction = renderInGameInfoOnSubscreen;
    static int counter = 0;
    static Uint64 countDownToScoreScreen = 5 * gameTicksPerSecond;
    inGameTime += delta;
    inGameDelta = delta;

    //todo: find a way to avoid audio jump game time, also adapt when stutters
    const bool songExists = chartSong != NULL;
    if_unlikely(counter < 5) {
        counter++;
        inGameTime = 0;
    } else if(counter < 6 && inGameTime >= 1.950 * gameTicksPerSecond) { //debug
        if(songExists){
            MIX_PlayTrack(chartSong, 0);
        }
        counter++;
        playAnswerSoundsAsRequired();
    } else if(counter >= 6 && (inGameTime < 5 * gameTicksPerSecond && counter <= 512)) {
        counter++;
        if(songExists){ //todo: do something to get real seconds
            const Uint64 songTime = (Uint64)SDL_max(MIX_TrackFramesToMS(chartSong, MIX_GetTrackPlaybackPosition(chartSong)) + 2000, 0);
            inGameTime = SDL_max(inGameTime, songTime * (gameTicksPerSecond / 1000));
        }
        playAnswerSoundsAsRequired();
    } else {
        playAnswerSoundsAsRequired();
    }

    const Sint64 currentChartTime = ((Sint64)inGameTime - (gameTicksPerSecond * 2) - firstOffset - realChartOffset);

    for(int button = 1; button <= 8; button++) {
        if(pressedButtons[button - 1]) {
            //find button
            handleTapAndHold(button, heldByButton, currentChartTime - realInputOffset);
        }

        if(releasedButtons[button - 1]) {
            handleButtonRelease(button, heldByButton, currentChartTime - realInputOffset);
        }
        hasInputSinceMiss = hasInputSinceMiss || heldButtons[button - 1];
    }

    int firstTouchEventI = -1;
    int lastTouchEventI = -2;

    for(int touch = 0; touch < 8 * 4 + 2; touch++) { //no input offset for touch and touch hold
        if(touch < 8) {
            if(pressedTouchSensors[touch]) {
                handleTapAndHold(touch + 1, heldByTouchscreen, currentChartTime - realInputOffset);
            }

            if(releasedTouchSensors[touch]) {
                handleButtonRelease(touch + 1, heldByTouchscreen, currentChartTime - realInputOffset);
            }
        }

        if(heldTouchSensors[touch] || releasedTouchSensors[touch]){ //somehow faster
            if(firstTouchEventI <= -1){
                firstTouchEventI = touch;
            }
            lastTouchEventI = touch;
            hasInputSinceMiss = true;
        }

        if(pressedTouchSensors[touch] && (chartNoteFeatures[featureTouchNormal] ||
                                          chartNoteFeatures[featureTouchEach] ||
                                          chartNoteFeatures[featureTouchHold])) {
            //touch
            handleTouchAndTouchHold(touch, currentChartTime);
        }

        if(releasedTouchSensors[touch] && chartNoteFeatures[featureTouchHold]){ //improves performance slightly on charts without touch hold notes
            for(int noteI = minIndexToRender; noteI < chartLength; noteI++) {
                const Sint64 timeBeforeHit = chartNotes[noteI].noteHitTime - currentChartTime;
                if(timeBeforeHit > touchTimingWindows[0]){
                    break; //easy performance optimization
                }
                const bool isCSensor = touch == 8 * 2 || touch == 8 * 2 + 1;
                if(!chartNotes[noteI].hasHit &&
                   chartNotes[noteI].noteType == noteTypeTouchHold &&
                   (chartNotes[noteI].location == (isCSensor ? 8 * 2 : touch))) {

                    if(isCSensor){
                        if(touch % 8 == 0){
                            chartNotes[noteI].info.touchHold.isC1Held = false;
                        } else {
                            chartNotes[noteI].info.touchHold.isC2Held = false;
                        }
                        if(chartNotes[noteI].info.touchHold.isC1Held || chartNotes[noteI].info.touchHold.isC2Held){
                            break;
                        }
                    }

                    chartNotes[noteI].info.touchHold.isHeld = false;
                    chartNotes[noteI].info.touchHold.unheldOn = currentChartTime;
                    break;
                }
            }
        }
    }

    for(int i = minIndexToRender; i < chartLength; i++) {
        if(chartNotes[i].hasHit) { //ignore
            continue;
        }

        const Sint64 chartTimeOffset = currentChartTime - chartNotes[i].noteHitTime;
        //if a note is not hittable under any circumstances, then all notes after it is also unhittable, stop loop to improve performance drastically
        if(chartTimeOffset + SDL_max(-realInputOffset, 0) < -EXNoteTimingWindow){
            break;
        }

        switch(chartNotes[i].noteType) {
            case noteTypeTap:
                if((chartTimeOffset - realInputOffset) > EXNoteTimingWindow) {
                    setJudgementToTapNote(chartNotes[i].location, currentChartTime, judgeLate, judgeMiss, chartNotes[i].isBreak, false);
                    chartNotes[i].hasHit = true;
                }
                break;
            case noteTypeHold:
                handleHoldNote(i, gameTicksPerSecond / 10, currentChartTime - realInputOffset);
                break;
            case noteTypeSlide:;
                const Sint64 timeSpentInLastSensor = chartNotes[i].info.slide.variableT * gameTicksPerSecond; //todo: check for accuracy
                const Sint64 maxLateGood = SDL_max(lateSlideTimingWindows[4], tooLateSlideTiming + timeSpentInLastSensor);
                const Sint64 realSlideHitOffset = chartTimeOffset + timeSpentInLastSensor;
                if(realSlideHitOffset > (chartNotes[i].info.slide.delayLength + chartNotes[i].info.slide.slideLength + maxLateGood)) {
                    //slides that only have one sensor remaining and is too late is upgraded to good judge instead
                    const int slideJudge = chartNotes[i].info.slide.lenOfStartIndex - chartNotes[i].info.slide.indexOfStartIndex <= 1 ?
                                               judgeGood :
                                               judgeMiss;

                    addSlideJudgement(chartNotes[i].info.slide.endButton,
                                      chartNotes[i].info.slide.drawType,
                                      chartNotes[i].info.slide.arrows[chartNotes[i].info.slide.lenOfArrows - 1].dir - 90.f,
                                      currentChartTime,
                                      judgeLate,
                                      slideJudge,
                                      chartNotes[i].isBreak,
                                      chartNotes[i].info.slide.flipBigJudgeX);
                    chartNotes[i].hasHit = true;
                } else if(firstTouchEventI >= 0 && chartTimeOffset >= gameTicksPerSecond / 10){
                    int oldIndex = chartNotes[i].info.slide.indexOfStartIndex;
                    int newIndex = oldIndex;
                    do {
                        oldIndex = newIndex;
                        for(int touch = firstTouchEventI; touch <= lastTouchEventI && !chartNotes[i].hasHit; touch++) {
                            if(pressedTouchSensors[touch] || heldTouchSensors[touch]){
                                handleRegularSlide(i, touch, currentChartTime, slideEventOn);
                            }

                            if(releasedTouchSensors[touch]){
                                handleRegularSlide(i, touch, currentChartTime, slideEventUp);
                            }
                        }
                        newIndex = chartNotes[i].info.slide.indexOfStartIndex;
                    } while(oldIndex != newIndex);
                }
                break;
            case noteTypeFanSlide:;
                const Sint64 maxTimeSpentInOuterFanSensor = 0.1793 * gameTicksPerSecond; //todo: check for accuracy
                const Sint64 maxFanLateGood = SDL_max(lateSlideTimingWindows[4], tooLateSlideTiming + maxTimeSpentInOuterFanSensor);
                const Sint64 realFanSlideHitOffset = chartTimeOffset + maxTimeSpentInOuterFanSensor;
                if(realFanSlideHitOffset > (chartNotes[i].info.fanSlide.delayLength + chartNotes[i].info.fanSlide.slideLength + maxFanLateGood)) {
                    //slides that only have one sensor remaining and is too late is upgraded to good judge instead
                    const bool isMinJudgeGood = chartNotes[i].info.fanSlide.slideIndex == chartNotes[i].info.fanSlide.totalFanSlides - 1 &&
                                                chartNotes[i].info.fanSlide.leftProgress >= 6 &&
                                                chartNotes[i].info.fanSlide.middleProgress >= 6 && //todo: make middle one have to be fully completed?
                                                chartNotes[i].info.fanSlide.rightProgress >= 6;
                    const int slideJudge = isMinJudgeGood ?
                                               judgeGood :
                                               judgeMiss;

                    addSlideJudgement(offsetButton(chartNotes[i].location, 4),
                                      judgeSlideFan,
                                      0,
                                      currentChartTime,
                                      judgeLate,
                                      slideJudge,
                                      chartNotes[i].isBreak,
                                      false);
                    chartNotes[i].hasHit = true;
                } else if(firstTouchEventI >= 0 && chartTimeOffset >= gameTicksPerSecond / 10){
                    int oldProgressionL = chartNotes[i].info.fanSlide.leftProgress;
                    int oldProgressionM = chartNotes[i].info.fanSlide.middleProgress;
                    int oldProgressionR = chartNotes[i].info.fanSlide.rightProgress;

                    int newProgressionL = oldProgressionL;
                    int newProgressionM = oldProgressionM;
                    int newProgressionR = oldProgressionR;
                    do {
                        oldProgressionL = newProgressionL;
                        oldProgressionM = newProgressionM;
                        oldProgressionR = newProgressionR;
                        for(int touch = firstTouchEventI; touch <= lastTouchEventI && !chartNotes[i].hasHit; touch++) {
                            if(pressedTouchSensors[touch] || heldTouchSensors[touch]){
                                handleFanSlide(i, touch, currentChartTime, slideEventOn);
                            }

                            if(releasedTouchSensors[touch]){
                                handleFanSlide(i, touch, currentChartTime, slideEventUp);
                            }
                        }
                        newProgressionL = chartNotes[i].info.fanSlide.leftProgress;
                        newProgressionM = chartNotes[i].info.fanSlide.middleProgress;
                        newProgressionR = chartNotes[i].info.fanSlide.rightProgress;
                    } while(oldProgressionL != newProgressionL || oldProgressionM != newProgressionM || oldProgressionR != newProgressionR);
                }
                break;
            case noteTypeTouch:
                if(chartTimeOffset > touchTimingWindows[6]) {
                    setJudgementToTouchNote(chartNotes[i].location, currentChartTime, judgeMiss, false, chartNotes[i].info.touch.fireworksEnabled);
                    chartNotes[i].hasHit = true;
                }
                break;
            case noteTypeTouchHold:
                handleHoldNote(i, gameTicksPerSecond / 4, currentChartTime);
            default:
                break;
        }
    }

    static bool isSafeToFadeIn = false;

    const bool buttonTrackSkip = (trackSkip % 2 == 1) &&
                                       heldButtons[1] &&
                                       heldButtons[2] &&
                                       heldButtons[5] &&
                                       heldButtons[6]; //buttons 2, 3, 6, 7
    const bool noTouchTrackSkip = trackSkip >= 2 && !hasInputSinceMiss && !isSafeToFadeIn;

    if(buttonTrackSkip || noTouchTrackSkip){
        if(buttonTrackSkip){ //make button track skip bit faster
            timeBeforeTrackSkip = SDL_min(timeBeforeTrackSkip, 7 * gameTicksPerSecond / 2);
        }
        subtractUnsignedNumberToAtLeast0(timeBeforeTrackSkip, delta);
        if(timeBeforeTrackSkip == 0){
            //force end of song
            countDownToScoreScreen = 0;
            minIndexToRender = chartLength - 1;
        }
    } else {
        timeBeforeTrackSkip = 4 * gameTicksPerSecond;
    }


    if(minIndexToRender >= chartLength - 1) {
        if(!isSafeToFadeIn && timeBeforeTrackSkip > 0){ //prevent track skip from erroring
            for(int i = 0; i < chartLength; i++){
                if(!chartNotes[i].hasHit){
                    return 0;
                }
            }
            isSafeToFadeIn = true;
            FCType = getFCMedal();
            if(FCType >= medalTypeFC){
                playIngameSound(fullComboSfx);
            }
        }

        if(!isSafeToFadeIn && timeBeforeTrackSkip > 0){
            return 0;
        }

        subtractUnsignedNumberToAtLeast0(countDownToScoreScreen, delta);
        if(FCType >= medalTypeFC){
            timeSinceFCShown += delta;
            timeSinceLastOffsetCall += delta;
        }

        if(countDownToScoreScreen >= 2 * gameTicksPerSecond) {
            return 0;
        }

        globalBrightness = convertTimerAndDurationToBrightness(countDownToScoreScreen, 2.0 * gameTicksPerSecond);

        if(countDownToScoreScreen > 0) {
            return 0;
        }

        countDownToScoreScreen = 5 * gameTicksPerSecond;
        minIndexToRender = 0;
        minIndexToPlaySounds = 0;
        generalMaxI = 0;
        touchMaxI = 0;
        chartLength = 0;
        MIX_StopAllTracks(mainMixer, 0);
        counter = 0;
        FCType = medalTypeNone;
        timeSinceFCShown = 0;
        timeSinceLastOffsetCall = 0;
        timeBeforeTrackSkip = 4 * gameTicksPerSecond;
        SDL_free(answerPlayTimes);
        answerPlayTimes = NULL;
        isSafeToFadeIn = false;
        hasInputSinceMiss = true;
        #ifdef Circular8_ENABLE_CONTROLLER
        if(isUsingSerialLightController){
            setVirtualLightForMultipleButtons(1, 8, 128, 128, 128);
            for(int i = 0; i < stackArraySizeInt(judgeButtonLED); i++){ //prevent leds misfiring after 1st song
                judgeButtonLED[i].judgementType = judgeMiss;
            }
            tryToMakeVirtualLightsReal();
        }
        #endif // Circular8_ENABLE_CONTROLLER
        runLoadFunction(loadScoreScreen, scoreScreenSlot);
    } else {
        globalBrightness = 255;
    }
    return 0;
}







