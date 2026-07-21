#pragma once
#ifndef Circular8_OPTIONS_H
#define Circular8_OPTIONS_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#ifdef Circular8_ENABLE_CONTROLLER
extern bool enableCustomControllerPorts;
#endif // Circular8_ENABLE_CONTROLLER
Uint64 gameOptions(Uint64 delta);
extern double chartOffset60fps;
extern double inputOffset60fps;

extern Sint64 realChartOffset;
extern Sint64 realInputOffset;
extern bool rotateStarNote;
extern int showFastLate;
extern int showCriticalPerfect;
extern int showSensors;
extern int displayType;
extern double machineMasterVolume;

extern double tapSpeed;
extern double touchSpeed;
extern int mirrorType;
extern int trackSkip;
extern double backgroundBrightness;
extern bool useAlternateStarNote;
extern SDL_Texture* judgeDisplaySpeedTypes[5];

extern double gameplaySfxVolume;
extern double userMusicVolume;
extern bool enableFireworksSFX;
extern bool enableSlideSFX;
extern double guideOffset60fps;
extern Sint64 realGuideOffset;

extern int optionVSync;
extern int optionIngameVSync;

extern SDL_Texture* optionOffOnTexts[2];

extern SDL_Texture* groupOptions[4];

extern SDL_Texture* gameplayOptions[5];
extern SDL_Texture* displayOptions[4];
extern SDL_Texture* judgeOptions[4];
extern SDL_Texture* audioOptions[5];

extern SDL_Texture* mirrorTexts[3];
extern SDL_Texture* trackSkipTexts[3];

extern SDL_Texture* slideTapNoteNames[2];

extern SDL_Texture* sensorTypes[3];
extern SDL_Texture* fastLateTypes[3];
extern SDL_Texture* criticalPerfectTypes[2];

enum {
    achievementNone,
    achievementCombo,
    achievementPlus,
    achievementAverage, //unofficial achievement type that I invented
    achievementMinus1,
    achievementMinus2,
    achievementBorderS,
    achievementBorderSS,
    achievementBorderSSS,
    achievementMyBest,
    achievementDXScorePlus,
    achievementDXScoreMinus,
    achievementDXScoreStars,
};

extern SDL_Texture* achievementTexts[achievementDXScoreStars];
extern SDL_Texture* changeMinus;
extern SDL_Texture* changePlus;

void loadOptionsINI(const char* optionsName);
extern bool useSubscreen;
extern int rotateScreenBy;
#endif // Circular8_OPTIONS_H
