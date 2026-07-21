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

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "main.h"

#include "render.h"
#include "events.h"
#include "load.h"
#include "gameinit.h"
#include "texture.h"
#include "language.h"
#include "miscutils.h"
#ifdef Circular8_ENABLE_FFmpeg
#include "video.h"
#endif // Circular8_ENABLE_FFmpeg

#include "game/songselect.h"
#include "game/chart.h"
#include "game/ingame.h"
#include "game/difficultyselect.h"
#include "game/scorescreen.h"
#include "game/profileselect.h"

#include "thirdparty/inih/ini.h"

SDL_Texture* scoreScreenSongBG = NULL;
SDL_Texture* scoreScreenScore = NULL;
SDL_Texture* scoreScreenRank = NULL;
SDL_Texture* scoreScreenDXScore = NULL;
SDL_Texture* scoreScreenFCBadge = NULL;

//https://listed.to/@donmai/45107/exploring-the-algorithm-behind-maimai-dx-s-scoring-and-dx-rating-computation

//original table
//const unsigned int pointsTable[5][8] = {
/*                miss   good  g.low  g.mid g.high  p.low p.high  p.crit */
/* Tap */        //{   0,   250,   400,   400,   400,   500,   500,    500},
/* Hold */       //{   0,   500,   800,   800,   800,  1000,  1000,   1000},
/* Slide */      //{   0,   750,  1200,  1200,  1200,  1500,  1500,   1500},
/* Break Base*/  //{   0,  1000,  1250,  1500,  2000,  2500,  2500,   2500},
/* Break Bonus*/ //{   0,    30,    40,    40,    40,    50,    75,    100}
//};

//new table (normalized)
const unsigned int pointsTable[5][8] = {
/*                miss   good  g.low  g.mid g.high  p.low p.high  p.crit */
/* Tap */        {   0,     5,     8,     8,     8,    10,    10,     10},
/* Hold */       {   0,    10,    16,    16,    16,    20,    20,     20},
/* Slide */      {   0,    15,    24,    24,    24,    30,    30,     30},
/* Break Base*/  {   0,    20,    25,    30,    40,    50,    50,     50},
/* Break Bonus*/ {   0,     6,     8,     8,     8,    10,    15,     20}
};

scoreInfo* scoreHistory = NULL;
int lengthOfScoreHistory = 0;

const unsigned int DXScoreTable[8] = {0, 0, 1, 1, 1, 2, 2, 3};

unsigned int judgementCounts[8] = {0};

int rankIndex = 0;

double highScore = -1;

bool inScoreScreen = false;
bool enableRating = false;
double addedRating = 0;
SDL_Texture* addedRatingTexture = NULL;

static double achievementTable[12][3] = {
    /*score*/ /*multiplier*/ /*level offset*/
    { 60,       0.1,            -5},
    { 70,       0.2,            -4},
    { 75,       0.3,            -3},
    { 80,       0.5,            -2},
    { 90,       0.65,           -1.5},
    { 94,       0.85,           -1},
    { 97,       1.0,             0},
    { 98,       1.0,             0.2},
    { 99,       1.0,             0.3},
    { 99.5,     1.0,             0.4},
    {100,       1.0,             0.55},
    {100.5,     1.0,             0.65}
};

static double getRealLevel(const char* level){
    char* levelEnd;
    double realLevel = SDL_strtod(level, &levelEnd);
    if(*levelEnd == '+'){
        realLevel += 0.7;
    }
    return realLevel;
}

static double getRatingPoints(const scoreInfo* score, double achievement){
    //const double actualAchievement = SDL_min(achievement, 100.5);
    const int len = stackArraySizeInt(achievementTable);
    double ratingPoints = 0;
    for(int i = len - 1; i >= 0; i--){
        if(achievementTable[i][0] > achievement){
            continue;
        }

        if(i == len - 1){
            ratingPoints = getRealLevel(score->level) * achievementTable[i][1] + achievementTable[i][2];
            break;
        }
        const double distance = achievementTable[i + 1][0] - achievementTable[i][0];
        const double progress = (achievement - achievementTable[i][0]) / distance;
        const double multiplier = interpolateDouble(achievementTable[i][1], achievementTable[i + 1][1], progress);
        const double addition = interpolateDouble(achievementTable[i][2], achievementTable[i + 1][2], progress);
        ratingPoints = getRealLevel(score->level) * multiplier + addition;
        break;
    }
    return SDL_max(ratingPoints, 0);
}

static double getAchievement(const scoreInfo* score){
    return calculateScore(score->score[0], score->score[1], score->score[2], score->score[3]);
}

