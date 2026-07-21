#pragma once
#ifndef Circular8_RENDERSONG_H
#define Circular8_RENDERSONG_H

#include <SDL3/SDL.h>
/*
//tap
extern SDL_Texture* normalTapNote;
extern SDL_Texture* breakTapNote;
extern SDL_Texture* eachTapNote;
extern SDL_Texture* EXNoteBlur;

//slide tap
extern SDL_Texture* normalSlideNote;
extern SDL_Texture* breakSlideNote;
extern SDL_Texture* eachSlideNote;

//multi slide tap
extern SDL_Texture* normalMultiSlideNote;
extern SDL_Texture* breakMultiSlideNote;
extern SDL_Texture* eachMultiSlideNote;

//hold
extern SDL_Texture* normalHoldNoteSize0;
extern SDL_Texture* breakHoldNoteSize0;
extern SDL_Texture* eachHoldNoteSize0;

extern SDL_Texture* normalHoldNoteSize1;
extern SDL_Texture* breakHoldNoteSize1;
extern SDL_Texture* eachHoldNoteSize1;
extern SDL_Texture* unheldHoldNoteSize1;

extern SDL_Texture* holdNoteHitMarker;

//touch notes
extern SDL_Texture* normalTouchPoint;
extern SDL_Texture* eachTouchPoint;
extern SDL_Texture* normalTouchArrow;
extern SDL_Texture* eachTouchArrow;

//multi touch
extern SDL_Texture* multiNormalTouchBox;
extern SDL_Texture* multiEachTouchBox;

extern SDL_Texture* holdTouchTriangles[4];
extern SDL_Texture* holdTouchProgression;
extern SDL_Texture* holdTouchProgressionUnheld;*/

//fireworks
//extern SDL_Texture* fireworksTexture;
extern SDL_Texture* fireworksTexture;
extern int fireworksTouchLocation;
extern double fireworksStartTime;

/*
//slide arrows
extern SDL_Texture* normalSlideArrow;
extern SDL_Texture* breakSlideArrow;
extern SDL_Texture* eachSlideArrow;

//fan slide arrows
extern SDL_Texture* normalFanSlideArrows[11];
extern SDL_Texture* breakFanSlideArrows[11];
extern SDL_Texture* eachFanSlideArrows[11];

extern SDL_Texture* judgements[5];
extern SDL_Texture* criticalJudge;
extern SDL_Texture* alternativeJudgements[5];
extern SDL_Texture* fastLateJudgements[2];
extern SDL_Texture* slideBigJudgements[3];
extern SDL_Texture* slideInfoJudgements[2];*/

extern int minIndexToRender;
int renderBeatMap(double generalMoveTime, double touchMoveTime, Sint64 gameTime, float offsetX, float offsetY);

extern SDL_Texture* sensors[5];
extern SDL_FRect sensorFRects[5];
extern SDL_FPoint renderingSensorFPoints[34];

SDL_FRect moveNoteObject(SDL_Texture *texture, float x, float y, float scale, int noteLocation, float distance);
SDL_FPoint moveNoteXY(float x, float y, int noteLocation, float distance);
extern SDL_Texture* gameBorder;
extern SDL_Texture* FCMedals[5];

typedef struct slideStarInfo {
    SDL_FRect starFRect;
    SDL_FPoint starFPoint;
    float scale;
    float dir;
    int starType;
    Uint8 alpha;
} slideStarInfo;

//texture atlas
extern int normalTapNoteID;
extern int breakTapNoteID;
extern int eachTapNoteID;
extern int EXNoteBlurID;

//slide tap
extern int normalSlideNoteID;
extern int breakSlideNoteID;
extern int eachSlideNoteID;
//multi slide tap
extern int normalMultiSlideNoteID;
extern int breakMultiSlideNoteID;
extern int eachMultiSlideNoteID;

//hold
extern int normalHoldNoteSize0ID; //the bane of my existence 1
extern int breakHoldNoteSize0ID; //the bane of my existence 2
extern int eachHoldNoteSize0ID; //the bane of my existence 3

extern int normalHoldNoteSize1ID; //the bane of my existence 4
extern int breakHoldNoteSize1ID; //the bane of my existence 5
extern int eachHoldNoteSize1ID; //the bane of my existence 6
extern int unheldHoldNoteSize1ID; //the bane of my existence 7

extern int holdNoteHitMarkerID; //the bane of my existence 8

//slide arrows
extern int normalSlideArrowID; //the actual bane of my existence 1
extern int breakSlideArrowID; //the actual bane of my existence 2
extern int eachSlideArrowID; //the actual bane of my existence 3

//the finale of the slides
extern int normalFanSlideArrowsStartID;
extern int breakFanSlideArrowsStartID;
extern int eachFanSlideArrowsStartID;

//touch notes
extern int normalTouchPointID;
extern int eachTouchPointID;

extern int normalTouchArrowID;
extern int eachTouchArrowID;
//when there are multiple touches in same area
extern int multiNormalTouchBoxID;
extern int multiEachTouchBoxID;

//fireworks
extern int fireworksTextureID;

//hold touch note
extern int holdTouchTrianglesStartID;
extern int holdTouchProgressionID;
extern int holdTouchProgressionUnheldID;

//judgements
extern int judgementsStartID; //miss, good, great, perfect, critical perfect
extern int alternativeJudgementsStartID; //miss, good, great, perfect, critical perfect
extern int fastLateJudgementsStartID; //fast, late

extern int slideBigJudgementsStartID; //line, arc, fan
extern int slideInfoJudgementsStartID; //Too late, Just, Critical is in criticalJudge and fast/late is used in fastLateJudgements
extern int criticalJudgeID;

extern int generalMaxI;
extern int touchMaxI;
#endif // Circular8_RENDERSONG_H
