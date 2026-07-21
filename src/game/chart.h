#pragma once
#ifndef Circular8_CHART_H
#define Circular8_CHART_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "game/songselect.h"

enum {
    noteTypeTap = 0,
    noteTypeHold = 1,
    noteTypeSlide = 2,
    noteTypeTouch = 3,
    noteTypeTouchHold = 4,
    noteTypeFanSlide = 5
};

enum {
    slideTypeEnd = -1, //no real purpose other than for chart loading
    slideTypeStraightLine = 0, // -
    slideTypeAutoArc = 1, // ^
    slideTypeArcLeft = 2, // <
    slideTypeArcRight = 3, // >
    slideTypeVShape = 4,
    slideTypeSShape = 5,
    slideTypeZShape = 6,
    slideTypePShape = 7, //clockwise
    slideTypeQShape = 8, //counter clockwise
    slideTypePPShape = 9, //clockwise
    slideTypeQQShape = 10, //counter clockwise
    slideTypeReflectiveShapeCW = 11, //V
    slideTypeReflectiveShapeCCW = 12, //V
    slideTypeFan = 13 //w
};

enum {
    lastZoneLine = 0,
    lastZoneArcCW = 1,
    lastZoneArcCCW = 2,
    lastZoneP = 3,
    lastZoneQ = 4,
    lastZoneZigZag = 5,
    lastZonev = 6,
    lastZonePP = 7,
    lastZoneQQ = 8,
    lastZoneReflectiveCW = 9,
    lastZoneReflectiveCCW = 10,
    lastZoneFanL = 11,
    lastZoneFanM = 12,
    lastZoneFanR = 13
};


enum { //different type of skins for tap note
    tapNoteNormalSkin = 0, //normal
    tapNoteStarSkin = 1,
    tapNoteMultiStarSkin = 2
};

enum {
    heldByNeither = -1,
    heldByButton = 0,
    heldByTouchscreen = 1,
    heldByBoth = 2
};

enum {
    normalSlideRatio = 0,
    customSlideDuration = 1,
    customBPMNormalSlideRatio = 2,
    customBPMAndSlideDuration = 3,
    customWaitingTimeAndSlideDuration = 4,
    customWaitingTimeNormalSlideRatio = 5,
    customWaitingTimeAndBPMNormalSlideRatio = 6
};

typedef struct slidePointInfo {
    double arrowTime;
    float x;
    float y;
    float dir;
    bool isVisible;
} slidePointInfo;

typedef struct slideItem {
    int location;
    int slideType;
} slideItem;

typedef struct slideTouchItem {
    int touchLocation1;
    int touchLocation2; //optional
    int eventType; //on or up
    int newArrowIndex;
} slideTouchItem;

enum { //used to only load images that are used
    featureTapNormal,
    featureTapEach,
    featureTapBreak,

    featureHoldNormal,
    featureHoldEach,
    featureHoldBreak,

    featureSlideTapNormal,
    featureSlideTapEach,
    featureSlideTapBreak,

    featureMultiSlideTapNormal,
    featureMultiSlideTapEach,
    featureMultiSlideTapBreak,

    featureSlideNormal,
    featureSlideEach,
    featureSlideBreak,

    featureFanSlideNormal,
    featureFanSlideEach,
    featureFanSlideBreak,

    featureTouchNormal,
    featureTouchEach,

    featureTouchHold,
    featureEX,

    featureFireworks
};


enum stages {
    scanning = 0,
    comment = 1,
    BPMFinder = 2,
    BPMDivisor = 3,
    PCheck = 4,
    QCheck = 5,
    ReflectiveCheck = 6,
    //touch note is in note and holdnote stage
    notes = 7,
    holdnote = 8,
    slidenote = 9,
    fanslidenote = 10,
};

enum {
    slideNoRestrictions = 0,
    slideAtLeastTwoDist = 1,
    slideAtExactlyTwoDist = 2,
    slideDistNotFourOrZero = 3,
    slideEndExactlyOpposite = 4
};

//https://www.bilibili.com/opus/970660281077202961
enum {
    slideEventOn = 0,
    slideEventUp = 1
};