static int SDLCALL ratingSortingCompare(const void *a, const void *b) {
    const scoreInfo* A = (const scoreInfo*)a;
    const scoreInfo* B = (const scoreInfo*)b;
    const double ratingA = getRatingPoints(A, getAchievement(A));
    const double ratingB = getRatingPoints(B, getAchievement(B));
    return ratingA > ratingB ? -1 : ratingA < ratingB ? 1 : 0;
}

static int SDLCALL checksumSortingCompare(const void *a, const void *b) {
    const scoreInfo* A = (const scoreInfo*)a;
    const scoreInfo* B = (const scoreInfo*)b;
    return SDL_strcmp(A->checksum, B->checksum);
}

static double getRating(void){
    scoreInfo* scoreHistoryCopy = SDL_malloc(sizeof(scoreInfo) * (lengthOfScoreHistory + 1));

    SDL_memcpy(scoreHistoryCopy, scoreHistory, sizeof(scoreInfo) * (lengthOfScoreHistory));
    //first: remove the duplicates
    //sort by checksum
    SDL_qsort(scoreHistoryCopy, lengthOfScoreHistory, sizeof(scoreHistoryCopy[0]), checksumSortingCompare);
    //do in place removal
    int newLength = 0;
    char* oldChecksum = "not a checksum";
    for(int i = 0; i < lengthOfScoreHistory - 1; i++){
        const bool isSameAsOldChecksum = SDL_strcmp(oldChecksum, scoreHistoryCopy[i].checksum) == 0;

        if((newLength <= 0 || !isSameAsOldChecksum) && SDL_strcmp(scoreHistoryCopy[i].checksum, scoreHistoryCopy[i + 1].checksum) == 0){
            oldChecksum = scoreHistoryCopy[i].checksum;
            int bestI = i;
            double bestRating = -999;
            for(int n = i; n < lengthOfScoreHistory && SDL_strcmp(oldChecksum, scoreHistoryCopy[n].checksum) == 0; n++){
                const double newRating = getRatingPoints(&scoreHistoryCopy[n], getAchievement(&scoreHistoryCopy[n]));
                if(newRating > bestRating){
                    bestRating = newRating;
                    bestI = n;
                }
            }

            scoreHistoryCopy[newLength] = scoreHistoryCopy[bestI];
            newLength++;
        } else if(!isSameAsOldChecksum){
            oldChecksum = scoreHistoryCopy[i].checksum;
            scoreHistoryCopy[newLength] = scoreHistoryCopy[i];
            newLength++;
        }
    }

    assert(lengthOfScoreHistory == 0 || lengthOfScoreHistory == 1 || newLength > 0);

    if(lengthOfScoreHistory == 1){
        newLength = 1; //works because in place
    } else if(lengthOfScoreHistory > 1 && SDL_strcmp(scoreHistoryCopy[lengthOfScoreHistory - 1].checksum, scoreHistoryCopy[newLength - 1].checksum) != 0){ //fix issue where new song doesn't get new rating immediately
        scoreHistoryCopy[newLength] = scoreHistoryCopy[lengthOfScoreHistory - 1];
        newLength++;
    }

    SDL_qsort(scoreHistoryCopy, newLength, sizeof(scoreHistoryCopy[0]), ratingSortingCompare);
    double ratingInfo = 0;
    for(int i = 0; i < SDL_min(newLength, 50); i++){
        const double points = getRatingPoints(&scoreHistoryCopy[i], getAchievement(&scoreHistoryCopy[i]));
        //printf("%d/%d %.2f %d %s\n", i, SDL_min(newLength, 50), points, scoreHistoryCopy[i].chartIndex, scoreHistoryCopy[i].checksum);
        ratingInfo += points;
    }

    SDL_free(scoreHistoryCopy);
    return ratingInfo / 50.0;
}

double getBestScoreFromChecksumAndDifficulty(char* checksum, int difficulty){
    double bestScore = 0.0;
    for(int i = 0; i < lengthOfScoreHistory; i++){
        if(scoreHistory[i].chartIndex == difficulty && SDL_strcmp(checksum, scoreHistory[i].checksum) == 0){
            bestScore = clampDouble(getAchievement(&scoreHistory[i]), bestScore, 101.0);
        }
    }
    return bestScore;
}

