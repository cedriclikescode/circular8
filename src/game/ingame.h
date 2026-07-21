#pragma once
#ifndef Circular8_INGAME_H
#define Circular8_INGAME_H

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
Uint64 inGame(Uint64 delta);

//good
enum {
    judgeMiss = 0,
    judgeGood = 1,
    judgeGreatLow = 2,
    judgeGreatMid = 3,
    judgeGreatHigh = 4,
    judgePerfectLow = 5,
    judgePerfectHigh = 6,
    judgePerfectCritical = 7
};
enum {
    judgeEarly = -1, //fast
    judgeCritical = 0, //critical
    judgeLate = 1 //late
};

typedef struct generalJudgementProperties {
    Sint64 judgementDisplayTimer;
    int judgementType;
    int judgementDirection; //early, critical or late
} generalJudgementProperties;

typedef struct holdJudgementProperties {
    Sint64 judgementDisplayTimer;
    Sint64 endTime;
    int judgementType;
    int judgementDirection; //early, critical or late
} holdJudgementProperties;

typedef struct slideJudgementProperties {
    generalJudgementProperties properties;
    union special {
        double angle;
        bool arcSlideFlipHorizontally;
    } special;
    int location;
    int drawType; //line, arc, fan
} slideJudgementProperties;


//touch notes have only critical or late
typedef struct touchJudgementProperties {
    Sint64 judgementDisplayTimer;
    int judgementType;
} touchJudgementProperties;

typedef struct touchHoldJudgementProperties {
    Sint64 judgementDisplayTimer;
    Sint64 endTime;
    int judgementType;
} touchHoldJudgementProperties;

enum {
    judgeSlideLine = 0,
    judgeSlideArc = 1,
    judgeSlideFan = 2
};

//smaller struct


//answer sound effects
typedef struct answerSfxInfo {
    Sint64 playSoundAt;
    int soundType;
} answerSfxInfo;

extern answerSfxInfo* answerPlayTimes;

extern generalJudgementProperties judgementInfoForButton[8];
extern holdJudgementProperties judgementInfoForHeadHold[8];
extern slideJudgementProperties judgementInfoForSlideEnd[64];
extern int lenOfJudgementInfoForSlideEnd;
extern touchJudgementProperties judgementInfoForTouch[8 * 4 + 2];
extern touchHoldJudgementProperties judgementInfoForTouchHold[8 * 4 + 2];

extern Sint64 firstOffset;

extern MIX_Audio* tapNoteSfx;
extern MIX_Audio* holdNoteSfx;
extern MIX_Audio* slideStartSfx;
extern MIX_Audio* touchNoteSfx;
extern MIX_Audio* breakNoteSfx;
extern MIX_Audio* EXNoteSfx;
extern MIX_Audio* fireworkSfx;
extern MIX_Audio* fullComboSfx;
extern MIX_Track* ingameTracks[32];
extern int indexOfIngameTracks;

enum {
    tapNoteSfxNum = 0,
    holdNoteSfxNum = 1,
    slideStartSfxNum = 2,
    breakNoteSfxNum = 3,
    touchNoteSfxNum = 4,
    EXNoteSfxNum = 5,
    fireworkSfxNum = 6
};

bool isValidSlideTouch(int noteI, int touch, int index, int eventType);
#define gameTicksPerSecond (1000000000LL)
extern SDL_Texture* centerDisplayTopInfo;
extern SDL_Texture* trackSkippingText;
extern SDL_Texture* centerDisplayTopDXStarsInfo[6];
extern SDL_Texture* gameBackground;
extern unsigned int comboCount;
extern int FCType;
extern SDL_Texture* userScreenScore;
extern SDL_Texture* songInfo;
extern SDL_Texture* subScreenDifficultyName;
extern SDL_Texture* subScreenDifficultyLevel;
extern SDL_Texture* subScreenSongTitle;
int renderInGameInfoOnSubscreen(void);
void renderDifficultyInfo(float offsetX, float offsetY);
extern int minIndexToPlaySounds;
extern int lenOfAnswerPlayTimes;

#ifdef Circular8_ENABLE_CONTROLLER
extern generalJudgementProperties judgeButtonLED[8];
extern const SDL_Color buttonLEDColors[4];
#endif // Circular8_ENABLE_CONTROLLER
#endif // Circular8_INGAME_H