//new struct (union used, could be better)
typedef struct noteObject {
    Sint64 noteHitTime;

    int noteType;
    int location; //is sensor if touch or touch hold

    bool hasHit;
    bool isBreak;
    bool isEX;
    bool isEach;

    union info {
        struct tap {
            double starNoteRotationSpeed;
            int tapNoteSkin;
        } tap;

        struct hold {
            Sint64 holdLength;
            Sint64 heldDuration; //god's work
            Sint64 oldChartOffset; //used only in judgement correction thing
            Sint64 unheldOn; //graphical effects only
            int inputType; //might be useful in utage charts
            int headJudgeType;
            int headJudgeDirection;
            bool stopChecking;
            bool hasStartedToCheck;
            bool isHeld;
            bool hasAcknowledgedHeld;
        } hold;

        struct slide { //on or off
            Sint64 delayLength;
            Sint64 slideLength;
            double variableT; //time inside the last sensor
            slidePointInfo* arrows; //display arrows location and direction
            slideTouchItem* startIndexes;
            int indexOfLocationTimes;
            int lenOfArrows;
            int lenOfStartIndex; //display from this to end
            int indexOfStartIndex; //display from this to end
            int endButton;
            int drawType;
            bool suddenStarAppearance; //opcode !
            bool flipBigJudgeX;
            bool isTransitionSlide; //is it just for V slides (e.g. 1V35) or can it be others?
        } slide;

        struct fanSlide {
            Sint64 delayLength;
            Sint64 slideLength; //for (touch) holds and slides only
            double* locationTimes;
            int indexOfLocationTimes;
            int leftProgress;
            int middleProgress; //todo: apparently theres bug where if this is not fully completed then late good promotion can never happen, emulate it?
            int rightProgress;
            //1w5w1w5 moment
            int totalFanSlides;
            int slideIndex;
            bool suddenStarAppearance; //opcode !
        } fanSlide;

        struct touch {
            int* connectedNotes; //51% rule
            int lenOfConnectedNotes;
            int noteID; //for touch group and the 51% rule
            bool fireworksEnabled;
        } touch;

        struct touchHold { //51% rule apparently applies too (todo: does it also impact holding them and is it shared between touch and touch hold?)
            Sint64 holdLength;
            Sint64 heldDuration; //god's work
            Sint64 oldChartOffset; //used only in judgement correction thing
            Sint64 unheldOn; //graphical effects only
            int* connectedNotes; //51% rule
            int lenOfConnectedNotes;
            int noteID; //for touch group and the 51% rule
            int headJudgeType;
            bool stopChecking;
            bool hasStartedToCheck;
            bool isHeld;
            bool hasAcknowledgedHeld;
            bool isC1Held;
            bool isC2Held;
            bool fireworksEnabled;
        } touchHold;
    } info;
} noteObject;

extern int chartLength;

typedef struct chartErrorRange {
    int start;
    int length;
} chartErrorRange;

chartErrorRange readChart(int startIndex, const char* chartLocation, const char* chartHash);
void generateTouchSensors(double dist);
extern MIX_Track* chartSong;
extern MIX_Audio* chartAudio;
extern noteObject* chartNotes;
extern SDL_FPoint touchLocations[8 * 4 + 2];

bool getInfoAboutMaidatatxt(songFile* chart); //puts info about maidata.txt into selected songfile
#define buttonNumberToDegrees(buttonNumber) (-22.5 + (buttonNumber) * 45.0)
#define buttonToNormalizedDegrees(buttonNumber) (-0.0625f + (float)(buttonNumber) * 0.125f)
#define offsetButton(buttonNumber, offsetValue) ((((buttonNumber) - 1 + (offsetValue)) % 8) + 1)

extern const double borderRadius;
extern const double travelDistance;

extern SDL_FPoint ASensorPoints[9];
extern const SDL_FPoint BSensorPoints[7];
extern SDL_FPoint CSensorPoints[6];
extern SDL_FPoint DSensorPoints[6];
extern const SDL_FPoint ESensorPoints[4];
int getTouchSensorAtFPoint(SDL_FPoint location);
int getSensor(int sensor, int button);
extern bool chartNoteFeatures[32];
#endif // Circular8_CHART_H