void updateScoreFile(void){
    //realloc time
    //printf("before\n");
    //printf("rating before: %.2f\n", getRating());
    scoreInfo* newPointer = (scoreInfo*)reallocfTypeSDL((scoreHistory), sizeof(scoreInfo) * (lengthOfScoreHistory + 1));
    scoreHistory = newPointer;

    //add new score to history
    scoreHistory[lengthOfScoreHistory].checksum = SDL_strdup(songs[currentSongIndex].checksumOfFile);
    scoreHistory[lengthOfScoreHistory].chartIndex = selectedDifficulty;
    const char* currentDiff = songs[currentSongIndex].difficultyLevels[selectedDifficulty];
    scoreHistory[lengthOfScoreHistory].level = SDL_strdup(currentDiff == NULL ? "" : currentDiff);
    scoreHistory[lengthOfScoreHistory].score[0] = currentBaseScore;
    scoreHistory[lengthOfScoreHistory].score[1] = absoluteMaxBaseScore;
    scoreHistory[lengthOfScoreHistory].score[2] = currentBreakBonus;
    scoreHistory[lengthOfScoreHistory].score[3] = absoluteMaxBreakBonus;
    for(int i = 0; i < 8; i++){
        scoreHistory[lengthOfScoreHistory].judgementCounts[i] = judgementCounts[i];
    }
    lengthOfScoreHistory++;
    const size_t lenOfNewString = 512 * (lengthOfScoreHistory + 2);
    char* newScoreFile = SDL_calloc(lenOfNewString, sizeof(char));
    SDL_strlcpy(newScoreFile, "[scores]\n; format: sha256ofmaidata.txt=chartIndex/level/miss/good/low great/mid great/high great/low perfect/high perfect/critical perfect/base score/max base score/break bonus/max break bonus/\n", lenOfNewString);
    char info[256] = {0};
    for(int i = 0; i < lengthOfScoreHistory; i++){
        simple_snprintf(info, "%s=%d/%s/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/%u/\n",
                        scoreHistory[i].checksum,
                        scoreHistory[i].chartIndex, scoreHistory[i].level,
                        scoreHistory[i].judgementCounts[judgeMiss],
                        scoreHistory[i].judgementCounts[judgeGood],
                        scoreHistory[i].judgementCounts[judgeGreatLow],
                        scoreHistory[i].judgementCounts[judgeGreatMid],
                        scoreHistory[i].judgementCounts[judgeGreatHigh],
                        scoreHistory[i].judgementCounts[judgePerfectLow],
                        scoreHistory[i].judgementCounts[judgePerfectHigh],
                        scoreHistory[i].judgementCounts[judgePerfectCritical],
                        scoreHistory[i].score[0], scoreHistory[i].score[1],
                        scoreHistory[i].score[2], scoreHistory[i].score[3]
                        );
        SDL_strlcat(newScoreFile, info, lenOfNewString);
    }

    simple_snprintf(info, "profiles/%s/scores.ini", profiles[currentProfileIndex].profileLocation);

    SDL_SaveFile(info, newScoreFile, SDL_strnlen(newScoreFile, lenOfNewString));

    if(enableRating){
        //printf("after\n");
        const double ratingInfo = getRating();
        //printf("rating after: %.2f\n", ratingInfo);
        profiles[currentProfileIndex].rating = ratingInfo;
    }
}

static int scoreHandler(void* user, const char* section, const char* name, const char* value){
    UNUSED(user);
    if(SDL_strcmp(section, "scores") == 0){
        if(lengthOfScoreHistory % 32 == 0){
            scoreInfo* newPointer = (scoreInfo*)reallocfTypeSDL((scoreHistory), sizeof(scoreInfo) * (lengthOfScoreHistory + 32));
            scoreHistory = newPointer;
        }

        //todo: use base64?
        /*if(SDL_strnlen(name, 65) == 64){
            unsigned char byteArray[32];
            for(int i = 0; i < 32; i++){
                const unsigned char value1 = (name[i * 2] >= 'a') ? (name[i * 2] - 'a' + 10) : (name[i * 2] - '0');
                const unsigned char value2 = (name[i * 2 + 1] >= 'a') ? (name[i * 2 + 1] - 'a' + 10) : (name[i * 2 + 1] - '0');
                byteArray[i] = value1 * 16 + value2;
            }
            scoreHistory[lengthOfScoreHistory].checksum = base64URLEncoding(byteArray, 32);
        } else {
            scoreHistory[lengthOfScoreHistory].checksum = SDL_strdup(name);
        }*/

        scoreHistory[lengthOfScoreHistory].checksum = SDL_strdup(name);

        int index = 0;
        unsigned int startI = 0;
        const unsigned int len = SDL_strlen(value);
        for(unsigned int i = 0; i < len; i++){
            if(value[i] == '/'){
                switch(index){
                    case 0:
                        scoreHistory[lengthOfScoreHistory].chartIndex = SDL_atoi(value + startI);
                        break;
                    case 1:
                        //more bugs
                        scoreHistory[lengthOfScoreHistory].level = SDL_calloc(16, sizeof(char));
                        SDL_strlcpy(scoreHistory[lengthOfScoreHistory].level, value + startI, SDL_min(8, i - startI + 1));
                        break;
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                        scoreHistory[lengthOfScoreHistory].score[index - 10] = SDL_atoi(value + startI);
                        break;
                    default:
                        scoreHistory[lengthOfScoreHistory].judgementCounts[index - 2] = SDL_atoi(value + startI);

                }

                startI = i + 1;
                index++;
            }
        }

        lengthOfScoreHistory++;
        return 0;
    }
    return 1;
}

