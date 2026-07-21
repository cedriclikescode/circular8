#pragma once
#ifndef Circular8_SCORESCREEN_H
#define Circular8_SCORESCREEN_H

#include <stdbool.h>
#include <SDL3/SDL.h>

Uint64 scoreScreen(Uint64 delta);
extern int rankIndex;
extern unsigned int currentBaseScore;
extern unsigned int currentMaxBaseScore;
extern unsigned int absoluteMaxBaseScore;

extern unsigned int currentBreakBonus;
extern unsigned int currentMaxBreakBonus;
extern unsigned int absoluteMaxBreakBonus;

extern unsigned int currentDXScore;
extern unsigned int currentMaxDXScore;
extern unsigned int absoluteMaxDXScore;
extern const unsigned int pointsTable[5][8];
double calculateScore(unsigned int baseScore, unsigned int maxBaseScore, unsigned int breakBonus, unsigned int maxBreakBonus);

extern SDL_Texture* scoreScreenSongBG;
extern SDL_Texture* scoreScreenScore;
extern SDL_Texture* scoreScreenRank;
extern SDL_Texture* scoreScreenDXScore;
extern SDL_Texture* scoreScreenFCBadge;

extern const unsigned int DXScoreTable[8];
extern unsigned int judgementCounts[8];
SDL_Point getMaxDXStarsAndHP(unsigned int baseDXScore, unsigned int maxDXScore, unsigned int maxStars);
void calculateDXStars(char* dest, size_t maxLen, unsigned int baseDXScore, unsigned int maxDXScore);

enum {
    medalTypeNone = -1,
    medalTypeFC = 0,
    medalTypeFCPlus = 1,
    medalTypeAP = 2,
    medalTypeAPPlus = 3,
    medalTypeACP = 4
};

int getFCMedal(void);

typedef struct scoreInfo {
    char* checksum;
    char* level;
    unsigned int judgementCounts[8];
    unsigned int score[4];
    int chartIndex;
} scoreInfo;

void loadScoreFile(void);
void updateScoreFile(void);
extern bool inScoreScreen;
extern double addedRating;
extern SDL_Texture* addedRatingTexture;

double getBestScoreFromChecksumAndDifficulty(char* checksum, int difficxulty);
extern double highScore;
extern bool enableRating;
#endif // Circular8_SCORESCREEN_H