void loadScoreFile(void){
    for(int i = 0; i < lengthOfScoreHistory; i++){
        SDL_free(scoreHistory[i].checksum);
        SDL_free(scoreHistory[i].level);
    }
    lengthOfScoreHistory = 0;
    char* info = joinPrefixMiddleEndCharArrays("profiles/", profiles[currentProfileIndex].profileLocation, "/scores.ini");
    if(ini_parse(info, scoreHandler, NULL) < 0){
        printf("BAD INI\n");
    }
    SDL_free(info);
}

double calculateScore(unsigned int baseScore, unsigned int maxBaseScore, unsigned int breakBonus, unsigned int maxBreakBonus){
    const double baseResult  = maxBaseScore  == 0 ? 100.0 : (baseScore * 100.0 / maxBaseScore);
    const double bonusResult = maxBreakBonus == 0 ?   0.0 : ((double)breakBonus / (double)maxBreakBonus);
    const double totalResult = SDL_trunc((baseResult + bonusResult) * 10000.0) / 10000.0;
    if(totalResult >= 100.5){
        rankIndex = 0;
    } else if(totalResult >= 100){
        rankIndex = 1;
    } else if(totalResult >= 99.5){
        rankIndex = 2;
    } else if(totalResult >= 99){
        rankIndex = 3;
    } else if(totalResult >= 98){
        rankIndex = 4;
    } else if(totalResult >= 97){
        rankIndex = 5;
    } else if(totalResult >= 94){
        rankIndex = 6;
    } else if(totalResult >= 90){
        rankIndex = 7;
    } else if(totalResult >= 80){
        rankIndex = 8;
    } else if(totalResult >= 75){
        rankIndex = 9;
    } else if(totalResult >= 70){
        rankIndex = 10;
    } else if(totalResult >= 60){
        rankIndex = 11;
    } else if(totalResult >= 50){
        rankIndex = 12;
    } else {
        rankIndex = 13;
    }
    return totalResult;
}

SDL_Point getMaxDXStarsAndHP(unsigned int baseDXScore, unsigned int maxDXScore, unsigned int maxStars){
    const double maxDXScoreFloat = (double)maxDXScore;
    const unsigned int starValues[6] = {
        SDL_ceil(maxDXScoreFloat * 0.85), //1 star
        SDL_ceil(maxDXScoreFloat * 0.90), //2 stars
        SDL_ceil(maxDXScoreFloat * 0.93), //3 stars
        SDL_ceil(maxDXScoreFloat * 0.95), //4 stars
        SDL_ceil(maxDXScoreFloat * 0.97), //5 stars
        SDL_ceil(maxDXScoreFloat * 0.99)  //6 stars (unofficial)
    };

    SDL_Point DXStarsHP = {-1, baseDXScore};

    for(int i = stackArraySizeInt(starValues) + maxStars - 7; i >= 0; i--){
        if(baseDXScore >= starValues[i]){
            DXStarsHP.x = i;
            DXStarsHP.y = baseDXScore - starValues[i];
            break;
        }
    }
    return DXStarsHP;
}

void calculateDXStars(char* dest, size_t maxLen, unsigned int baseDXScore, unsigned int maxDXScore){
    const SDL_Point DXStars = getMaxDXStarsAndHP(baseDXScore, maxDXScore, 6);
    if(DXStars.x >= 0){
        SDL_strlcat(dest, ", ", maxLen);

        SDL_strlcat(dest, langGetStr(DXStars.x == 0 ? "1dxstar" :
                                     DXStars.x == 1 ? "2dxstars" :
                                     DXStars.x == 2 ? "3dxstars" :
                                     DXStars.x == 3 ? "4dxstars" :
                                     DXStars.x == 4 ? "5dxstars" :
                                     DXStars.x == 5 ? "6dxstars" : "invalidstars"), maxLen);
    }

    SDL_strlcat(dest, ")", maxLen);
}

unsigned int currentBaseScore = 0;
unsigned int currentMaxBaseScore = 0;
unsigned int absoluteMaxBaseScore = 0;

unsigned int currentBreakBonus = 0;
unsigned int currentMaxBreakBonus = 0;
unsigned int absoluteMaxBreakBonus = 0;

unsigned int currentDXScore = 0;
unsigned int currentMaxDXScore = 0;
unsigned int absoluteMaxDXScore = 0;

int getFCMedal(void){
    if(currentDXScore >= absoluteMaxDXScore){ //max dx score (critical perfect is +3, all other judgements give less)
        return medalTypeACP;
    } else if(currentBaseScore >= absoluteMaxBaseScore && currentBreakBonus >= absoluteMaxBreakBonus && absoluteMaxBreakBonus > 0){ //101%
        return medalTypeAPPlus;
    } else if(judgementCounts[judgeMiss] == 0){
        if(judgementCounts[judgeGood] == 0){
            if(judgementCounts[judgeGreatLow] == 0 && judgementCounts[judgeGreatMid] == 0 && judgementCounts[judgeGreatHigh] == 0){
                return medalTypeAP;
            } else {
                return medalTypeFCPlus;
            }
        } else {
            return medalTypeFC;
        }
    }

    return medalTypeNone;
}

static int renderScoreScreen(void){
    const SDL_FRect songBGFRect = xywhScaled(getGuiCenterX(), 247.f, 200.f, 200.f);
    if(scoreScreenSongBG != NULL){
        renderTextureSimpleManual(scoreScreenSongBG, &songBGFRect, true);
    }
    renderTextureSimpler(scoreScreenScore, getCenterX(), 378.f * guiScale);
    renderTextureSimpler(scoreScreenRank, getCenterX(), 435.f * guiScale);

    if(scoreScreenFCBadge != NULL){
        renderTextureSimpler(scoreScreenFCBadge, getCenterX(), 485.f * guiScale);
    }

    renderTextureSimpler(scoreScreenDXScore, getCenterX(), 520.f * guiScale);

    if(addedRatingTexture != NULL){
        renderTextureSimpler(addedRatingTexture, getCenterX(), 560.f * guiScale);
    }

    const SDL_Color nextColor = buttonBoxColors[nextButtonBoxI - 1];
    renderButtonTextBox(nextButtonBoxI, buttonBoxTexts[nextButtonBoxI - 1], nextColor.r, nextColor.g, nextColor.b);

    renderDifficultyInfo(0.f, 90.f);
    return 0;
}

Uint64 scoreScreen(Uint64 delta){
    renderFunction = renderScoreScreen;
    subscreenRenderFunction = renderInGameInfoOnSubscreen;
    static Uint64 fadeInScoreScreen = 0;
    fadeInScoreScreen += delta;
    globalBrightness = convertTimerAndDurationToBrightness(fadeInScoreScreen, (Uint64)gameTicksPerSecond);
    if(!isButtonOrTouchPressed(nextButtonBoxI) || fadeInScoreScreen < (Uint64)gameTicksPerSecond){
        return 0;
    }

    currentBaseScore = 0;
    currentMaxBaseScore = 0;
    currentBreakBonus = 0;
    currentMaxBreakBonus = 0;
    currentDXScore = 0;
    currentMaxDXScore = 0;
    comboCount = 0;
    fadeInScoreScreen = 0;
    globalBrightness = 255;
    for(unsigned int i = 0; i < stackArraySize(judgementCounts); i++){
        judgementCounts[i] = 0;
    }

    #ifdef Circular8_ENABLE_FFmpeg
    if(isShowingVideo){
        closeVideo();
    }
    #endif // Circular8_ENABLE_FFmpeg
    //free some memory and also fix issues like song background not updating
    destroyTextureIfNotNull(scoreScreenSongBG);
    destroyTextureIfNotNull(scoreScreenRank);
    destroyTextureIfNotNull(scoreScreenScore);
    destroyTextureIfNotNull(scoreScreenDXScore);
    destroyTextureIfNotNull(scoreScreenFCBadge);
    destroyTextureIfNotNull(addedRatingTexture);

    resetButtons(); //prevents double click
    inScoreScreen = false;
    runLoadFunction(loadSongSelect, songSelectSlot);
    return 0;
}
