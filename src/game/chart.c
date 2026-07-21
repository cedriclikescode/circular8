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
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "main.h"
#include "load.h"
#include "render.h"
#include "miscutils.h"

#include "game/ingame.h"
#include "game/chart.h"
#include "game/songselect.h"
#include "game/scorescreen.h"
#include "game/rendersong.h"
#include "game/options.h"

#include "thirdparty/SHA-256/sha256.h"

//magic numbers that doesn't even make sense

bool chartNoteFeatures[32] = {0};

const double borderRadius = 640.0 / 2.0;
const double travelDistance = borderRadius - 77.5; //242.0; //239.5; //from start to hit for tap and hold note
//more bugs
static const double borderRadiusPercent = borderRadius / 360.0; //for slides
static const float borderRadiusPercentReciprocal = 1.0 / borderRadiusPercent;
static const float borderRadius50Percent = borderRadius / 720.0; //for slides

int chartLength = 0;
SDL_FPoint touchLocations[8 * 4 + 2] = {0}; //34 possible sensors
struct noteObject* chartNotes = NULL;

static const float normalizedLineDist = 0.05f / borderRadiusPercentReciprocal;
static float lineDistanceChangeCalculator(const SDL_FPoint* startFPoint, const SDL_FPoint* endFPoint) {
    //magic numbers
    return 0.05f / (distanceBetweenTwoFPoints(startFPoint, endFPoint) * borderRadiusPercentReciprocal);
}

static int applyMirrorToSensorDE(int button){
    const bool flipX = (mirrorType == 1 || mirrorType == 3);
    const bool flipY = (mirrorType == 2 || mirrorType == 3);
    const int flipXMap[8] = {1, 8, 7, 6, 5, 4, 3, 2};
    const int flipYMap[8] = {5, 4, 3, 2, 1, 8, 7, 6};

    if(flipX){
        button = flipXMap[button - 1];
    }

    if(flipY){
        button = flipYMap[button - 1];
    }
    return button;
}

static int applyMirrorToButton(int button){
    const int flipYMap[8] = {4, 3, 2, 1, 8, 7, 6, 5};
    switch(mirrorType){
        case 0:
            return button; //no mirror
        case 1:
            return 8 - (button - 1); //flip x
        case 2:
            return flipYMap[button - 1]; //flip y
        case 3:
            return flipYMap[8 - (button - 1) - 1]; //rotate 180
        default:
            return button;
    }
    return button;
}

static float angleAwayFromTwoFPoints(SDL_FPoint startLocation, SDL_FPoint endLocation, float angle){
    const float newAngle = startFPPointsTowardEndFP(&startLocation, &endLocation);
    if(angle < 0.f){
        angle += 360.f;
    } else {
        angle = SDL_fmodf(angle, 360.f);
    }
    return circularFloatDistance(angle, newAngle, 360.f);
}

static float isGoingTowardsButton(SDL_FPoint startLocation, float angle, int button){
    const SDL_FPoint buttonFPoint = moveNoteXY(0.5f, 0.5f, button, borderRadius50Percent);
    return angleAwayFromTwoFPoints(startLocation, buttonFPoint, angle);
}

static int shortestSlideDirection(int startPos, int endPos) {
    int result = endPos - startPos;
    if(SDL_abs(result) == 4){
        return 0;
    }

    while(result > 4){
        result -= 8;
    }

    while(result < -4){
        result += 8;
    }

    if(result < 0){
        return -1;
    }
    return 1;
}

static int convertSlideType(int slideType, int startLocation, int endLocation){
    //line is safe
    if(slideType == slideTypeAutoArc || slideType == slideTypeArcLeft || slideType == slideTypeArcRight){
        bool clockwise = false;
        if(slideType == slideTypeAutoArc){
            clockwise = shortestSlideDirection(startLocation, endLocation) == 1;
        } else {
            //welcome to absolute nonsense that shouldn't even exist
            const bool arcIsLeft = slideType == slideTypeArcLeft;
            if (startLocation >= 3 && startLocation <= 6) {
                clockwise = arcIsLeft;
            } else {
                clockwise = !arcIsLeft;
            }
        }
        return clockwise ? lastZoneArcCW : lastZoneArcCCW;
    }

    switch(slideType){
        case slideTypeStraightLine:
            return lastZoneLine;
        case slideTypeVShape:
            return lastZonev;
        case slideTypeSShape:
        case slideTypeZShape:
            return lastZoneZigZag;
        case slideTypePShape:
            return lastZoneP;
        case slideTypeQShape:
            return lastZoneQ;
        case slideTypePPShape:
            return lastZonePP;
        case slideTypeQQShape:
            return lastZoneQQ;
        case slideTypeReflectiveShapeCW: //V
            //panic
            return lastZoneReflectiveCW;
        case slideTypeReflectiveShapeCCW: //V
            //what
            return lastZoneReflectiveCCW;
        default:
            return -700;
    }
    return slideType;
}

//math formula/numbers taken from https://www.reddit.com/r/maimai/comments/y0bile/longimg_but_before_i_drew_the_sensors_i_desmosd/
//original source: https://www.desmos.com/calculator/nt8zighroh

static const double sqrtOf2 = 1.414213562373095048801688724209698078569671875;

SDL_FPoint ASensorPoints[9]; //uses non constant expressions, loaded in generateTouchSensors

const SDL_FPoint BSensorPoints[7] = {
    {0.29,                      0.434 + (0.01  * sqrtOf2)},
    {0.29,                      0.29  + (0.01  * sqrtOf2)},
    {0.29 - (0.036 * sqrtOf2),  0.29  - (0.026 * sqrtOf2)},
    {(0.316 * sqrtOf2) - 0.342, 0.29  - (0.026 * sqrtOf2)},
    {0.01,                      (0.29 * sqrtOf2) - 0.062},
    {0.01,                      (0.29 * sqrtOf2) + 0.01},
    {(0.072 * sqrtOf2) + 0.01,  (0.362 * sqrtOf2) + 0.01}
};

SDL_FPoint CSensorPoints[6]; //uses non constant expressions, loaded in generateTouchSensors
SDL_FPoint DSensorPoints[6]; //uses non constant expressions, loaded in generateTouchSensors

const SDL_FPoint ESensorPoints[4] = {
    { 0.0,             0.31  * sqrtOf2},
    { 0.096 * sqrtOf2, 0.406 * sqrtOf2},
    { 0.0,             0.502 * sqrtOf2},
    {-0.096 * sqrtOf2, 0.406 * sqrtOf2}
};

// 3 sensors
// 2 a (9)
// 2 b (7)
// 1 c (6, can delete 5th point due to mirroring)
static SDL_FPoint optimizedSlideSensorPoints[(9 * 2) + (7 * 2) + 6 - 1] = {0};
static SDL_FPoint rotatedSensorPoints[5][8][9] = {0};

static SDL_FPoint mathFunctionO(double r, double d, double w){
    const double sqrtPart = SDL_sqrt(squareDouble(r) - squareDouble(0.01));
    const double hundredthOfW = 0.01 * w;
    return (SDL_FPoint){sqrtPart * cosDeg(d) + hundredthOfW * (-sinDeg(d)),
                        sqrtPart * sinDeg(d) + hundredthOfW * cosDeg(d)};
}

static const int sensorsLength[5] = {9, 7, 6, 6, 4};

static SDL_FPoint getSensorFPoint(int sensor, int index){
    switch(sensor){
        case 0:
            return ASensorPoints[index];
        case 1:
            return BSensorPoints[index];
        case 2:
            return CSensorPoints[index];
        case 3:
            return DSensorPoints[index];
        case 4:
            return ESensorPoints[index];
        default:
            return (SDL_FPoint){-10.5, -11.5};
    }
    return (SDL_FPoint){-10.5, -11.5};
}

void generateTouchSensors(double dist) {
    const double o = 67.5 - ((atanDeg(0.522 / ((16.0 / 9.0) * cosDeg(22.5) - 0.522 * cotDeg(22.5)))) -
                             asinDeg(0.01 / SDL_sqrt(squareDouble((16.0 / 9.0) * cosDeg(22.5) - 0.522 * cotDeg(22.5)) + squareDouble(0.522))));

    const double iVar = 8.5; //used in the desmos drawing apparently
    ASensorPoints[0].x = cosDeg(67.5 - iVar);
    ASensorPoints[0].y = sinDeg(67.5 - iVar);

    const SDL_FPoint O67dot5Minus = mathFunctionO(1, 67.5 - o, 1);

    ASensorPoints[1].x = O67dot5Minus.x;
    ASensorPoints[1].y = O67dot5Minus.y;

    ASensorPoints[2].x = (16.0 / 9.0) * cosDeg(22.5) - 0.522 * cotDeg(22.5);
    ASensorPoints[2].y = 0.522;

    ASensorPoints[3].x = 0.292;
    ASensorPoints[3].y = 0.522;

    ASensorPoints[4].x = 0.115 * sqrtOf2;
    ASensorPoints[4].y = 0.407 * sqrtOf2;

    const double resultOfMath1 = (8 * sqrtOf2 * cosDeg(22.5)) / 9.0;

    ASensorPoints[5].x = 0.522 * cotDeg(22.5) - resultOfMath1;
    ASensorPoints[5].y = resultOfMath1 - 0.522;

    const SDL_FPoint O67dot5Minus1R = mathFunctionO(1, 67.5 + o, -1);

    ASensorPoints[6].x = O67dot5Minus1R.x;
    ASensorPoints[6].y = O67dot5Minus1R.y;

    ASensorPoints[7].x = cosDeg(67.5 + iVar);
    ASensorPoints[7].y = sinDeg(67.5 + iVar);

    ASensorPoints[8].x = cosDeg(67.5);
    ASensorPoints[8].y = sinDeg(67.5);

    CSensorPoints[0].x = 0.01;
    CSensorPoints[0].y = 0.192;

    CSensorPoints[1].x = 0.192 * cotDeg(67.5);
    CSensorPoints[1].y = 0.192;

    CSensorPoints[2].x = 0.192;
    CSensorPoints[2].y = 0.192 * tanDeg(22.5);

    CSensorPoints[3].x = 0.192;
    CSensorPoints[3].y = 0.192 * tanDeg(-22.5);

    CSensorPoints[4].x = 0.192 * cotDeg(67.5);
    CSensorPoints[4].y = -0.192;

    CSensorPoints[5].x = 0.01;
    CSensorPoints[5].y = -0.192;

    const SDL_FPoint O67dot5 = mathFunctionO(1, 67.5 + o, 1);

    DSensorPoints[0].x = O67dot5.x;
    DSensorPoints[0].y = O67dot5.y;

    const double resultOfMathX = (0.522 * sqrtOf2 - 0.01 * SDL_sqrt(1 + tanSquaredDeg(67.5 + o))) /
                                 (1 + tanDeg(67.5 + o));

    const double resultOfMathY = (0.522 * sqrtOf2 * tanDeg(67.5 + o) + 0.01 * SDL_sqrt(1 + tanSquaredDeg(67.5 + o))) /
                                 (1 + tanDeg(67.5 + o));

    DSensorPoints[1].x = resultOfMathX;
    DSensorPoints[1].y = resultOfMathY;

    DSensorPoints[2].x = 0;
    DSensorPoints[2].y = 0.522 * sqrtOf2;

    DSensorPoints[3].x = -resultOfMathX;
    DSensorPoints[3].y = resultOfMathY;

    const SDL_FPoint O112dot5 = mathFunctionO(1, 112.5 - o, -1);

    DSensorPoints[4].x = O112dot5.x;
    DSensorPoints[4].y = O112dot5.y;

    DSensorPoints[5].x = 0;
    DSensorPoints[5].y = 1;

    const SDL_FPoint invalidFPoint = {-10000, -10000};
    for(int i = 0; i < 8 * 4 + 2; i++) {
        touchLocations[i] = invalidFPoint;
    }

    //note to self: moveNoteXY is 1 indexed
    //letter A
    const float centerX = 0.5f;
    const float centerY = 0.5f;
    for(int i = 0; i < 8; i++) {
        //closer is required
        touchLocations[i] = moveNoteXY(centerX, centerY, i + 1, dist / 1.1f / 720.0f);
    }

    //letter B
    for(int i = 0; i < 8; i++) {
        touchLocations[i + 8] = moveNoteXY(centerX, centerY, i + 1, dist / 2.05f / 720.0f);
    }

    //letter C
    touchLocations[0 + 8 * 2].x = centerX;
    touchLocations[0 + 8 * 2].y = centerY;

    //letter D
    for(int i = 0; i < 8; i++) {
        //closer is also required
        touchLocations[i + 8 * 2 + 2] = moveXYWithDirAndDistFloat(centerX, centerY, 45.f * i, dist / 1.1f / 720.0f);
    }

    //letter E
    for(int i = 0; i < 8; i++) {
        touchLocations[i + 8 * 3 + 2] = moveXYWithDirAndDistFloat(centerX, centerY, 45.f * i, dist / 1.44f / 720.0f);
    }

    //random bug
    for(int i = 0; i < 5; i++){
        const float dirChange = i == 2 ? 180.f : 45.f; //C sensor moment
        const int maxN = i == 2 ? 2 : 8;
        for(int n = 0; n < maxN; n++){
            for(int point = 0; point < sensorsLength[i]; point++){
                //garbage that has no answers
                SDL_FPoint currentXY = rotateFPointBy(dirChange * -n, getSensorFPoint(i, point));
                currentXY.y *= -1;
                rotatedSensorPoints[i][n][point] = currentXY;
            }
        }
    }

    //real
    int total = 0;

    //sensor a
    for(int n = 0; n < 2; n++){
        for(int i = 0; i < 9; i++){
            optimizedSlideSensorPoints[total] = rotatedSensorPoints[0][n][i];
            total++;
        }
    }

    //sensor b
    for(int n = 0; n < 2; n++){
        for(int i = 0; i < 7; i++){
            optimizedSlideSensorPoints[total] = rotatedSensorPoints[1][n][i];
            total++;
        }
    }

    //censor c
    for(int i = 0; i < 6; i++){
        if(i == 4){ //4th point not needed due to mirroring
            break;
        }
        optimizedSlideSensorPoints[total] = rotatedSensorPoints[2][0][i];
        total++;
    }
}

bool getInfoAboutMaidatatxt(songFile* chart) { //todo: apparently maidata.txt is encoded like html form??
    char* location = joinTwoCharArraysToOne(chart->chartLocation, "/maidata.txt");

    SDL_IOStream* chartRWops = SDL_IOFromFile(location, "rb");
    SDL_free(location);
    if_unlikely(chartRWops == NULL) {
        setError("Chart failed to load.\nError: %s", SDL_GetError());
        return false;
    }

    const size_t sizeOfFile = (size_t)SDL_GetIOSize(chartRWops);
    char* chartFile = SDL_calloc(sizeOfFile + 16, 1); //i love magic

    SDL_ReadIO(chartRWops, chartFile, sizeOfFile);
    if_unlikely(!SDL_CloseIO(chartRWops)){
        setError("Closing chart file handler failed.\nError: %s", SDL_GetError());
        return false;
    }

    //const int length = strlen(chartFile);
    //hash moment
    //chart->checksumOfFile = SDL_calloc(SHA256_HEX_SIZE, sizeof(char));

    //sha256_hex(chartFile, sizeOfFile, chart->checksumOfFile);
    //todo: use base64?
    /*char sha256ByteArray[32] = {0};
    sha256_bytes(chartFile, sizeOfFile, sha256ByteArray);
    chart->checksumOfFile = base64URLEncoding(sha256ByteArray, 32);*/

    chart->checksumOfFile = SDL_calloc(SHA256_HEX_SIZE, sizeof(char));
    sha256_hex(chartFile, sizeOfFile, chart->checksumOfFile);

    SDL_strlcat(chartFile, "\n\n\n\n\n\n\n\n", sizeOfFile + 16); //works around partially broken files
    const int length = sizeOfFile + 8;


    //get level number, etc
    int stage = 0;
    bool isInComment = false;
    int varStartIndex = 0;
    int varLength = 0;
    int valueStart = 0;
    unsigned int valueLength = 0;
    bool isGettingVar = true;
    bool isInINote = false;
    int firstI = -1;
    int levelI = -1;

    int difficultyNameI = -1;
    int colorI = -1;

    //initalize stuff
    const SDL_Color defaultDifficultyColors[7] = {
        {  0, 136, 255, 255},
        {  0, 204,   0, 255},
        {229, 230,   0, 255},
        {255,   0,   0, 255},
        {128,   0, 255, 255},
        {191, 128, 255, 255},
        {255,  85, 255, 255}
    };

    chart->songTitle = NULL;
    bool individualOffsetSet[7] = {0};
    Sint64 globalOffset = 0;
    for(int i = 0; i < 7; i++){
        chart->individualOffset[i] = 0;
        chart->difficultyLevels[i] = NULL;
        chart->difficultyStartIndexes[i] = 0;
        chart->difficultyNames[i] = NULL;
        chart->difficultyColors[i] = defaultDifficultyColors[i];
    }

    for(int n = 0; n < length; n++) {
        switch(stage) {
            case 0: //scanning mode
                if(chartFile[n] == '&') { //find ampersand
                    stage = 1;
                    varStartIndex = n + 1;
                    varLength = 0;
                    isGettingVar = true; //another rare bug
                } else if(chartFile[n] == '|') { //find comment
                    stage = 2;
                }
                break;
            case 1: //ampersand
#define isEqualToVarString(s) (SDL_strncmp(chartFile + varStartIndex, s, (size_t)varLength) == 0)
                if(isGettingVar) {
                    if(chartFile[n] != '=') {
                        varLength++;
                        break;
                    }

                    isGettingVar = false;
                    valueStart = varStartIndex + varLength + 1;
                    valueLength = 0;

                    if(isEqualToVarString("first")){
                        firstI = 0;
                        break;
                    }

                    for(int i = 1; i <= 7; i++) {
                        char ampersandStr[32] = {0};

                        simple_snprintf(ampersandStr, "inote_%d", i);
                        if(isEqualToVarString(ampersandStr)) {
                            chart->difficultyStartIndexes[i - 1] = n + 1;
                            isInINote = true;
                            stage = 3;
                            break;
                        }

                        simple_snprintf(ampersandStr, "first_%d", i);
                        if(isEqualToVarString(ampersandStr)){
                            firstI = i;
                            break;
                        }

                        simple_snprintf(ampersandStr, "lv_%d", i);
                        if(isEqualToVarString(ampersandStr)){
                            levelI = i;
                            break;
                        }

                        simple_snprintf(ampersandStr, "alias_%d", i);
                        if(isEqualToVarString(ampersandStr)){
                            difficultyNameI = i;
                            break;
                        }

                        simple_snprintf(ampersandStr, "color_%d", i);
                        if(isEqualToVarString(ampersandStr)){
                            colorI = i;
                            break;
                        }
                    }
                    break;
                }

                static unsigned int realLen = 0;
                if(chartFile[n] != '\n') {
                    valueLength++;
                    realLen = valueLength;
                    break;
                }

                valueLength++;
                if(n > 0 && chartFile[n - 1] == '\r' && realLen > 0){ //random check because windows
                    realLen--;
                }

                //safer
                #define callocStringHelper(nameOfVar) nameOfVar = SDL_calloc(valueLength + 4, 1); \
                                                      SDL_strlcpy(nameOfVar, chartFile + valueStart, realLen + 1); \
                                                      realLen = 0;
                if(isEqualToVarString("title")) { //title of song
                    callocStringHelper(chart->songTitle);
                } else if(firstI >= 0 && firstI <= 7){
                    char* firstIStr = NULL;
                    callocStringHelper(firstIStr);

                    if(firstI == 0){
                        globalOffset = SDL_round(SDL_atof(firstIStr) * gameTicksPerSecond);
                    } else {
                        chart->individualOffset[firstI - 1] = SDL_round(SDL_atof(firstIStr) * gameTicksPerSecond);
                        individualOffsetSet[firstI - 1] = true;
                    }
                    SDL_free(firstIStr);
                    firstI = -1;
                } else if(levelI >= 1 && levelI <= 7){
                    char* levelIStr = NULL;
                    callocStringHelper(levelIStr);

                    chart->difficultyLevels[levelI - 1] = levelIStr;
                    levelI = -1;
                } else if(difficultyNameI >= 1 && difficultyNameI <= 7){
                    char* difficultyNameIStr = NULL;
                    callocStringHelper(difficultyNameIStr);

                    chart->difficultyNames[difficultyNameI - 1] = difficultyNameIStr;
                    difficultyNameI = -1;
                } else if(colorI >= 1 && colorI <= 7){
                    char* colorIStr = NULL;
                    callocStringHelper(colorIStr);
                    if(isValidHex(colorIStr)){
                        chart->difficultyColors[colorI] = hexCodeToColor(colorIStr);
                    }
                    SDL_free(colorIStr);
                    colorI = -1;
                }
                #undef callocStringHelper
                stage = 0;
                realLen = 0;
#undef isEqualToVarString
                break;
            case 2: //comment mode
                if(isInComment) {
                    if(chartFile[n] == '\n') {
                        isInComment = false;
                        stage = isInINote ? 3 : 0;
                    }
                } else if(chartFile[n] == '|') {
                    isInComment = true;
                } else {
                    return setError("invalid comment at char %d\n", n);
                }
                break;
            case 3: //inside a inote
                if(chartFile[n] == '|') { //find comment
                    stage = 2;
                } else if(chartFile[n] == 'E' && n + 1 < length && !SDL_isdigit(chartFile[n + 1])) { //random errors here
                    isInINote = false;
                    stage = 0;
                } else if(chartFile[n] == '&'){ //workaround broken charts
                    isInINote = false;
                    stage = 0;
                    n--;
                }
                break;
            default:
                setError("Invalid stage (%d) in\ngetInfoAboutMaidatatxt", stage);
        }
    }
    SDL_free(chartFile);

    //post processing
    for(int i = 0; i < 7; i++){
        if(!individualOffsetSet[i]){
            chart->individualOffset[i] = globalOffset;
        }
    }
    return true;
}

static chartErrorRange errorAtWithLength(int location, int length){
    return (chartErrorRange){location, length};
}


static chartErrorRange errorAt(int location){
    return errorAtWithLength(location, 1);
}

static void spreadTouchGroups(int noteI){
    const int lenOfConnectedNotes = chartNotes[noteI].info.touch.lenOfConnectedNotes;
    for(int i = 0; i < lenOfConnectedNotes; i++){
        const int selectedNote = chartNotes[noteI].info.touch.connectedNotes[i];
        const size_t memSize = sizeof(int) * lenOfConnectedNotes;
        chartNotes[selectedNote].info.touch.connectedNotes = (int*)SDL_malloc(memSize);
        chartNotes[selectedNote].info.touch.lenOfConnectedNotes = lenOfConnectedNotes;
        SDL_memcpy(chartNotes[selectedNote].info.touch.connectedNotes,
                   chartNotes[noteI].info.touch.connectedNotes, memSize);
        //correct one value
        for(int n = 0; n < lenOfConnectedNotes; n++){
            if(chartNotes[selectedNote].info.touch.connectedNotes[n] == selectedNote){
                chartNotes[selectedNote].info.touch.connectedNotes[n] = noteI;
                break;
            }
        }
    }
}

static void spreadTouchHoldGroups(int noteI){
    const int lenOfConnectedNotes = chartNotes[noteI].info.touchHold.lenOfConnectedNotes;
    for(int i = 0; i < lenOfConnectedNotes; i++){
        const int selectedNote = chartNotes[noteI].info.touchHold.connectedNotes[i];
        const size_t memSize = sizeof(int) * lenOfConnectedNotes;
        chartNotes[selectedNote].info.touchHold.connectedNotes = (int*)SDL_malloc(memSize);
        chartNotes[selectedNote].info.touchHold.lenOfConnectedNotes = lenOfConnectedNotes;
        SDL_memcpy(chartNotes[selectedNote].info.touchHold.connectedNotes,
                   chartNotes[noteI].info.touchHold.connectedNotes, memSize);
        //correct one value
        for(int n = 0; n < lenOfConnectedNotes; n++){
            if(chartNotes[selectedNote].info.touchHold.connectedNotes[n] == selectedNote){
                chartNotes[selectedNote].info.touchHold.connectedNotes[n] = noteI;
                break;
            }
        }
    }
}

int getSensor(int sensor, int button){
    return (sensor - 'A') * 8 + (button) - ((sensor == 'D' || (sensor) == 'E') ? 7 : 1);
}

static int getAdjacentTouches(int (*touches)[8], int sensor, int button){
    switch(sensor){
        case 0:
            (*touches)[0] = getSensor('D', button);
            (*touches)[1] = getSensor('D', offsetButton(button, 1));
            (*touches)[2] = getSensor('E', button);
            (*touches)[3] = getSensor('E', offsetButton(button, 1));
            (*touches)[4] = getSensor('B', button);
            return 5;
        case 1:
            (*touches)[0] = getSensor('C', 1);
            (*touches)[1] = getSensor('B', offsetButton(button, 7));
            (*touches)[2] = getSensor('E', button);
            (*touches)[3] = getSensor('E', offsetButton(button, 1));
            (*touches)[4] = getSensor('B', offsetButton(button, 1));
            return 5;
        case 2:
            (*touches)[0] = getSensor('B', 1);
            (*touches)[1] = getSensor('B', 2);
            (*touches)[2] = getSensor('B', 3);
            (*touches)[3] = getSensor('B', 4);
            (*touches)[4] = getSensor('B', 5);
            (*touches)[5] = getSensor('B', 6);
            (*touches)[6] = getSensor('B', 7);
            (*touches)[7] = getSensor('B', 8);
            return 8;
        case 3:
            (*touches)[0] = getSensor('A', offsetButton(button, 7));
            (*touches)[1] = getSensor('E', button);
            (*touches)[2] = getSensor('A', button);
            return 3;
        case 4:
            (*touches)[0] = getSensor('D', button);
            (*touches)[1] = getSensor('A', offsetButton(button, 7));
            (*touches)[2] = getSensor('B', offsetButton(button, 7));
            (*touches)[3] = getSensor('B', button);
            (*touches)[4] = getSensor('A', button);
            return 5;
        default:
            printf("illegal touch sensor %d in touch group creation\n", sensor);
    }
    return 0;
}

//todo: include current note so only one malloc for each connected note
static bool hasSameNoteIDOrIsNotTouchType(int noteI, int noteID){
    if(chartNotes[noteI].noteType == noteTypeTouch){
        return chartNotes[noteI].info.touch.noteID == noteID;
    }

    if(chartNotes[noteI].noteType == noteTypeTouchHold){
        return chartNotes[noteI].info.touchHold.noteID == noteID;
    }

    return true;
}

static void assignTouchGroups(int originNote){
    bool connectedTouches[34] = {0};

    noteObject* note = &chartNotes[originNote];
    const int originNoteID = note->info.touch.noteID; //touch

    int startN = originNote;
    for(startN = originNote; startN < chartLength &&
                             chartNotes[startN].isEach &&
                             hasSameNoteIDOrIsNotTouchType(startN, originNoteID); startN++){}

    for(int connectedI = -1; connectedI < chartNotes[originNote].info.touch.lenOfConnectedNotes; connectedI++){
        const int noteI = connectedI <= -1 ? originNote : chartNotes[originNote].info.touch.connectedNotes[connectedI];
        const int realLocation = chartNotes[noteI].location < 2 * 8 + 2 ?
                                 chartNotes[noteI].location :
                                 chartNotes[noteI].location + 6;
        const int button = realLocation % 8 + 1;

        int touches[8] = {0};
        const int len = getAdjacentTouches(&touches, realLocation / 8, button);

        for(int n = startN - 1; n >= 0 &&
                                chartNotes[n].isEach &&
                                hasSameNoteIDOrIsNotTouchType(n, originNoteID); n--){
            if(chartNotes[n].noteType != noteTypeTouch){
                continue;
            }

            const int illegalTouch = note->location;
            const int newTouch = chartNotes[n].location;
            if(newTouch == illegalTouch){
                continue;
            }

            for(int i = 0; i < len; i++){
                if(touches[i] != newTouch || touches[i] == illegalTouch || connectedTouches[newTouch]){
                    continue;
                }

                const int lenOfConnectedNotes = note->info.touch.lenOfConnectedNotes;

                if(lenOfConnectedNotes == 0){
                    note->info.touch.connectedNotes = (int*)SDL_malloc(sizeof(int) * 33);
                }
                note->info.touch.connectedNotes[lenOfConnectedNotes] = n;
                note->info.touch.lenOfConnectedNotes++;
                connectedTouches[newTouch] = true;
            }
        }
    }
}

static void assignTouchHoldGroups(int originNote){
    bool connectedTouches[34] = {0};

    noteObject* note = &chartNotes[originNote];
    const int originNoteID = note->info.touchHold.noteID; //touch hold

    int startN = originNote;
    for(startN = originNote; startN < chartLength &&
                             chartNotes[startN].isEach &&
                             hasSameNoteIDOrIsNotTouchType(startN, originNoteID); startN++){}

    for(int connectedI = -1; connectedI < chartNotes[originNote].info.touchHold.lenOfConnectedNotes; connectedI++){
        const int noteI = connectedI <= -1 ? originNote : chartNotes[originNote].info.touchHold.connectedNotes[connectedI];
        const int realLocation = chartNotes[noteI].location < 2 * 8 + 2 ?
                                 chartNotes[noteI].location :
                                 chartNotes[noteI].location + 6;
        const int button = realLocation % 8 + 1;

        int touches[8] = {0};
        const int len = getAdjacentTouches(&touches, realLocation / 8, button);

        for(int n = startN - 1; n >= 0 &&
                                chartNotes[n].isEach &&
                                hasSameNoteIDOrIsNotTouchType(n, originNoteID); n--){
            if(chartNotes[n].noteType != noteTypeTouchHold){
                continue;
            }

            const int illegalTouch = note->location;
            const int newTouch = chartNotes[n].location;
            if(newTouch == illegalTouch){
                continue;
            }

            for(int i = 0; i < len; i++){
                if(touches[i] != newTouch || touches[i] == illegalTouch || connectedTouches[newTouch]){
                    continue;
                }

                const int lenOfConnectedNotes = note->info.touchHold.lenOfConnectedNotes;

                if(lenOfConnectedNotes == 0){
                    note->info.touchHold.connectedNotes = (int*)SDL_malloc(sizeof(int) * 33);
                }
                note->info.touchHold.connectedNotes[lenOfConnectedNotes] = n;
                note->info.touchHold.lenOfConnectedNotes++;
                connectedTouches[newTouch] = true;
            }
        }
    }
}

static int minimalToNormalSensor(int sensor, int n, bool isBottomSide ,bool isLeftSide){
    if(sensor == 2){
        return sensor * 8 + isLeftSide;
    } else if(isLeftSide){
        if(isBottomSide){
            return sensor * 8 + n + 4;
        } else {
            return sensor * 8 + 1 - n + 6;
        }
    } else {
        if(isBottomSide){
            return sensor * 8 + 1 - n + 2;
        } else {
            return sensor * 8 + n;
        }
    }
}

static int getSlideTouchSensor(SDL_FPoint location, bool noSensorA, bool noSensorC){ //faster for slide sensors due to optimized array accesses and no division polygon check
    const bool isBottomSide = location.y > 0.5f; //less but noticeable
    const bool isLeftSide = location.x < 0.5f; //easy

    float px = (location.x - 0.5f) * 2;
    float py = (location.y - 0.5f) * 2;
    if(isLeftSide){
        px = -px;
    }

    if(isBottomSide){
        py = -py;
    }

    int total = noSensorA ? sensorsLength[0] * 2 : 0;
    for(int sensor = noSensorA; sensor < (3 - noSensorC); sensor++){
        for(int n = 0; n < 2 - (sensor == 2); n++){
            //polygon collision time:
            //sensors A, B, and C are convex, special check used to avoid division
            bool isInside = true;
            bool isRight = false;
            const int len = sensorsLength[sensor] - (sensor == 2);
            for(int i = 0; i < len; i++){
                const int nextToCompare = i + 1 >= len ? total : total + i + 1; //easiest speed up ever


                //more speed
                const SDL_FPoint currentXY = optimizedSlideSensorPoints[total + i];
                const SDL_FPoint nextXY = optimizedSlideSensorPoints[nextToCompare];

                //absolute performance
                const float result = (nextXY.x - currentXY.x) * (py - currentXY.y) - (px - currentXY.x) * (nextXY.y - currentXY.y);
                if(i > 0 && (result > 0.f) != isRight){ //high branch mispredict
                    isInside = false;
                    break;
                }

                isRight = result > 0.f;
            }

            total += len;

            if(!isInside){
                continue;
            }

            return minimalToNormalSensor(sensor, n, isBottomSide, isLeftSide);
        }
    }
    return -1;
}

int getTouchSensorAtFPoint(SDL_FPoint location){
    const int slideSensorResult = getSlideTouchSensor(location, false, false);

    if(slideSensorResult != -1){
        return slideSensorResult;
    }
    //very easy performance optimizations
    const bool isLeftSide = location.x < 0.5f;
    const bool isVertical = isLeftSide ? location.y < 0.5f : location.y > 0.5f;
    const int offsetN = isLeftSide ? (isVertical ? 6 : 4) : (isVertical ? 2 : 0);
    const float px = (location.x - 0.5f) * 2;
    const float py = (location.y - 0.5f) * 2;
    for(int sensor = 4; sensor >= 3; sensor--){ //reverse order to prevent issues with d sensor
        for(int n = 0; n < 3; n++){
            //same convex polygon code with hacks for d sensor
            bool isInside = true;
            bool isRight = false;
            const int realN = offsetN + n == 8 ? 0 : offsetN + n;
            const int len = sensorsLength[sensor] - (sensor == 3);
            for(int i = 0; i < len; i++){
                const int realI = i + ((sensor == 3) && i >= 2); //skip concave part
                const int nextToCompare = i + 1 >= len ? 0 : i + 1 + ((sensor == 3) && i >= 1); //easiest speed up ever

                const SDL_FPoint currentXY = rotatedSensorPoints[sensor][realN][realI];
                const SDL_FPoint nextXY = rotatedSensorPoints[sensor][realN][nextToCompare];

                const float result = (nextXY.x - currentXY.x) * (py - currentXY.y) - (px - currentXY.x) * (nextXY.y - currentXY.y);
                if(i > 0 && (result > 0.f) != isRight){ //high branch mispredict
                    isInside = false;
                    break;
                }

                isRight = result > 0.f;
            }

            if(!isInside){
                continue;
            }

            if(sensor != 3){
                return sensor * 8 + realN - 6;
            }

            //check to see if point is inside invisible triangle
            for(int i = 1; i <= 3; i++){
                const int nextToCompare = i >= 3 ? 1 : i + 1; //easiest speed up ever

                const SDL_FPoint currentXY = rotatedSensorPoints[sensor][realN][i];
                const SDL_FPoint nextXY = rotatedSensorPoints[sensor][realN][nextToCompare];

                const float result = (nextXY.x - currentXY.x) * (py - currentXY.y) - (px - currentXY.x) * (nextXY.y - currentXY.y);
                if(i > 1 && (result > 0.f) != isRight){ //high branch mispredict
                    return sensor * 8 + realN - 6;
                }

                isRight = result > 0.f;
            }
            return -1;
        }
    }
    return -1;
}

typedef struct chartNoteProperties {
    int start;
    int length;
} chartNoteProperties;

static chartErrorRange getSlideDelayAndEndDuration(int* slideInputType, int* slideNumUsed, char* slideNumbersStart[4],
                                                   double* delayLength, double* slideBeatsPerMinute, double* endLength,
                                                   double* beatDivisor, double* beatDivisorMultiplier, int i){
    #define checkForSlideError(validNum) if_unlikely(*slideNumUsed != validNum){setError("Invalid slideNumUsed number (%d) in slideInputType %d.\nExpected: %d", *slideNumUsed, slideInputType, validNum);return errorAt(i);}

    switch(*slideInputType){
        case normalSlideRatio:
            checkForSlideError(1);
            *beatDivisor = SDL_strtod(slideNumbersStart[0], NULL);
            *beatDivisorMultiplier = SDL_strtod(slideNumbersStart[1], NULL);
            break;
        case customSlideDuration:
            checkForSlideError(0);
            *endLength = SDL_strtod(slideNumbersStart[0], NULL);
            break;
        case customBPMNormalSlideRatio:
            checkForSlideError(2);
            *slideBeatsPerMinute = SDL_strtod(slideNumbersStart[0], NULL);
            *beatDivisor = SDL_strtod(slideNumbersStart[1], NULL);
            *beatDivisorMultiplier = SDL_strtod(slideNumbersStart[2], NULL);
            break;
        case customBPMAndSlideDuration:
            checkForSlideError(1);
            *slideBeatsPerMinute = SDL_strtod(slideNumbersStart[0], NULL);
            *endLength = SDL_strtod(slideNumbersStart[1], NULL);
            break;
        case customWaitingTimeAndSlideDuration:
            checkForSlideError(1);
            *delayLength = SDL_strtod(slideNumbersStart[0], NULL);
            *endLength = SDL_strtod(slideNumbersStart[1], NULL);
            break;
        case customWaitingTimeNormalSlideRatio:
            checkForSlideError(2);
            *delayLength = SDL_strtod(slideNumbersStart[0], NULL);
            *beatDivisor = SDL_strtod(slideNumbersStart[1], NULL);
            *beatDivisorMultiplier = SDL_strtod(slideNumbersStart[2], NULL);
            break;
        case customWaitingTimeAndBPMNormalSlideRatio:
            checkForSlideError(3);
            *delayLength = SDL_strtod(slideNumbersStart[0], NULL);
            *slideBeatsPerMinute = SDL_strtod(slideNumbersStart[1], NULL);
            *beatDivisor = SDL_strtod(slideNumbersStart[2], NULL);
            *beatDivisorMultiplier = SDL_strtod(slideNumbersStart[3], NULL);
            break;
        default:
            setError("invalid slideInputType (%d).", slideInputType);
            return errorAt(i);
    }
    #undef checkForSlideError
    return errorAtWithLength(0, 0); //not error
}

static void addIndexToSlide(int noteI, int touch1, int touch2, int arrowIndex, int eventType){
    const int len = chartNotes[noteI].info.slide.lenOfStartIndex;
    if(len % 32 == 0){
        chartNotes[noteI].info.slide.startIndexes = (slideTouchItem*)reallocfTypeSDL(chartNotes[noteI].info.slide.startIndexes, (size_t)(len + 32) * sizeof(slideTouchItem));
    }
    chartNotes[noteI].info.slide.startIndexes[len].touchLocation1 = touch1;
    chartNotes[noteI].info.slide.startIndexes[len].touchLocation2 = touch2;
    chartNotes[noteI].info.slide.startIndexes[len].eventType = eventType;
    chartNotes[noteI].info.slide.startIndexes[len].newArrowIndex = arrowIndex;
    chartNotes[noteI].info.slide.lenOfStartIndex++;
}

static void fixSlideIndexesForNote(int note){
    const int len = chartNotes[note].info.slide.lenOfStartIndex;
    if(len <= 1){
        return;
    }

    //todo: implement actual slide indexes once and for all
    double* startIndexes = (double*)SDL_malloc(sizeof(double) * len);

    int previousValue = chartNotes[note].info.slide.startIndexes[0].newArrowIndex; //this or 0?
    startIndexes[0] = previousValue;
    for(int i = 1; i < len; i++){
        const int newPreviousValue = chartNotes[note].info.slide.startIndexes[i].newArrowIndex;
        chartNotes[note].info.slide.startIndexes[i].newArrowIndex += previousValue;
        const double value = chartNotes[note].info.slide.startIndexes[i].newArrowIndex / 2.0;
        chartNotes[note].info.slide.startIndexes[i].newArrowIndex = SDL_ceil(value);
        startIndexes[i] = value;
        previousValue = newPreviousValue;
    }


    for(int i = 0; i < len - 1; i++){
        const double currentIndex = SDL_floor(startIndexes[i]);
        const double nextIndex = SDL_ceil(startIndexes[i + 1]);
        chartNotes[note].info.slide.startIndexes[i].newArrowIndex += SDL_ceil((nextIndex - currentIndex) / 2.0);
    }

    SDL_free(startIndexes);
}

static void applyIndexesToSlide(int noteI){
    int lastButton1 = -1;
    int lastButton2 = -1;

    bool noSensorA = false; //to speed up slightly
    bool noSensorC = true; //slide starts at one of the A or C sensors, smaller speed up

    for(int i = 0; i < chartNotes[noteI].info.slide.lenOfArrows; i++) {
#define currentArrow (chartNotes[noteI].info.slide.arrows[i])

        const SDL_FPoint currentFPoint = ((SDL_FPoint){currentArrow.x, currentArrow.y});
        const int newButton = getSlideTouchSensor(currentFPoint, noSensorA, noSensorC); //worlds easiest speedup

        if(newButton == -1 || lastButton1 == newButton || lastButton2 == newButton){
            continue;
        }

        int otherButton = newButton == getSensor('C', 1) ? getSensor('C', 2) :
                          newButton == getSensor('C', 2) ? getSensor('C', 1) : -1;

        if(otherButton != -1){ //sensor c
            noSensorA = true;
            noSensorC = true;
        } else if(newButton < 8){ //sensor a
            noSensorA = false;
            noSensorC = true;
        } else { //sensor b
            noSensorA = false;
            noSensorC = false;
        }

        //true nightmare to calibrate

        if(newButton / 8 == 0){ //sensor A
            const float dist = squareDistanceBetweenTwoFPoints(&currentFPoint, &touchLocations[newButton + 8]);
            if(dist < 0.018f &&
               !(dist > 0.014f && dist < 0.015f) && //very specific hack for 1q5
               !(dist > 0.0175f && dist < 0.01784f) && //another hack for 1q4 and 1pp7
               newButton + 8 != lastButton1 &&
               newButton + 8 != lastButton2){
                otherButton = newButton + 8;
            }
        } else if(newButton / 8 == 1){ //sensor B
            //try c sensor
            float dist = squareDistanceBetweenTwoFPoints(&currentFPoint, &touchLocations[8 * 2]);
            if(dist < 0.0275f && 8 * 2 != lastButton1 && 8 * 2 != lastButton2){
                otherButton = 8 * 2;
            } else {
                int minus1 = newButton - 1;
                if(minus1 < 8){
                    minus1 = 8 * 2 - 1;
                }
                dist = squareDistanceBetweenTwoFPoints(&currentFPoint, &touchLocations[minus1]);
                if(dist < 0.011f && minus1 != lastButton1 && minus1 != lastButton2){
                    otherButton = minus1;
                } else {
                    int plus1 = newButton + 1;
                    if(plus1 >= 8 * 2){
                        plus1 = 8;
                    }
                    dist = squareDistanceBetweenTwoFPoints(&currentFPoint, &touchLocations[plus1]);
                    if(dist < 0.011f && plus1 != lastButton1 && plus1 != lastButton2){
                        otherButton = plus1;
                    }
                }
            }
        } else if(newButton / 8 == 2 && angleAwayFromTwoFPoints(currentFPoint, touchLocations[8 * 2], currentArrow.dir) > 15.f){
            for(int t = 8; t < 8 * 2; t++){
                const float dist = squareDistanceBetweenTwoFPoints(&currentFPoint, &touchLocations[t]);
                //printf("%.4f\n", dist);
                if(dist < 0.016f && t != lastButton1 && t != lastButton2){
                    otherButton = t;
                    break;
                }
            }
        }

        addIndexToSlide(noteI, newButton, otherButton == -1 ? newButton : otherButton, i, slideEventOn);
        addIndexToSlide(noteI, newButton, otherButton == -1 ? newButton : otherButton, i, slideEventUp);

        lastButton1 = newButton;
        lastButton2 = otherButton == -1 ? newButton : otherButton;
        #undef currentArrow
    }

    //last index is not up
    if(chartNotes[noteI].info.slide.lenOfStartIndex > 0 &&
       chartNotes[noteI].info.slide.startIndexes[chartNotes[noteI].info.slide.lenOfStartIndex - 1].eventType == slideEventUp){
        chartNotes[noteI].info.slide.lenOfStartIndex--;
    }
    fixSlideIndexesForNote(noteI);
}

MIX_Audio* chartAudio = NULL;
MIX_Track* chartSong = NULL;

static int reallocChartNotesBy(int changeByValue){
    const int divBy = SDL_ceil(chartLength / 256.0);
    const int newDivBy = SDL_ceil((chartLength + changeByValue) / 256.0);
    if(divBy != newDivBy){ //would this work for decreasing size?
        chartNotes = (noteObject*)reallocfTypeSDL(chartNotes, (size_t)(newDivBy * 256) * sizeof(noteObject));
    }
    return 0;
}

static slideItem* slidePoints = NULL;
int lenOfSlidePoints = 0;

static void addSlidePoint(int noteLocation, int slideTypeInput){
    if(slidePoints == NULL || (lenOfSlidePoints % 32 == 0 && lenOfSlidePoints > 0)){
        slidePoints = (slideItem*)reallocfTypeSDL(slidePoints, (size_t)(lenOfSlidePoints + 32) * sizeof(slideItem));
    }
    slidePoints[lenOfSlidePoints].location = noteLocation;
    slidePoints[lenOfSlidePoints].slideType = slideTypeInput;
    lenOfSlidePoints++;
}

static int getDistBetweenButton(int button1, int button2){
    const int diff = SDL_max(button1 - 1, button2 - 1) - SDL_min(button1 - 1, button2 - 1);

    return SDL_min(diff, 8 - diff);
}

static int getSlideDistBetweenTwoButtons(int button1, int button2){
    const int result = button2 - button1;
    if(result < 0){
        return 8 + result;
    }
    return result;
}

static void addArrow(float x, float y, float dir, bool isVisible) {
    const int len = chartNotes[chartLength].info.slide.lenOfArrows;
    if(len % 32 == 0){
        chartNotes[chartLength].info.slide.arrows = (slidePointInfo*)reallocfTypeSDL(chartNotes[chartLength].info.slide.arrows, sizeof(slidePointInfo) * (size_t)(len + 32));
    }
    chartNotes[chartLength].info.slide.arrows[len].x = x;
    chartNotes[chartLength].info.slide.arrows[len].y = y;
    chartNotes[chartLength].info.slide.arrows[len].dir = dir;
    chartNotes[chartLength].info.slide.arrows[len].isVisible = isVisible;
    if(len == 0){
        chartNotes[chartLength].info.slide.arrows[len].arrowTime = 0.0;
    } else {
        const double oldTime = chartNotes[chartLength].info.slide.arrows[len - 1].arrowTime;
        const float oldX = chartNotes[chartLength].info.slide.arrows[len - 1].x;
        const float oldY = chartNotes[chartLength].info.slide.arrows[len - 1].y;

        chartNotes[chartLength].info.slide.arrows[len].arrowTime = oldTime + distanceBetweenTwoPositions(oldX, oldY, x, y);
    }

    chartNotes[chartLength].info.slide.lenOfArrows++;
}

static void addFirstArrowIfStarted(float x, float y, float dir){
    if(chartNotes[chartLength].info.slide.lenOfArrows <= 0){
        addArrow(x, y, dir, false);
    }
}

static float addArrowsFromStartToEnd(const SDL_FPoint startFPoint, const SDL_FPoint endFPoint, float arrowDir, float distChange, bool shortEnd) {
    addFirstArrowIfStarted(startFPoint.x, startFPoint.y, arrowDir);

    const float normalizedArrowSize = 22.f / 720.f; //accurate apparently

    const float diffSize = (distChange / normalizedLineDist) * normalizedArrowSize;

    float distProgress = 0.f;
    for(distProgress = diffSize; distProgress < 1.f - diffSize * (float)shortEnd; distProgress += distChange) {
        const SDL_FPoint result = interpolateFPoint(startFPoint, endFPoint, distProgress);
        addArrow(result.x, result.y, arrowDir, true);
    }

    addArrow(endFPoint.x, endFPoint.y, arrowDir, false);
    return distProgress - distChange;
}

static float addArrowsFromStartToEndSimple(const SDL_FPoint startFPoint, const SDL_FPoint endFPoint){
    const float arrowDir = startFPPointsTowardEndFP(&startFPoint, &endFPoint);
    const float simpleDistChange = lineDistanceChangeCalculator(&startFPoint, &endFPoint);
    return addArrowsFromStartToEnd(startFPoint, endFPoint, arrowDir, simpleDistChange, true);
}

static float addArrowsFromStartToFullEnd(const SDL_FPoint startFPoint, const SDL_FPoint endFPoint){
    const float arrowDir = startFPPointsTowardEndFP(&startFPoint, &endFPoint);
    const float simpleDistChange = lineDistanceChangeCalculator(&startFPoint, &endFPoint);
    return addArrowsFromStartToEnd(startFPoint, endFPoint, arrowDir, simpleDistChange, false);
}

static void setupGenericNote(int noteI, int noteLocation, Sint64 noteHitTime, bool isEach, bool isBreak, bool isEX){
    chartNotes[noteI].location = noteLocation;
    chartNotes[noteI].noteHitTime = noteHitTime;
    chartNotes[noteI].hasHit = false;
    chartNotes[noteI].isEach = isEach;
    chartNotes[noteI].isBreak = isBreak;
    chartNotes[noteI].isEX = isEX;
}

static void setupTapNote(int noteI, int tapNoteSkin, double starNoteRotationSpeed){
    chartNotes[noteI].noteType = noteTypeTap;
    chartNotes[noteI].info.tap.starNoteRotationSpeed = starNoteRotationSpeed;
    chartNotes[noteI].info.tap.tapNoteSkin = tapNoteSkin;
}

static void setupHoldNote(int noteI, Sint64 holdLength){
    chartNotes[noteI].noteType = noteTypeHold;
    chartNotes[noteI].info.hold.holdLength = holdLength;
    chartNotes[noteI].info.hold.inputType = heldByNeither;
    chartNotes[noteI].info.hold.hasAcknowledgedHeld = false;
    chartNotes[noteI].info.hold.isHeld = false;
    chartNotes[noteI].info.hold.stopChecking = false;
    chartNotes[noteI].info.hold.hasStartedToCheck = false;
    chartNotes[noteI].info.hold.headJudgeDirection = judgeLate;
    chartNotes[noteI].info.hold.headJudgeType = -1; //fix some issues with rehitting for better judge
    chartNotes[noteI].info.hold.heldDuration = 0;
    chartNotes[noteI].info.hold.unheldOn = LLONG_MIN;
}

static void setupTouchNote(int noteI, int noteID, bool fireworksEnabled){
    chartNotes[noteI].noteType = noteTypeTouch;
    chartNotes[noteI].info.touch.noteID = noteID;
    chartNotes[noteI].info.touch.lenOfConnectedNotes = 0;
    chartNotes[noteI].info.touch.connectedNotes = NULL;
    chartNotes[noteI].info.touch.fireworksEnabled = fireworksEnabled;
    if(chartNotes[noteI].location > 2 * 8){
        chartNotes[noteI].location -= 6;
    }
}

static void setupTouchHoldNote(int noteI, int noteID, Sint64 holdLength, bool fireworksEnabled){
    chartNotes[noteI].noteType = noteTypeTouchHold;
    chartNotes[noteI].info.touchHold.noteID = noteID;
    chartNotes[noteI].info.touchHold.lenOfConnectedNotes = 0;
    chartNotes[noteI].info.touchHold.connectedNotes = NULL;
    chartNotes[noteI].info.touchHold.holdLength = holdLength;
    chartNotes[noteI].info.touchHold.fireworksEnabled = fireworksEnabled;
    chartNotes[noteI].info.touchHold.isC1Held = false;
    chartNotes[noteI].info.touchHold.isC2Held = false;
    chartNotes[noteI].info.touchHold.hasAcknowledgedHeld = false;
    chartNotes[noteI].info.touchHold.isHeld = false;
    chartNotes[noteI].info.touchHold.stopChecking = false;
    chartNotes[noteI].info.touchHold.hasStartedToCheck = false;
    chartNotes[noteI].info.touchHold.headJudgeType = -1; //fix some issues with rehitting for better judge
    chartNotes[noteI].info.touchHold.heldDuration = 0;
    chartNotes[noteI].info.touchHold.unheldOn = LLONG_MIN;

    if(chartNotes[noteI].location > 2 * 8){
        chartNotes[noteI].location -= 6;
    }
}

static void setupSlideNote(int noteI, int endLocation, Sint64 delayLength, Sint64 slideLength, bool isTransitionSlide, bool suddenStarAppearance){
    chartNotes[noteI].noteType = noteTypeSlide;
    chartNotes[noteI].info.slide.arrows = NULL; //prevent crash
    chartNotes[noteI].info.slide.lenOfArrows = 0;
    chartNotes[noteI].info.slide.startIndexes = NULL; //heap moment
    chartNotes[noteI].info.slide.lenOfStartIndex = 0;
    chartNotes[noteI].info.slide.suddenStarAppearance = suddenStarAppearance;
    chartNotes[noteI].info.slide.endButton = endLocation; //for judgement to work properly
    chartNotes[noteI].info.slide.indexOfLocationTimes = 0; //segfault moment
    chartNotes[noteI].info.slide.indexOfStartIndex = 0;
    chartNotes[noteI].info.slide.isTransitionSlide = isTransitionSlide;
    chartNotes[noteI].info.slide.delayLength = delayLength;
    chartNotes[noteI].info.slide.slideLength = slideLength;
    chartNotes[noteI].info.slide.flipBigJudgeX = false;
}

static void setupFanSlideNote(int noteI, int numberOfFanSlides, double* locationTimes, Sint64 delayLength, Sint64 slideLength, bool suddenStarAppearance){
    chartNotes[noteI].noteType = noteTypeFanSlide;
    chartNotes[noteI].info.fanSlide.indexOfLocationTimes = 0;
    chartNotes[noteI].info.fanSlide.locationTimes = locationTimes;
    chartNotes[noteI].info.fanSlide.suddenStarAppearance = suddenStarAppearance;
    chartNotes[noteI].info.fanSlide.delayLength = delayLength;
    chartNotes[noteI].info.fanSlide.slideLength = slideLength;
    chartNotes[noteI].info.fanSlide.totalFanSlides = numberOfFanSlides;
    chartNotes[noteI].info.fanSlide.slideIndex = 0;
    chartNotes[noteI].info.fanSlide.leftProgress = 0;
    chartNotes[noteI].info.fanSlide.middleProgress = 0;
    chartNotes[noteI].info.fanSlide.rightProgress = 0;
}

static int SDLCALL answerSoundSortByStartTime(const void *a, const void *b) {
    const answerSfxInfo* A = (const answerSfxInfo*)a;
    const answerSfxInfo* B = (const answerSfxInfo*)b;
    return SDL_clamp(A->playSoundAt - B->playSoundAt, -1, 1);
}

static void addSfxToNotes(void){
    lenOfAnswerPlayTimes = 0;
    if(answerPlayTimes != NULL){
        SDL_free(answerPlayTimes);
    }
    answerPlayTimes = (answerSfxInfo*)SDL_malloc(sizeof(answerSfxInfo) * (size_t)(chartLength * 3 + 1)); //+ 1 to prevent malloc(0)

    for(int i = 0; i < chartLength; i++){ //
        //make sure to specify break somewhere
        //add sfx based on note type
        #define addAnswerSfx(additionalTime, sfxNumber) answerPlayTimes[lenOfAnswerPlayTimes++] = (answerSfxInfo){chartNotes[i].noteHitTime + additionalTime, sfxNumber};
        switch(chartNotes[i].noteType){
            case noteTypeTap:
                if(chartNotes[i].isBreak){
                    addAnswerSfx(0, breakNoteSfxNum);
                    if(chartNotes[i].isEX){
                        addAnswerSfx(0, EXNoteSfxNum);
                    }
                } else if(chartNotes[i].isEX){
                    addAnswerSfx(0, EXNoteSfxNum);
                } else {
                    addAnswerSfx(0, tapNoteSfxNum);
                }
                break;
            case noteTypeHold:
                if(chartNotes[i].isBreak){
                    addAnswerSfx(0, breakNoteSfxNum);
                    if(chartNotes[i].isEX){
                        addAnswerSfx(0, EXNoteSfxNum);
                    }
                } else if(chartNotes[i].isEX){
                    addAnswerSfx(0, EXNoteSfxNum);
                } else {
                    addAnswerSfx(0, tapNoteSfxNum);
                }


                addAnswerSfx(chartNotes[i].info.hold.holdLength, holdNoteSfxNum);
                break;
            case noteTypeSlide:
                if(enableSlideSFX){
                    addAnswerSfx(chartNotes[i].info.slide.delayLength, slideStartSfxNum);
                }
                break;
            case noteTypeFanSlide:
                if(enableSlideSFX){
                    addAnswerSfx(chartNotes[i].info.fanSlide.delayLength, slideStartSfxNum);
                }
                break;
            case noteTypeTouch:
                addAnswerSfx(0, touchNoteSfxNum);
                break;
            case noteTypeTouchHold:
                addAnswerSfx(0, touchNoteSfxNum);
                addAnswerSfx(chartNotes[i].info.touchHold.holdLength, holdNoteSfxNum);
                break;
            default:
                break;
        }
        #undef addAnswerSfx
    }
    SDL_qsort(answerPlayTimes, (size_t)lenOfAnswerPlayTimes, sizeof(answerPlayTimes[0]), answerSoundSortByStartTime);
}

static void calculateMaxScores(void){
    absoluteMaxBaseScore = 0;
    absoluteMaxBreakBonus = 0;
    absoluteMaxDXScore = chartLength * DXScoreTable[judgePerfectCritical];
    for(int i = 0; i < chartLength; i++){
        if(chartNotes[i].isBreak){
            absoluteMaxBaseScore += pointsTable[3][judgePerfectCritical];
            absoluteMaxBreakBonus += pointsTable[4][judgePerfectCritical];
            continue;
        }

        switch(chartNotes[i].noteType){
            case noteTypeTap:
            case noteTypeTouch:
                absoluteMaxBaseScore += pointsTable[0][judgePerfectCritical];
                break;
            case noteTypeHold:
            case noteTypeTouchHold:
                absoluteMaxBaseScore += pointsTable[1][judgePerfectCritical];
                break;
            case noteTypeSlide:
            case noteTypeFanSlide:
                absoluteMaxBaseScore += pointsTable[2][judgePerfectCritical];
                break;
            default:
                break;
        }
    }
}

static void setChartFeatures(void){
    for(int i = 0; i < chartLength; i++){
        if(chartNotes[i].isEX){
            chartNoteFeatures[featureEX] = true;
        }

        if(chartNotes[i].isBreak){
            switch(chartNotes[i].noteType){
                case noteTypeTap:
                    chartNoteFeatures[chartNotes[i].info.tap.tapNoteSkin == tapNoteNormalSkin ? featureTapBreak :
                                      chartNotes[i].info.tap.tapNoteSkin == tapNoteMultiStarSkin ? featureMultiSlideTapBreak :
                                      featureSlideTapBreak] = true;

                    break;
                case noteTypeHold:
                    chartNoteFeatures[featureHoldBreak] = true;
                    break;
                case noteTypeSlide:
                    chartNoteFeatures[featureSlideBreak] = true;
                    break;
                case noteTypeFanSlide:
                    chartNoteFeatures[featureFanSlideBreak] = true;
                    break;
                default:
                    break;
            }
            continue;
        }

        if(chartNotes[i].isEach){
            switch(chartNotes[i].noteType){
                case noteTypeTap:
                    chartNoteFeatures[chartNotes[i].info.tap.tapNoteSkin == tapNoteNormalSkin ? featureTapEach :
                                      chartNotes[i].info.tap.tapNoteSkin == tapNoteMultiStarSkin ? featureMultiSlideTapEach :
                                      featureSlideTapEach] = true;
                    break;
                case noteTypeTouch:
                    if(chartNotes[i].info.touch.fireworksEnabled){
                        chartNoteFeatures[featureFireworks] = true;
                    }

                    chartNoteFeatures[featureTouchEach] = true;
                    break;
                case noteTypeHold:
                    chartNoteFeatures[featureHoldEach] = true;
                    break;
                case noteTypeTouchHold:
                    if(chartNotes[i].info.touchHold.fireworksEnabled){
                        chartNoteFeatures[featureFireworks] = true;
                    }

                    chartNoteFeatures[featureTouchHold] = true;
                    break;
                case noteTypeSlide:
                    chartNoteFeatures[featureSlideEach] = true;
                    break;
                case noteTypeFanSlide:
                    chartNoteFeatures[featureFanSlideEach] = true;
                    break;
                default:
                    break;
            }
            continue;
        }

        switch(chartNotes[i].noteType){
            case noteTypeTap:
                chartNoteFeatures[chartNotes[i].info.tap.tapNoteSkin == tapNoteNormalSkin ? featureTapNormal :
                                  chartNotes[i].info.tap.tapNoteSkin == tapNoteMultiStarSkin ? featureMultiSlideTapNormal :
                                  featureSlideTapNormal] = true;
                break;
            case noteTypeTouch:
                if(chartNotes[i].info.touch.fireworksEnabled){
                    chartNoteFeatures[featureFireworks] = true;
                }

                chartNoteFeatures[featureTouchNormal] = true;
                break;
            case noteTypeHold:
                chartNoteFeatures[featureHoldNormal] = true;
                break;
            case noteTypeTouchHold:
                if(chartNotes[i].info.touchHold.fireworksEnabled){
                    chartNoteFeatures[featureFireworks] = true;
                }

                chartNoteFeatures[featureTouchHold] = true;
                break;
            case noteTypeSlide:
                chartNoteFeatures[featureSlideNormal] = true;
                break;
            case noteTypeFanSlide:
                chartNoteFeatures[featureFanSlideNormal] = true;
                break;
            default:
                break;
        }
    }
}

static void applyTouchGroupSpread(void){
    for(int i = 0; i < chartLength; i++){
        if(!chartNotes[i].isEach){
            continue;
        }

        if(chartNotes[i].noteType == noteTypeTouch &&
           chartNotes[i].info.touch.lenOfConnectedNotes == 0){
            assignTouchGroups(i);
            spreadTouchGroups(i); //speeds up 51% rule assignment
        } else if(chartNotes[i].noteType == noteTypeTouchHold &&
                  chartNotes[i].info.touchHold.lenOfConnectedNotes == 0){
            assignTouchHoldGroups(i);
            spreadTouchHoldGroups(i); //speeds up 51% rule assignment
        }
    }
}

static void normalizeSlideTimes(void){
    for(int i = 0; i < chartLength; i++){
        if(chartNotes[i].noteType != noteTypeSlide){
            continue;
        }

        const int lenOfArrows = chartNotes[i].info.slide.lenOfArrows;
        const double maxDist = chartNotes[i].info.slide.arrows[lenOfArrows - 1].arrowTime;
        //printf("maxDist for %d: %.8f\n", i, maxDist);
        for(int x = 0; x < lenOfArrows - 1; x++){
            chartNotes[i].info.slide.arrows[x].arrowTime /= maxDist;
        }
        chartNotes[i].info.slide.arrows[lenOfArrows - 1].arrowTime = 1.0;

        //uncomment if u want to update last zone table (might need some changes)
        /*double normalizedTime = 0.03;
        double sensorDFirst = 0.0;
        double sensorDSecond = 0.0;
        const int len = chartNotes[i].info.slide.lenOfStartIndex;
        int n = 0;
        Uint64 counter = 0;
        bool forceSlow = false;
        double data[512] = {0};
        int lenOfData = 0;
        while(n < len){
            //normalizedTime = nextafter(normalizedTime, 10000.0);
            if(n + 1 == len || chartNotes[i].info.slide.drawType == judgeSlideArc){
                normalizedTime += 0.00000001;
            } else {
                normalizedTime += 0.00001;
            }
            counter++;
            if(counter % 50000000 == 0){
                //printf("counter %llu %d/%d normalized: %.17f\n", counter, n, len, normalizedTime);
            }
            while(chartNotes[i].info.slide.arrows[chartNotes[i].info.slide.indexOfLocationTimes].arrowTime < normalizedTime){
                chartNotes[i].info.slide.indexOfLocationTimes = SDL_min(chartNotes[i].info.slide.indexOfLocationTimes + 1, lenOfArrows);
            }

            const int indexOfLocationTimes = chartNotes[i].info.slide.indexOfLocationTimes;

            const double nextTime = chartNotes[i].info.slide.arrows[indexOfLocationTimes].arrowTime;
            const double previousTime = indexOfLocationTimes == 0 ? 0.0 : chartNotes[i].info.slide.arrows[indexOfLocationTimes - 1].arrowTime;
            const float realProgress = SDL_min((normalizedTime - previousTime) / (nextTime - previousTime), 1.f);
            SDL_FPoint start = {0, 0};
            SDL_FPoint end = {0, 0};

            if(indexOfLocationTimes == 0){
                start = moveNoteXY(0.5f, 0.5f, chartNotes[i].location, borderRadiusPercent);
            } else {
                start = ((SDL_FPoint){chartNotes[i].info.slide.arrows[indexOfLocationTimes - 1].x, chartNotes[i].info.slide.arrows[indexOfLocationTimes - 1].y});
            }

            if(indexOfLocationTimes == lenOfArrows){
                end = moveNoteXY(getCenterX(), getCenterY(), chartNotes[i].info.slide.endButton, borderRadius * guiScale);
            } else {
                end = ((SDL_FPoint){chartNotes[i].info.slide.arrows[indexOfLocationTimes].x, chartNotes[i].info.slide.arrows[indexOfLocationTimes].y});
            }
            const SDL_FPoint noteFPoint = interpolateFPoint(start, end, realProgress);
            const int result = getTouchSensorAtFPoint(noteFPoint); //todo: do not check E sensor??
            if(result >= 8 * 2 + 2){
                if(lenOfData % 2 == 0 && lenOfData < stackArraySizeInt(data)){
                    data[lenOfData] = normalizedTime;
                    lenOfData++;
                }
            } else if(lenOfData % 2 == 1 && lenOfData < stackArraySizeInt(data)){
                data[lenOfData] = normalizedTime;
                lenOfData++;
            }

            if(isValidSlideTouch(i, result, n, slideEventOn)){
                n += 2;
                if(n < len){
                    normalizedTime += 0.001;
                }
            }

            if(result < 8 * 2 + 2 && n < len && result >= 0){
                normalizedTime += 0.00001;
            }
        }
        chartNotes[i].info.slide.indexOfLocationTimes = 0;
        printf("results for note %d: a: %.17f d: %.17f\n", i, 1.0 - normalizedTime, 1.0 - (lenOfData >= 2 ? ((1 - data[lenOfData - 1]) + (1 - data[lenOfData - 2])) / 2.0 : 1.0));*/
    }
}

static void drawSlideLine(int startLocation, int endLocation){
    const SDL_FPoint startFPoint = moveNoteXY(0.5f, 0.5f, startLocation, borderRadius50Percent);
    const SDL_FPoint endFPoint   = moveNoteXY(0.5f, 0.5f, endLocation, borderRadius50Percent);
    addArrowsFromStartToEndSimple(startFPoint, endFPoint);
}

static void drawSlideArc(int startLocation, int endLocation, bool clockwise){
    bool continueAnyway = !clockwise ? //for wrap around fix
      startLocation <= endLocation : //left
      startLocation >= endLocation;  //right

    //close enough values i guess
    const float distChange = !clockwise ? -0.0155f : 0.0155f;

    bool isFirstRun = true;
    SDL_FPoint circularFPoint = {0.f, 0.f};
    const float endDist = buttonToNormalizedDegrees(endLocation);

    for(float dist = buttonToNormalizedDegrees(startLocation); continueAnyway || SDL_fabsf(dist - endDist) > SDL_fabsf(distChange); dist += distChange){
        if(dist < 0) {
            dist++;
            continueAnyway = false;
        } else if(dist > 1) {
            dist--;
            continueAnyway = false;
        }

        circularFPoint = moveXYWithDirAndDistFloat(0.5f, 0.5f, dist * 360.f, borderRadius50Percent);
        const float dir = dist * 360.f - 180.f * !clockwise + 90.f;

        if(isFirstRun){ //fixes inaccurate display with slides such as 1^2^3^4[1:16]
            addFirstArrowIfStarted(circularFPoint.x, circularFPoint.y, dir);
            isFirstRun = false;
            continue;
        }

        addArrow(circularFPoint.x, circularFPoint.y, dir, true);
    }

    //final point
    circularFPoint = moveNoteXY(0.5f, 0.5f, endLocation, borderRadius50Percent);
    const float dir = endDist * 360.f - 180.f * !clockwise + 90.f;
    addArrow(circularFPoint.x, circularFPoint.y, dir, false);
}

static void drawSlideV(int startLocation, int endLocation){
    const SDL_FPoint startFPoint = moveNoteXY(0.5f, 0.5f, startLocation, borderRadius50Percent);
    const SDL_FPoint centerFPoint = {0.5f, 0.5f};
    const SDL_FPoint endFPoint = moveNoteXY(0.5f, 0.5f, endLocation, borderRadius50Percent);

    //first part: start to center
    addArrowsFromStartToFullEnd(startFPoint, centerFPoint); //different apparently
    //second part: center to end
    addArrowsFromStartToEndSimple(centerFPoint, endFPoint);
}

static void drawSlideZigzag(int startLocation, bool isSlideS){
    const int helperFPoint1Offset = isSlideS ? 5 : 3;
    const int helperFPoint2Offset = isSlideS ? 1 : 7;

    const int oppositeButton = offsetButton(startLocation, 4); //end point is always the opposite button
    const SDL_FPoint startFPoint = moveNoteXY(0.5f, 0.5f, startLocation, borderRadius50Percent);
    const SDL_FPoint endFPoint = moveNoteXY(0.5f, 0.5f, oppositeButton, borderRadius50Percent);

    const SDL_FPoint helperFPoint1 = moveNoteXY(0.5f, 0.5f, offsetButton(startLocation, helperFPoint1Offset), borderRadius50Percent);
    const SDL_FPoint helperFPoint2 = moveNoteXY(0.5f, 0.5f, offsetButton(startLocation, helperFPoint2Offset), borderRadius50Percent);

    const float resultDegree1 = startFPPointsTowardEndFP(&startFPoint, &helperFPoint1);
    const float resultDegree2 = startFPPointsTowardEndFP(&endFPoint, &helperFPoint2);

    const SDL_FPoint intermediateFPoint1 = moveXYWithDirAndDistFloat(startFPoint.x, startFPoint.y, resultDegree1, borderRadiusPercent / 10.0 * 5.4); //start to middle part 1
    const SDL_FPoint intermediateFPoint2 = moveXYWithDirAndDistFloat(endFPoint.x, endFPoint.y, resultDegree2, borderRadiusPercent / 10.0 * 5.4); //middle part 1 to middle part 2

    //different apparently
    addArrowsFromStartToFullEnd(startFPoint, intermediateFPoint1);
    addArrowsFromStartToFullEnd(intermediateFPoint1, intermediateFPoint2);
    addArrowsFromStartToEndSimple(intermediateFPoint2, endFPoint); //not sure how to make it more accurate
}

static void drawSlidePQ(int startLocation, int endLocation, bool isQShape){
    const float distMultiplier = isQShape ? 1.f : -1.f;
    const int buttonInFrontOfStart = offsetButton(startLocation, isQShape ? 3 : 5);
    const SDL_FPoint startFPoint = moveNoteXY(0.5f, 0.5f, startLocation, borderRadius50Percent);
    const SDL_FPoint helperFPoint1 = moveNoteXY(0.5f, 0.5f, buttonInFrontOfStart, borderRadius50Percent);
    const SDL_FPoint circleStartFPoint = moveXYWithDirAndDistFloat(startFPoint.x, startFPoint.y, startFPPointsTowardEndFP(&startFPoint, &helperFPoint1), 0.425f / 0.9f * borderRadiusPercent);
    addArrowsFromStartToEndSimple(startFPoint, circleStartFPoint);
    //objective: survive

    //circle part
    const float distChange = 0.015f * (2.7f);

    int intDist = isQShape ? endLocation - startLocation : startLocation - endLocation;
    if(intDist <= 0){
        intDist += 8;
    }
    float endDist = (intDist - 3.25f) * 45.f / 360.f;

    if(endDist < 0){
        endDist += 1;
    }
    SDL_FPoint circleCenterFPoint = {0.5f, 0.5f};


    const float startDist = buttonToNormalizedDegrees(startLocation + 1.65f * distMultiplier);
    SDL_FPoint lastFPoint = {0.f, 0.f};
    for(float dist = 0; dist < endDist; dist += distChange) {
        if(dist < 0) {
            dist++;
        } else if(dist > 1) {
            dist--;
            if(endDist >= 1){
                endDist--;
            }
        }
        const SDL_FPoint circularFPoint = moveXYWithDirAndDistFloat(circleCenterFPoint.x, circleCenterFPoint.y, (startDist + dist * distMultiplier) * 360.f, 0.1695f);

        addArrow(circularFPoint.x, circularFPoint.y, (startDist + dist * distMultiplier) * 360.f + 90.f * distMultiplier, true);
        lastFPoint.x = circularFPoint.x;
        lastFPoint.y = circularFPoint.y;

    }

    //end line part
    const SDL_FPoint endFPoint = moveNoteXY(0.5f, 0.5f, endLocation, borderRadius50Percent);
    const SDL_FPoint circleEndFPoint = moveXYWithDirAndDistFloat(lastFPoint.x, lastFPoint.y, startFPPointsTowardEndFP(&lastFPoint, &endFPoint), 0.02f * borderRadiusPercent);
    addArrowsFromStartToEndSimple(circleEndFPoint, endFPoint);
}

static void drawSlidePPQQ(int startLocation, int endLocation, bool isSlidePP){
    const SDL_FPoint startFPoint = moveNoteXY(0.5f, 0.5f, startLocation, borderRadius50Percent);
    const float distMultiplier = isSlidePP ? -1.f : 1.f; //real moment
    const SDL_FPoint circleCenterFPoint = moveXYWithDirAndDistFloat(startFPoint.x, startFPoint.y, buttonNumberToDegrees(startLocation) - 3.f * distMultiplier, -0.4f);


    const int towardsButton = offsetButton(startLocation, isSlidePP ? 2 : 6);
    //const SDL_FPoint newCircleCenterFPoint = moveNoteXY(circleCenterFPoint.x, circleCenterFPoint.y, towardsButton, (0.2 / 0.9) * borderRadiusPercent);
    //turns out this slide is not exactly 22.5 aligned
    const SDL_FPoint newCircleCenterFPoint = moveXYWithDirAndDistFloat(circleCenterFPoint.x, circleCenterFPoint.y, buttonNumberToDegrees(towardsButton) + 9.f * distMultiplier, (0.245f) * borderRadiusPercent);
    addArrowsFromStartToEndSimple(startFPoint, circleCenterFPoint);
    //time for impossible:


    const float startDist = buttonToNormalizedDegrees(startLocation + 2.125f * distMultiplier);


    float newDirectionForDraw;
    //const float endDist = 0.575f;
    const float distChange = 0.015f * (2.3f);
    int countIncrease = 16; //what
    float dist = -distChange; //counteracts increase in first iteration
    SDL_FPoint circularFPoint;
    int counter = 0;
    bool continueLoop = true;
    float angleDist = 0.f;

    do {
        dist += distChange / 16.f * ((float)countIncrease);
        const float newDirectionForMovement = (startDist + dist * distMultiplier) * 360.f;
        newDirectionForDraw = newDirectionForMovement + 90.f * distMultiplier;
        circularFPoint = moveXYWithDirAndDistFloat(newCircleCenterFPoint.x, newCircleCenterFPoint.y, newDirectionForMovement, (0.238f) * borderRadiusPercent);
        if(counter % 16 == 0){
            addArrow(circularFPoint.x, circularFPoint.y, newDirectionForDraw, true);
            if(dist >= 0.275f){
                countIncrease = (angleDist > 45.f) ? 16 : 1; //easy optimization
            }
        }

        //printf("counter: %d %.2f\n", counter / 30, dist);
        //printf("counteri: %d\n", countIncrease);
        if(dist < 0.275f){
            continueLoop = true;
        } else if_unlikely(dist >= 2.f){
            printf("pp/qq slide logic broken!\n");
            continueLoop = false;
        } else {
            angleDist = isGoingTowardsButton(circularFPoint, newDirectionForDraw, endLocation);
            //printf("%.4f %d\n", angleDist, counter % 16);
            continueLoop = angleDist > 1.f;
        }
        counter += countIncrease;
    } while(continueLoop);
    const SDL_FPoint endFPoint = moveNoteXY(0.5f, 0.5f, endLocation, borderRadius50Percent);
    const SDL_FPoint arrowStartFPoint = moveXYWithDirAndDistFloat(circularFPoint.x,
                                                                  circularFPoint.y,
                                                                  startFPPointsTowardEndFP(&circularFPoint, &endFPoint),
                                                                  distChange * (float)(8 - (counter % 16)) / 16.f);

    addArrowsFromStartToEndSimple(arrowStartFPoint, endFPoint);
}

static void drawSlideReflective(int startLocation, int endLocation, bool isCCW){
    const int secondaryButton = offsetButton(startLocation, isCCW ? 6 : 2);

    const SDL_FPoint startFPoint  = moveNoteXY(0.5f, 0.5f, startLocation, borderRadius50Percent);
    const SDL_FPoint middleFPoint = moveNoteXY(0.5f, 0.5f, secondaryButton, borderRadius50Percent);
    const SDL_FPoint endFPoint    = moveNoteXY(0.5f, 0.5f, endLocation, borderRadius50Percent);

    addArrowsFromStartToEndSimple(startFPoint, middleFPoint);
    addArrowsFromStartToEndSimple(middleFPoint, endFPoint);
}

static bool markPreviousSlideAsEach(void){
    for(int substractI = 1; (chartLength - substractI) >= 0; substractI++){
        const int result = chartLength - substractI;
        if(chartNotes[result].noteType == noteTypeSlide || chartNotes[result].noteType == noteTypeFanSlide){
            chartNotes[result].isEach = true;
            return true;
        }
    }
    return true;
}

chartErrorRange readChart(int startIndex, const char* chartLocation, const char* chartHash){
    minIndexToRender = 0;
    minIndexToPlaySounds = 0;
    generalMaxI = 0;
    touchMaxI = 0;
    fireworksStartTime = -99999999999;
    for(int i = 0; i < 8; i++){
        judgementInfoForButton[i].judgementDisplayTimer = -99999999999; //place holder
        judgementInfoForHeadHold[i].judgementDisplayTimer = -99999999999; //place holder
        judgementInfoForSlideEnd[i].properties.judgementDisplayTimer = -99999999999; //place holder
    }

    //touch
    for(int i = 0; i < 34; i++){
        judgementInfoForTouch[i].judgementDisplayTimer = -99999999999;
        judgementInfoForTouchHold[i].judgementDisplayTimer = -99999999999;
    }

    //reset chart
    for(int i = 0; i < chartLength; i++) {
        if(chartNotes[i].noteType == noteTypeSlide){
            if(chartNotes[i].info.slide.lenOfArrows > 0){
                SDL_free(chartNotes[i].info.slide.arrows);
            }

            if(chartNotes[i].info.slide.lenOfStartIndex > 0){
                SDL_free(chartNotes[i].info.slide.startIndexes);
            }
        } else if(chartNotes[i].noteType == noteTypeTouch){
            SDL_free(chartNotes[i].info.touch.connectedNotes);
        } else if(chartNotes[i].noteType == noteTypeFanSlide){
            SDL_free(chartNotes[i].info.fanSlide.locationTimes);
        }
    }

    if(chartNotes != NULL){
        SDL_free(chartNotes);
        chartNotes = NULL;
    }

    SDL_memset(chartNoteFeatures, 0, sizeof(chartNoteFeatures));


    const size_t locationMaxSize = SDL_strlen(chartLocation) + 32;
    char *location = SDL_calloc(locationMaxSize, sizeof(char));

    SDL_snprintf(location, locationMaxSize, "%s/maidata.txt", chartLocation);

    SDL_IOStream* chartRWops = SDL_IOFromFile(location, "rb");
    SDL_free(location);
    if_unlikely(chartRWops == NULL) {
        setError("Chart load error:\n%s\n", SDL_GetError());
        return errorAt(-1);
    }

    const size_t sizeOfFile = (size_t)SDL_GetIOSize(chartRWops);
    const int length = sizeOfFile;

    char* chartFile = SDL_calloc(sizeOfFile + 4, 1); //i think +4 is fine
    SDL_ReadIO(chartRWops, chartFile, sizeOfFile);
    if_unlikely(!SDL_CloseIO(chartRWops)){
        setError("Closing chart file handler failed.\nError: %s", SDL_GetError());
        return errorAt(-1);
    }

    if_unlikely(startIndex <= 0 || startIndex >= length || chartFile[startIndex - 1] != '='){
        setError("Chart modified since last load.\n");
        return errorAt(-1);
    }

    //makes loading slower but needed for protection
    char chartChecksum[65] = {0};
    sha256_hex(chartFile, sizeOfFile, chartChecksum);
    if_unlikely(SDL_strncmp(chartHash, chartChecksum, SHA256_HEX_SIZE) != 0){
        setError("Chart modified since last load.\n");
        return errorAt(-1);
    }

    //a hack for broken charts with one difficulty to work
    chartFile[sizeOfFile] = 'E'; //I can't believe it


    chartLength = 0;


    double beatsPerMinute = -1;
    double bpmDivisor = -1;
    double oldTotalInternalTime = 0;
    double lengthPerComma = 0;
    int lengthPerCommaMultiplier = 0;
    #define resetLengthPerComma() {oldTotalInternalTime += lengthPerComma * lengthPerCommaMultiplier; lengthPerCommaMultiplier = 0; lengthPerComma = 240.0 / (beatsPerMinute * bpmDivisor);}
    #define internalTime (oldTotalInternalTime + lengthPerComma * lengthPerCommaMultiplier) * gameTicksPerSecond

    int noteLocation = 0;
    lenOfSlidePoints = 0;

    //https://rentry.co/maiguide#making-the-chart
    //https://w.atwiki.jp/simai/pages/1003.html#id_b1b44348

    int stage = 0;
    int oldStage = -1;
    bool isBreak = false;
    bool isEXNote = false;
    bool isTouchFireworks = false;
    bool isSlideBreak = false;
    bool isEachNote = false;
    bool skipSlideTapNote = false;
    bool shouldStarSuddenlyAppear = false;
    int touchGroup = -1;
    int utageTapSkin = -1;
    int currentNoteID = 0;

    const int tapNoteSpinningStarSkin = 1; //thing before slide + a utage thing
    const int tapNoteStaticStarSkin = 2;

    //slide static moment
    bool isMultiSlide = false;
    bool slideHasGrandV = false;
    int numberOfSlidesAtTheSameTime = 0;
    int lastLocation = 0;
    double* slideTimings = NULL;
    int lenOfSlideTimings = 0;

    for(int n = startIndex; n <= length; n++) {
        //printf("%c", chartFile[n]);
        const char symbol = chartFile[n];
        switch(stage) {
            case scanning: //scanning mode
                isBreak = false;
                isEXNote = false;
                isSlideBreak = false;
                isTouchFireworks = false;
                utageTapSkin = -1;

                if(SDL_isdigit(symbol)){
                    noteLocation = symbol - '0';
                    const int maxNoteLocation = touchGroup == 2 ? 2 : 8;
                    if_unlikely(!(noteLocation == 0 && touchGroup == -1) && (noteLocation < 1 || noteLocation > maxNoteLocation)){
                        setError("Invalid note location (%d).\nLocation: %d", noteLocation, n);
                        return errorAt(n);
                    }

                    if(touchGroup == 2){ //C note moment
                        noteLocation = 1;
                    } else if(noteLocation != 0){
                        if(touchGroup < 2){
                            noteLocation = applyMirrorToButton(noteLocation);
                        } else {
                            noteLocation = applyMirrorToSensorDE(noteLocation);
                        }
                    }
                    stage = notes;
                    break;
                } else if(touchGroup >= 0){
                    if_likely(touchGroup == 2){ //note location is optional for C touch sensor
                        noteLocation = 1;
                        stage = notes;
                        n--;
                        break;
                    }
                    setError("Touch other than C not immediately followed by sensor number.\nLocation: %d", n);
                    return errorAt(n);
                } else if(isspace(symbol)){ //whitespace is allowed
                    break;
                }

                switch(symbol){
                    case '(':
                        stage = BPMFinder;
                        break;
                    case '{':
                        //todo: should {#1} be supported without requiring a bpm beforehand?
                        //if_likely(beatsPerMinute >= 0 || (n < length && chartFile[n + 1] == '#')){
                        if_likely(beatsPerMinute > 0){
                            stage = BPMDivisor;
                        } else {
                            setError("BPM not found before finding BPM divisor.\nLocation of { with invalid order: %d", n);
                            return errorAt(n);
                        }
                        break;
                    case '|':
                        oldStage = stage;
                        stage = comment;
                        break;
                    case 'A':
                    case 'B':
                    case 'C':
                    case 'D':
                        touchGroup = symbol - 'A';
                        break;
                    case 'E': //special case because it can either mean end of chart or touch note in E group
                    case '&': //make "broken" charts work
                        if(chartFile[n] == '&' || n >= length - 1 || isspace(chartFile[n + 1])){ //end of chart
                            SDL_free(chartFile);
                            SDL_free(slidePoints);
                            SDL_free(slideTimings);
                            slidePoints = NULL;
                            setChartFeatures();
                            calculateMaxScores();
                            normalizeSlideTimes();
                            applyTouchGroupSpread();
                            addSfxToNotes();
                            return errorAtWithLength(0, 0); //not error
                        } else if(SDL_isdigit(chartFile[n + 1])){ //touch
                            touchGroup = symbol - 'A';
                            break;
                        } else {
                            setError("Invalid symbol after letter E.");
                            return errorAt(n);
                        }
                        break;
                    case ',':
                        lengthPerCommaMultiplier++;
                        break;
                    default:
                        setError("Invalid char at %d.", n);
                        return errorAt(n);
                }
                break;
            case comment:;
                static bool isInComment = false;
                if(isInComment) {
                    if(symbol == '\n') {
                        isInComment = false;
                        stage = oldStage;
                    }
                } else if(symbol == '|') {
                    isInComment = true;
                } else {
                    isInComment = false;
                    setError("Invalid comment at char %d\n", n);
                    return errorAt(n);
                }
                break;

            case BPMFinder:; //Beat per minute find mode
                static char* BPMValueStart = 0;
                if(symbol == ')') { //bpm value found, switch to stage 0
                    if_unlikely(BPMValueStart == 0){
                        setError("No number for BPM is provided.\nLocation: %d", n);
                        break;
                    }

                    bool isValid = false;
                    char *BPMEnd = NULL;
                    beatsPerMinute = safeStrtod(BPMValueStart, &BPMEnd, &isValid);
                    if_unlikely(*BPMEnd != ')' || !isValid){
                        setError("BPM is not a valid number.\nWrong char: %c\nLocation: %d", *BPMEnd, n);
                        return errorAt(n);
                    } else if_unlikely(beatsPerMinute <= 0){
                        setError("Invalid BPM (%.4f).\nLocation: %d", beatsPerMinute, n);
                        return errorAt(n);
                    } else if(bpmDivisor > 0){
                        resetLengthPerComma(); //more bugs
                    }

                    BPMValueStart = 0;
                    stage = scanning;
                } else if(BPMValueStart == 0){ //add error when not number
                    BPMValueStart = chartFile + n;
                }
                break;
            case BPMDivisor:;// beat per minute DIVISOR mode
                static char* BPMDivisorStart;
                static bool isSeconds = false;
                if(symbol == '}') { //bpm divisor found, switch to stage 0
                    if_unlikely(BPMDivisorStart == 0){
                        setError("No number for the BPM Divisor is provided.\nLocation: %d", n);
                        return errorAt(n);
                    }

                    char *BPMDivisorEnd = NULL;
                    bool isValid = false;
                    const double result = safeStrtod(BPMDivisorStart, &BPMDivisorEnd, &isValid);

                    if_unlikely(*BPMDivisorEnd != '}' || !isValid){
                        setError("BPM Divisor is not a number.\nWrong char: %c\nLocation: %d", *BPMDivisorEnd, n);
                        return errorAt(n);
                    } else if_unlikely(result <= 0){
                        setError("Invalid BPM Divisor (%.4f).", bpmDivisor);
                        return errorAt(n);
                    }

                    BPMDivisorStart = 0;

                    if(isSeconds){
                        bpmDivisor = -1;
                        isSeconds = false;
                        if(lengthPerComma != result){ //prevents unnecessary resets
                            resetLengthPerComma(); //hack
                            lengthPerComma = result;
                        }
                    } else if(bpmDivisor != result){ //prevents unnecessary resets
                        bpmDivisor = result;
                        resetLengthPerComma();
                        #undef resetLengthPerComma
                    }
                    stage = scanning;
                } else if(BPMDivisorStart == 0){
                    if(symbol == '#'){
                        isSeconds = true;
                        BPMDivisorStart = chartFile + n + 1;
                    } else if_unlikely(beatsPerMinute < 0){
                        setError("BPM not found before finding BPM divisor.\nLocation of { with invalid order: %d", n);
                        return errorAt(n);
                    } else {
                        BPMDivisorStart = chartFile + n;
                    }
                }
                break;
            case PCheck: //check if it is either p or pp
            case QCheck: //check if it is either q or qq
        //weirdest bug
        #define addSlidePointWithType(slideTypeInput) {addSlidePoint(lenOfSlidePoints == 0 ? noteLocation : lastLocation, slideTypeInput); stage = slidenote;}
                if(SDL_isdigit(chartFile[n])){
                    n--; //offset
                    addSlidePointWithType(stage == PCheck ? slideTypePShape :
                                          stage == QCheck ? slideTypeQShape :
                                          slideTypeStraightLine);
                } else {
                    if(stage == PCheck && chartFile[n] == 'p'){
                        addSlidePointWithType(slideTypePPShape);
                    } else if(stage == QCheck && chartFile[n] == 'q'){
                        addSlidePointWithType(slideTypeQQShape);
                    } else {
                        setError("Invalid pp/qq char (%c).\nLocation: %d", chartFile[n], n);
                        return errorAt(n);
                    }
                }

                break;
            case ReflectiveCheck:
                if_unlikely(!SDL_isdigit(chartFile[n])){
                    setError("Invalid reflective (V) point value\nexpected: a number\nreceived: %c\nLocation: %d", chartFile[n], n);
                    return errorAt(n);
                }

                const int reflectiveLocation = applyMirrorToButton(chartFile[n] - '0');
                const int reflectiveDist = getDistBetweenButton(noteLocation, reflectiveLocation);
                if_unlikely(reflectiveDist != 2){
                    setError("Invalid reflective (V) dist between first and second point\nexpected: 2\nreceived: %u\nLocation: %d", reflectiveDist, n);
                    return errorAt(n);
                }


                addSlidePoint(noteLocation, shortestSlideDirection(noteLocation, reflectiveLocation) == -1 ?
                                            slideTypeReflectiveShapeCCW : //example: 1V72
                                            slideTypeReflectiveShapeCW);  //example: 1V35

                stage = slidenote;
                slideHasGrandV = true;
                break;
            case notes:{ //tap or break or hold or slide
                const bool isNumber = SDL_isdigit(symbol);
                const bool isSlash = symbol == '/';
                const bool isBacktick = symbol == '`';
                if(isNumber || isSlash || isBacktick || symbol == ',') { //end
                    if(isNumber || isSlash){
                        isEachNote = true;
                    }

                    if(noteLocation != 0){
                        reallocChartNotesBy(1);
                        const int realLocation = touchGroup >= 0 ? touchGroup * 8 + (noteLocation - 1) : noteLocation;
                        setupGenericNote(chartLength, realLocation, internalTime, isEachNote, isBreak, isEXNote);
                        if(touchGroup >= 0){
                            setupTouchNote(chartLength, currentNoteID, isTouchFireworks);
                            isTouchFireworks = false;
                            touchGroup = -1;
                        } else {
                            setupTapNote(chartLength, utageTapSkin >= 0 ? utageTapSkin : tapNoteNormalSkin, 360.0);
                        }
                        isBreak = false;
                        isEXNote = false;
                        chartLength++;
                    }

                    if(isNumber){
                        noteLocation = symbol - '0';
                        if_unlikely(noteLocation < 0 || noteLocation > 8){
                            setError("Invalid note location (%d).\nLocation: %d", noteLocation, n);
                            return errorAt(n);
                        } else if(noteLocation != 0){
                            noteLocation = applyMirrorToButton(symbol - '0');
                        }
                        break;
                    }

                    stage = scanning;
                    if(!isSlash){
                        if(!isBacktick){ //might not be intended
                            lengthPerCommaMultiplier++;
                        }
                        isEachNote = false;
                        currentNoteID++;
                    }
                    break;
                }

                if_unlikely(noteLocation == 0){
                    setError("Note 0 not immediately terminated.\nLocation: %d", n);
                    return errorAt(n);
                }

                switch(symbol){
                    case 'b':
                        if_unlikely(isBreak){
                            setError("More than one break opcode specified in a note.\nLocation: %d", n);
                            return errorAt(n);
                        } else {
                            isBreak = true;
                        }
                        break;
                    case 'x':
                        if_unlikely(isEXNote){
                            setError("More than one EX opcode specified in a note.\nLocation: %d", n);
                            return errorAt(n);
                        } else {
                            isEXNote = true;
                        }
                        break;
                    case 'f':
                        if_unlikely(isTouchFireworks){
                            setError("Fireworks specified twice in same touch (hold) note.\nLocation: %d", n);
                            return errorAt(n);
                        } else if_unlikely(touchGroup < 0){
                            setError("Fireworks specified in a note that isn't a touch (hold) note.\nLocation: %d", n);
                            return errorAt(n);
                        }
                        isTouchFireworks = true;
                        break;
                    case '$': {
                        static int oldN = 0;
                        if(utageTapSkin < 0){
                            utageTapSkin = tapNoteStaticStarSkin;
                            oldN = n;
                            break;
                        }

                        if_likely(n - 1 == oldN){
                            oldN = -1; //make third $ invalid
                            utageTapSkin = tapNoteSpinningStarSkin;
                            break;
                        }

                        if(oldN < 0){
                            setError("Too many $ detected.\nLocation: %d", n);
                        } else {
                            setError("Secondary $ not stacked.\nLocation: %d", n);
                        }
                        return errorAt(n);
                    }
                        break;
                    case '@':
                        if_likely(utageTapSkin < 0){
                            utageTapSkin = tapNoteNormalSkin;
                            break;
                        }

                        if(utageTapSkin == tapNoteNormalSkin){
                            setError("@ operation used twice in the same TAP note.\nLocation: %d", n);
                        } else {
                            setError("@ and $ operations used in the same TAP note.\nLocation: %d", n);
                        }
                        return errorAt(n);
                    case '?':
                        if_likely(!skipSlideTapNote){
                            skipSlideTapNote = true;
                            break;
                        }

                        if(shouldStarSuddenlyAppear){
                            setError("? and ! opcodes both used.\nLocation: %d", n);
                        } else {
                            setError("Too many ? opcodes.\nLocation: %d", n);
                        }
                        return errorAt(n);
                    case '!':
                        if_likely(!skipSlideTapNote){
                            skipSlideTapNote = true;
                            shouldStarSuddenlyAppear = true;
                            break;
                        }

                        if(shouldStarSuddenlyAppear){
                            setError("Too many ! opcodes.\nLocation: %d", n);
                        } else {
                            setError("? and ! opcodes both used.\nLocation: %d", n);
                        }
                        return errorAt(n);
                    case 'h':
                        stage = holdnote;
                        break;
                    case '-':
                        addSlidePointWithType(slideTypeStraightLine);
                        break;
                    case '^':
                        addSlidePointWithType(slideTypeAutoArc);
                        break;
                    case '<':
                        addSlidePointWithType(slideTypeArcLeft);
                        break;
                    case '>':
                        addSlidePointWithType(slideTypeArcRight);
                        break;
                    case 'v':
                        addSlidePointWithType(slideTypeVShape);
                        break;
                    case 's':
                        addSlidePointWithType(slideTypeSShape);
                        break;
                    case 'z':
                        addSlidePointWithType(slideTypeZShape);
                        break;
                    case 'p':
                        stage = PCheck;
                        break;
                    case 'q':
                        stage = QCheck;
                        break;
                    case 'V':
                        stage = ReflectiveCheck;
                        break;
                    case 'w':
                        if_unlikely(lenOfSlidePoints > 0){
                            setError("Fan slide is not exclusive.\nLocation: %d", n);
                            return errorAt(n);
                        }
                        stage = fanslidenote; //pain
                        break;
                    default:
                        //random bug fix with new lines
                        if_unlikely((isBreak || isEXNote || isTouchFireworks || skipSlideTapNote || shouldStarSuddenlyAppear || utageTapSkin >= 0) || !SDL_isspace(chartFile[n])){
                            setError("Invalid char (%c).\nLocation: %d", chartFile[n], n);
                            return errorAt(n);
                        }

                }
            }
            break;

            case holdnote:{ //hold note
                static int holdNumUsed = 0;
                static char* holdNumbersStart[3] = {0};
                static bool hasGoneInsideSquareBrackets = false;
                static bool holdIsGettingDuration = false;

                if(holdIsGettingDuration){
                    if(symbol == ']'){
                        if_likely(hasGoneInsideSquareBrackets){ //
                            holdIsGettingDuration = false;
                        } else { //no number inputted
                            setError("Square brackets duration input has no value.\nLocation: %d", n);
                            return errorAt(n);
                        }
                    } else if(holdNumbersStart[0] == 0){
                        if(chartFile[n] == '#'){ //custom seconds
                            holdNumbersStart[0] = chartFile + n + 1;
                        } else {
                            holdNumbersStart[0] = chartFile + n;
                        }
                    } else if(chartFile[n] == '#' || chartFile[n] == ':'){ //custom bpm
                        holdNumUsed++;
                    } else if(holdNumbersStart[holdNumUsed] == 0){
                        holdNumbersStart[holdNumUsed] = chartFile + n;
                    }

                    //sneaky hack
                    hasGoneInsideSquareBrackets = true;
                    break;
                }

                switch(symbol){
                    case 'b':
                        if_unlikely(isBreak){
                            setError("More than one break opcode specified in a note.\nLocation: %d", n);
                            return errorAt(n);
                        } else {
                            isBreak = true;
                        }
                        break;
                    case 'x':
                        if_unlikely(isEXNote){ //random break bug
                            setError("More than one EX opcode specified in a note.\nLocation: %d", n);
                            return errorAt(n);
                        } else {
                            isEXNote = true;
                        }
                        break;
                    case 'f':
                        if_unlikely(isTouchFireworks){
                            setError("Fireworks specified twice in same touch (hold) note.\nLocation: %d", n);
                            return errorAt(n);
                        } else if_unlikely(touchGroup < 0){
                            setError("Fireworks specified in a note that isn't a touch (hold) note.\nLocation: %d", n);
                            return errorAt(n);
                        }
                        isTouchFireworks = true;
                        break;
                    case ',':
                    case '/':
                    case '`':;
                        const bool isSlash = symbol == '/';
                        const bool isBacktick = symbol == '`';
                        if(isSlash){
                            isEachNote = true;
                        }

                        reallocChartNotesBy(1);

                        double beatDivisor = 1280;
                        double beatDivisorMultiplier = 1;
                        double endLength = -1;
                        double holdBeatsPerMinute = beatsPerMinute;
                        char* endLengthEnd = NULL;
                        char* holdBeatsPerMinuteEnd = NULL;
                        char* beatDivisorEnd = NULL;
                        char* beatDivisorMultiplierEnd = NULL;
                        if(hasGoneInsideSquareBrackets){
                            switch(holdNumUsed){
                                case 0: // #secs
                                    endLength = SDL_strtod(holdNumbersStart[0], &endLengthEnd);
                                    break;
                                case 2: // bpm#ratio1:ratio2
                                    holdBeatsPerMinute = SDL_strtod(holdNumbersStart[0], &holdBeatsPerMinuteEnd);
                                    beatDivisor = SDL_strtod(holdNumbersStart[holdNumUsed - 1], &beatDivisorEnd);
                                    beatDivisorMultiplier = SDL_strtod(holdNumbersStart[holdNumUsed], &beatDivisorMultiplierEnd);
                                    break;
                                case 1: // ratio1:ratio2
                                    beatDivisor = SDL_strtod(holdNumbersStart[holdNumUsed - 1], &beatDivisorEnd);
                                    beatDivisorMultiplier = SDL_strtod(holdNumbersStart[holdNumUsed], &beatDivisorMultiplierEnd);
                                    break;
                                default:
                                    break;

                            }
                        } else {
                            beatDivisor = 1280.0;
                            beatDivisorMultiplier = 1;
                        }

                        if(endLengthEnd != NULL){
                            if_unlikely(*endLengthEnd != ']'){
                                setError("Hold end length is not a number.\nWrong char: %c\nLocation: %d", *endLengthEnd, n);
                                return errorAt(n);
                            } else if_unlikely(endLength < 0){
                                setError("Hold end length is negative (%.2f).\nLocation: %d", endLength, n);
                                return errorAt(n);
                            }
                        }

                        if(holdBeatsPerMinuteEnd != NULL){
                            if_unlikely(*holdBeatsPerMinuteEnd != '#'){
                                setError("Hold BPM is not a number.\nWrong char: %c\nLocation: %d", *holdBeatsPerMinuteEnd, n);
                                return errorAt(n);
                            } else if_unlikely(holdBeatsPerMinute <= 0){
                                setError("Hold BPM is negative or zero (%.2f).\nLocation: %d", holdBeatsPerMinute, n);
                                return errorAt(n);
                            }

                        }

                        if(beatDivisorEnd != NULL){
                            if_unlikely(*beatDivisorEnd != ':'){
                                setError("Hold beat divisor is not a number.\nWrong char: %c\nLocation: %d", *beatDivisorEnd, n);
                                return errorAt(n);
                            } else if_unlikely(beatDivisor <= 0){
                                setError("Hold beat divisor is negative or zero (%.2f).\nLocation: %d", beatDivisor, n);
                                return errorAt(n);
                            }
                        }

                        if(beatDivisorMultiplierEnd != NULL){
                            if_unlikely(*beatDivisorMultiplierEnd != ']'){
                                setError("Beat Divisor Multiplier is not a number.\nWrong char: %c\nLocation: %d", *beatDivisorMultiplierEnd, n);
                                return errorAt(n);
                            } else if_unlikely(beatDivisorMultiplier < 0){
                                setError("Beat Divisor Multiplier is negative.\nLocation: %d", n);
                                return errorAt(n);
                            }

                        }

                        if(endLength < 0){
                            endLength = 240.0 / (holdBeatsPerMinute * beatDivisor) * beatDivisorMultiplier;
                        }

                        const int realLocation = touchGroup >= 0 ? touchGroup * 8 + (noteLocation - 1) : noteLocation;
                        setupGenericNote(chartLength, realLocation, internalTime, isEachNote, isBreak, isEXNote);
                        if(touchGroup >= 0){
                            setupTouchHoldNote(chartLength, currentNoteID, endLength * gameTicksPerSecond, isTouchFireworks);
                            isTouchFireworks = false;
                            touchGroup = -1;
                        } else {
                            setupHoldNote(chartLength, endLength * gameTicksPerSecond);
                        }

                        isBreak = false;
                        isEXNote = false;

                        holdNumUsed = 0;
                        holdNumbersStart[0] = 0;
                        holdNumbersStart[1] = 0;
                        holdNumbersStart[2] = 0;
                        chartLength++;

                        stage = scanning;
                        hasGoneInsideSquareBrackets = false;
                        if(!isSlash){
                            if(!isBacktick){ //might not be intended
                                lengthPerCommaMultiplier++;
                            }
                            isEachNote = false;
                            currentNoteID++;
                        }
                        break;
                    case '[':
                        if_unlikely(hasGoneInsideSquareBrackets){
                            setError("Square brackets appears twice.\nLocation: %d", n);
                            return errorAt(n);
                        }
                        holdIsGettingDuration = true;
                        break;
                    default:
                        //?
                        break;
                }
            }
            break;
            case slidenote:{ //slide notes OH GOD
                static bool isGettingDuration = false;
                static bool hasObtainedDuration = false;
                static double delayLength = 0.0;

                static int slideInputType = normalSlideRatio;
                static int slideNumUsed = 0;
                static char* slideNumbersStart[4] = {0};

                if(isGettingDuration){
                    if(symbol == ']'){
                        isGettingDuration = false;
                        hasObtainedDuration = true;
                        if(slideTimings == NULL || (lenOfSlideTimings % 16 == 0 && lenOfSlideTimings > 0)){
                            slideTimings = (double*)SDL_realloc(slideTimings, sizeof(double) * (16 + lenOfSlideTimings));
                        }

                        double slideBeatsPerMinute = beatsPerMinute;
                        double beatDivisor = 1280;
                        double beatDivisorMultiplier = 1;
                        double endLength = -1;
                        double newDelayLength = -1;

                        chartErrorRange slideErrorCheck = getSlideDelayAndEndDuration(&slideInputType, &slideNumUsed, slideNumbersStart,
                                                                                        &newDelayLength, &slideBeatsPerMinute, &endLength,
                                                                                        &beatDivisor, &beatDivisorMultiplier, n);

                        if(slideErrorCheck.length > 0){
                            return slideErrorCheck;
                        }

                        if(newDelayLength < 0){
                            newDelayLength = 60.0 / slideBeatsPerMinute;
                        }

                        if(endLength < 0){
                            endLength = 240.0 / (slideBeatsPerMinute * beatDivisor) * beatDivisorMultiplier;
                        }

                        if(lenOfSlideTimings <= 0){ //is this accurate?
                            delayLength = newDelayLength;
                        }

                        slideTimings[lenOfSlideTimings] = endLength;

                        lenOfSlideTimings++;

                        slideInputType = normalSlideRatio;
                        slideNumUsed = 0;
                        slideNumbersStart[0] = 0;
                        slideNumbersStart[1] = 0;
                        slideNumbersStart[2] = 0;
                        slideNumbersStart[3] = 0;
                        break;
                    } else if(slideNumbersStart[0] == 0){
                        slideNumbersStart[0] = chartFile + n + (symbol == '#');
                    }

                    if(symbol == '#'){
                        if(chartFile[n + 1] == '#'){ //custom waiting time
                            slideInputType = customWaitingTimeAndSlideDuration; //prototype
                            n++;
                        } else { //custom bpm
                            if(slideInputType == customWaitingTimeAndSlideDuration){
                                slideInputType = customWaitingTimeAndBPMNormalSlideRatio;
                            } else if(slideInputType == normalSlideRatio){
                                slideInputType = customSlideDuration;
                            } else {
                                slideInputType = customBPMAndSlideDuration;
                            }
                        }
                        if(chartFile[n - 1] != '['){
                            slideNumUsed++;
                        }
                    } else if(symbol == ':'){
                        slideNumUsed++;
                        if(slideInputType == customBPMAndSlideDuration || slideInputType == customSlideDuration){
                            slideInputType = customBPMNormalSlideRatio;
                        } else if(slideInputType == customWaitingTimeAndSlideDuration){
                            slideInputType = customWaitingTimeNormalSlideRatio;
                        }
                    } else if(slideNumbersStart[slideNumUsed] == 0){
                        slideNumbersStart[slideNumUsed] = chartFile + n;
                    }
                    break;
                } else if(SDL_isdigit(symbol)){
                    lastLocation = applyMirrorToButton(symbol - '0');
                    break;
                }

                int slideType = slideTypeEnd;
                switch(symbol){
                    case '[':
                        isGettingDuration = true;
                        break;
                    case 'b':
                        isSlideBreak = true;
                        break;
                    case '-':
                        slideType = slideTypeStraightLine;
                        break;
                    case '^':
                        slideType = slideTypeAutoArc;
                        break;
                    case '<':
                        slideType = slideTypeArcLeft;
                        break;
                    case '>':
                        slideType = slideTypeArcRight;
                        break;
                    case 'v':
                        slideType = slideTypeVShape;
                        break;
                    case 's':
                        slideType = slideTypeSShape;
                        break;
                    case 'z':
                        slideType = slideTypeZShape;
                        break;
                    case 'p':
                        if(n + 1 >= length){
                            break;
                        }

                        if(chartFile[n + 1] == 'p'){
                            slideType = slideTypePPShape;
                            n++;
                        } else {
                            slideType = slideTypePShape;
                        }
                        break;
                    case 'q':
                        if(n + 1 >= length){
                            break;
                        }

                        if(chartFile[n + 1] == 'q'){
                            slideType = slideTypeQQShape;
                            n++;
                        } else {
                            slideType = slideTypeQShape;
                        }
                        break;
                    case 'V':
                        if(n + 1 >= length){
                            break;
                        }

                        const int secondaryNoteLocation = applyMirrorToButton(chartFile[n + 1] - '0');
                        const int dist = getDistBetweenButton(lastLocation, secondaryNoteLocation);
                        if_unlikely(dist != 2){
                            setError("Invalid reflective (V) dist between first and second point\nexpected: 2\nreceived: %u\nLocation: %d", dist, n);
                            return errorAt(n);
                        }

                        addSlidePoint(lastLocation, shortestSlideDirection(lastLocation, secondaryNoteLocation) == -1 ?
                                            slideTypeReflectiveShapeCCW : //example: 1V72
                                            slideTypeReflectiveShapeCW);  //example: 1V35

                        slideHasGrandV = true;
                        n++; //what
                        break;
                    //fan slide is exclusive apparently
                    case 'w':
                        if_unlikely(lenOfSlidePoints > 0){
                            setError("Fan slide is not exclusive.\nLocation: %d", n);
                            return errorAt(n);
                        }
#undef addSlidePointWithType
                        stage = fanslidenote; //pain
                        break;
                    case ',':
                    case '/':
                    case '`':
                    case '*':
                        if_unlikely(!hasObtainedDuration){
                            setError("Slide duration missing.\nLocation: %d", n);
                            return errorAt(n);
                        }

                        hasObtainedDuration = false;
                        numberOfSlidesAtTheSameTime++;

                        //this is a joke
                        const bool isSlash = chartFile[n] == '/';
                        const bool isAsterisk = chartFile[n] == '*';
                        const bool isBacktick = chartFile[n] == '`';
                        if(isSlash){
                            isEachNote = true;
                        } else if(isAsterisk){
                            isMultiSlide = true;
                        } else if(numberOfSlidesAtTheSameTime == 2){
                            //previous slide wasn't marked as each
                            if_unlikely(!markPreviousSlideAsEach()){
                                setError("Failed to mark previous slide as each.\nLocation: %d", n);
                                return errorAt(n);
                            }
                        }

                        double endLength = slideTimings[0];
                        for(int i = 1; i < lenOfSlideTimings; i++){
                            endLength += slideTimings[i];
                        }

                        //step one: add tap note (except in utage or multi slides oh god)

                        reallocChartNotesBy(2 - skipSlideTapNote);

                        if(!skipSlideTapNote){
                            //rotation speed apparently dependent on slide size
                            /*double rotationSpeed = 360.0;
                            if(utageTapSkin == tapNoteSpinningStarSkin){ //utage
                                rotationSpeed = 360.0; //place holder rotate speed
                            } else if(chartNotes[chartLength].info.tap.tapNoteSkin >= tapNoteSpinningStarSkin){ //normal
                                rotationSpeed = 360.0 / SDL_max(1.0 / 3.0, endLength); //div by 0 protection
                            }*/
                            setupGenericNote(chartLength, noteLocation, internalTime, isEachNote, isBreak, isEXNote);
                            setupTapNote(chartLength, utageTapSkin < 0 && isMultiSlide ? tapNoteMultiStarSkin : tapNoteStarSkin, 0.0);
                            isBreak = false;
                            isEXNote = false;
                            chartLength++;
                        }

                        //step two: the slide
                        //i will choke on this
                        setupGenericNote(chartLength, noteLocation, internalTime, isMultiSlide || numberOfSlidesAtTheSameTime >= 2, isSlideBreak, false);
                        setupSlideNote(chartLength, lastLocation,
                                       delayLength * gameTicksPerSecond, endLength * gameTicksPerSecond,
                                       slideHasGrandV && lenOfSlidePoints == 2, shouldStarSuddenlyAppear);

                        slideHasGrandV = false;
                        shouldStarSuddenlyAppear = false;
                        isSlideBreak = false;

                        //final boss
                        int oldLenOfArrows = 0;
                        double oldDist = 0.0;
                        double maxDist = 0.0;
                        for(int i = 0; i < lenOfSlidePoints; i++) {
                            oldLenOfArrows = chartNotes[chartLength].info.slide.lenOfArrows;
                            const int endLocation = i == lenOfSlidePoints - 1 ? lastLocation : slidePoints[i + 1].location;

                            //convert certain slide types to different types depending on their conditions and error checking also
                            switch(slidePoints[i].slideType) {
                                case slideTypeStraightLine:
                                    if_unlikely(getDistBetweenButton(slidePoints[i].location, endLocation) <= 1){
                                        setError("Straight line (-) distance from start to end is too short.\n(has to be at least 2)\nLocation: %d", n);
                                        return errorAt(n);
                                    }
                                    break;
                                case slideTypeAutoArc:
                                    if_unlikely(shortestSlideDirection(slidePoints[i].location, endLocation) == 0){
                                        setError("Both directions in Auto-Arc is optimal.\nStart: %d\nEnd: %d\nLocation: %d", slidePoints[i].location, endLocation, n);
                                        return errorAt(n);
                                    }
                                    break;
                                case slideTypeVShape:
                                    if(shortestSlideDirection(slidePoints[i].location, endLocation) == 0) {
                                        slidePoints[i].location = slideTypeStraightLine; //same
                                    }
                                    break;
                                case slideTypeSShape:
                                case slideTypeZShape:
                                    if_unlikely(shortestSlideDirection(slidePoints[i].location, endLocation) != 0){
                                        setError("Invalid S/Z (zigzag) start (%d) and end (%d).\nLocation: %d", slidePoints[i].location, endLocation, n);
                                        return errorAt(n);
                                    }
                                    break;
                                //reflective check done in interpret stage
                                default:
                                    break;
                            }

                            switch(slidePoints[i].slideType) {
                                case slideTypeStraightLine:
                                    drawSlideLine(slidePoints[i].location, endLocation);
                                    break;
                                case slideTypeArcLeft:
                                case slideTypeArcRight:
                                case slideTypeAutoArc:
                                    //pure confusion for literally no reason
                                    if(slidePoints[i].slideType == slideTypeAutoArc){
                                        drawSlideArc(slidePoints[i].location, endLocation,
                                                     shortestSlideDirection(slidePoints[i].location, endLocation) == 1);
                                        break;
                                    }

                                    //welcome to absolute nonsense that shouldn't even exist
                                    bool clockwise = false;
                                    const bool arcIsLeft = slidePoints[i].slideType == slideTypeArcLeft;
                                    if (slidePoints[i].location >= 3 && slidePoints[i].location <= 6) {
                                        clockwise = arcIsLeft;
                                    } else {
                                        clockwise = !arcIsLeft;
                                    }
                                    drawSlideArc(slidePoints[i].location, endLocation, clockwise);
                                    break;
                                case slideTypeVShape:
                                    drawSlideV(slidePoints[i].location, endLocation);
                                    break;
                                case slideTypeSShape:
                                case slideTypeZShape:
                                    //living hell saved by one thing
                                    drawSlideZigzag(slidePoints[i].location, slidePoints[i].slideType == slideTypeSShape);
                                    break;
                                case slideTypePShape:
                                case slideTypeQShape:
                                    drawSlidePQ(slidePoints[i].location, endLocation, slidePoints[i].slideType == slideTypeQShape);
                                    break;
                                case slideTypePPShape:
                                case slideTypeQQShape:
                                    drawSlidePPQQ(slidePoints[i].location, endLocation, slidePoints[i].slideType == slideTypePPShape);
                                    break;
                                case slideTypeReflectiveShapeCCW:
                                case slideTypeReflectiveShapeCW:
                                    drawSlideReflective(slidePoints[i].location, endLocation, slidePoints[i].slideType == slideTypeReflectiveShapeCCW);
                                    break;
                                case slideTypeFan:
                                    setError("uh. what? wrong stage for fan slides\nLocation of last slide: %d", n);
                                    break;
                                default:
                                    printf("Invalid slide type: %d\n", slidePoints[i].slideType);
                            }

                            const int lenOfArrows = chartNotes[chartLength].info.slide.lenOfArrows;
                            maxDist = chartNotes[chartLength].info.slide.arrows[lenOfArrows - 1].arrowTime;

                            if(lenOfSlideTimings < 2){
                                continue;
                            }

                            //apply speed changes
                            const double slideTimeMultiplier = slideTimings[i];
                            for(int j = oldLenOfArrows; j < lenOfArrows - 1; j++){
                                chartNotes[chartLength].info.slide.arrows[j].arrowTime = oldDist + ((chartNotes[chartLength].info.slide.arrows[j].arrowTime - oldDist) /
                                                                                                    (maxDist - oldDist)) * slideTimeMultiplier;
                            }
                            oldDist += slideTimeMultiplier;
                            chartNotes[chartLength].info.slide.arrows[lenOfArrows - 1].arrowTime = oldDist;
                        }

                        if(!skipSlideTapNote ){ //true math formula??
                            chartNotes[chartLength - 1].info.tap.starNoteRotationSpeed = 360.0 / SDL_max(1.0 / 3.0, endLength / maxDist * 1.26);
                        }
                        skipSlideTapNote = false;


                        //get buttons
                        //const int lenOfArrows = chartNotes[chartLength].info.slide.lenOfArrows;

                        //time for judgement stuff that i have no idea if it is correct or not
                        //last zone table is different, convert
                        const int lastButtonStart = slidePoints[lenOfSlidePoints - 1].location;
                        const int lastButtonEnd = lastLocation;
                        const int lastZoneSlide = convertSlideType(slidePoints[lenOfSlidePoints - 1].slideType, lastButtonStart, lastButtonEnd);
                        //step 2: get distance
                        const int lastZoneDistance = getSlideDistBetweenTwoButtons(lastButtonStart, lastButtonEnd);
                        //step 3: get data

                        //last zone table taken from https://listed.to/@donmai/44545/how-maimai-dx-judges-slides

                        const double lastZoneDistanceTable[11][8] = {

                        // -1 means invalid
                        //                 dist 0/8 dist 1  dist 2  dist 3  dist 4  dist 5  dist 6  dist 7
                            /* - */        {    -1,     -1, 0.1920, 0.1793, 0.1629, 0.1793, 0.1920,     -1},
                            /* Arc CW  */  {0.0582, 0.4653, 0.2326, 0.1551, 0.1163, 0.0931, 0.0775, 0.0665},
                            /* Arc CCW */  {0.0582, 0.0665, 0.0775, 0.0931, 0.1163, 0.1551, 0.2326, 0.4653},
                            /* p */        {0.0921, 0.1114, 0.1247, 0.1436, 0.1693, 0.0752, 0.0817, 0.0840},
                            /* q */        {0.0921, 0.0840, 0.0817, 0.0752, 0.1693, 0.1436, 0.1247, 0.1114},
                            /* Zigzag  */  {    -1,     -1,     -1,     -1, 0.1055,     -1,     -1,     -1},
                            /* v */        {0.1629, 0.1629, 0.1629, 0.1629,     -1, 0.1629, 0.1629, 0.1629}, //note: dist 0/8 is originally blank but added as extension
                            /* pp */       {0.0734, 0.0872, 0.1509, 0.0698, 0.0698, 0.0711, 0.0811, 0.0603},
                            /* qq */       {0.0734, 0.0603, 0.0811, 0.0711, 0.0698, 0.0698, 0.1509, 0.0872},
                            /* V CW */     {    -1,     -1,     -1,     -1, 0.0960, 0.1018, 0.0955, 0.0948},
                            /* V CCW */    {    -1, 0.0948, 0.0955, 0.1018, 0.0960,     -1,     -1,     -1}
                        };

                        const double distanceOnLastZone = lastZoneDistanceTable[lastZoneSlide][lastZoneDistance];
                        if_unlikely(distanceOnLastZone < 0){
                            setError("distanceOnLastZone negative (%.4f).\nlastZoneSlide: %d\n", distanceOnLastZone, lastZoneSlide, lastZoneDistance);
                            return errorAt(n);
                        }
                        //printf("last zone: %.6f\n", distanceOnLastZone);

                        //step 4: MATH
                        //get only last slide of multi-slide
                        double variableT = 0.0;
                        if(lenOfSlideTimings >= 2){
                            variableT = slideTimings[lenOfSlideTimings - 1] * distanceOnLastZone;
                        } else {
                            //const double maxDist = chartNotes[chartLength].info.slide.arrows[lenOfArrows - 1].arrowTime;
                            const double oldMaxDist = chartNotes[chartLength].info.slide.arrows[SDL_max(0, oldLenOfArrows - 1)].arrowTime;
                            const double proportionOfOldToNewLenOfArrows = (maxDist - oldMaxDist) / maxDist;
                            variableT = endLength * proportionOfOldToNewLenOfArrows * distanceOnLastZone;
                        }
                        lenOfSlideTimings = 0;

                        chartNotes[chartLength].info.slide.variableT = variableT;

                        applyIndexesToSlide(chartLength);

                        const int lastI = lenOfSlidePoints - 1;
                        lenOfSlidePoints = 0;

                        switch(slidePoints[lastI].slideType){
                            case slideTypeArcLeft:
                            case slideTypeArcRight:
                            case slideTypeAutoArc:
                                chartNotes[chartLength].info.slide.drawType = judgeSlideArc;
                                bool clockwise = true;
                                if(slidePoints[lastI].slideType == slideTypeAutoArc){
                                    clockwise = shortestSlideDirection(slidePoints[lastI].location, lastLocation) == 1;
                                } else {
                                    //welcome to absolute nonsense that shouldn't even exist
                                    const bool arcIsLeft = slidePoints[lastI].slideType == slideTypeArcLeft;
                                    if (slidePoints[lastI].location >= 3 && slidePoints[lastI].location <= 6) {
                                        clockwise = arcIsLeft;
                                    } else {
                                        clockwise = !arcIsLeft;
                                    }
                                }
                                chartNotes[chartLength].info.slide.flipBigJudgeX = !clockwise;
                                break;
                            case slideTypeFan:
                                chartNotes[chartLength].info.slide.drawType = judgeSlideFan;
                                break;
                            default:
                                chartNotes[chartLength].info.slide.drawType = judgeSlideLine;
                                break;
                        }


                        chartLength++;
                        if(isAsterisk){
                            lastLocation = slidePoints[0].location;
                            skipSlideTapNote = true;
                            numberOfSlidesAtTheSameTime = 0; //is this intended
                        } else {
                            stage = scanning;
                        }

                        SDL_free(slidePoints);
                        slidePoints = NULL;
                        isGettingDuration = false;

                        if(!isSlash && !isAsterisk){
                            if(!isBacktick){ //might not be intended
                                lengthPerCommaMultiplier++;
                            }
                            currentNoteID++;
                            isEachNote = false;
                            isMultiSlide = false;
                            numberOfSlidesAtTheSameTime = 0;
                        }
                        break;
                    default:
                        break;
                }

                if(slideType != slideTypeEnd) {
                    addSlidePoint(lastLocation, slideType);
                }
                }
                break;
            case fanslidenote:{
                //actual final boss
                static bool isGettingDuration = false;
                static int numberOfFanSlides = 1; //niche use case
                static bool hasLastLocationUpdated = false; //for said niche use case
                static double fanDelayLength = 0;

                static int slideInputType = normalSlideRatio;
                static int slideNumUsed = 0;
                static char* slideNumbersStart[4] = {0};
                if(isGettingDuration){
                    if(symbol == ']'){
                        isGettingDuration = false;
                        if(slideTimings == NULL || (lenOfSlideTimings % 16 == 0 && lenOfSlideTimings > 0)){
                            slideTimings = (double*)SDL_realloc(slideTimings, sizeof(double) * (16 + lenOfSlideTimings));
                        }

                        double slideBeatsPerMinute = beatsPerMinute;
                        double beatDivisor = 1280;
                        double beatDivisorMultiplier = 1;
                        double endLength = -1;
                        double newDelayLength = -1;

                        chartErrorRange slideErrorCheck = getSlideDelayAndEndDuration(&slideInputType, &slideNumUsed, slideNumbersStart,
                                                                                        &newDelayLength, &slideBeatsPerMinute, &endLength,
                                                                                        &beatDivisor, &beatDivisorMultiplier, n);

                        if(slideErrorCheck.length > 0){
                            return slideErrorCheck;
                        }

                        if(newDelayLength < 0){
                            newDelayLength = 60.0 / slideBeatsPerMinute;
                        }

                        if(endLength < 0){
                            endLength = 240.0 / (slideBeatsPerMinute * beatDivisor) * beatDivisorMultiplier;
                        }

                        if(lenOfSlideTimings <= 0){ //is this accurate?
                            fanDelayLength = newDelayLength;
                        }

                        slideTimings[lenOfSlideTimings] = endLength;

                        lenOfSlideTimings++;

                        slideInputType = normalSlideRatio;
                        slideNumUsed = 0;
                        slideNumbersStart[0] = 0;
                        slideNumbersStart[1] = 0;
                        slideNumbersStart[2] = 0;
                        slideNumbersStart[3] = 0;
                        break;
                    } else if(slideNumbersStart[0] == 0){
                        slideNumbersStart[0] = chartFile + n + (symbol == '#');
                    }

                    if(symbol == '#'){
                        if(chartFile[n + 1] == '#'){ //custom waiting time
                            slideInputType = customWaitingTimeAndSlideDuration; //prototype
                            n++;
                        } else { //custom bpm
                            if(slideInputType == customWaitingTimeAndSlideDuration){
                                slideInputType = customWaitingTimeAndBPMNormalSlideRatio;
                            } else if(slideInputType == normalSlideRatio){
                                slideInputType = customSlideDuration;
                            } else {
                                slideInputType = customBPMAndSlideDuration;
                            }
                        }
                        if(chartFile[n - 1] != '['){
                            slideNumUsed++;
                        }
                    } else if(symbol == ':'){
                        slideNumUsed++;
                        if(slideInputType == customBPMAndSlideDuration || slideInputType == customSlideDuration){
                            slideInputType = customBPMNormalSlideRatio;
                        } else if(slideInputType == customWaitingTimeAndSlideDuration){
                            slideInputType = customWaitingTimeNormalSlideRatio;
                        }
                    } else if(slideNumbersStart[slideNumUsed] == 0){
                        slideNumbersStart[slideNumUsed] = chartFile + n;
                    }
                    break;
                } else if(SDL_isdigit(symbol)){
                    lastLocation = applyMirrorToButton(symbol - '0');
                    if_unlikely((numberOfFanSlides % 2 == 0) ? (noteLocation != lastLocation) : (getDistBetweenButton(noteLocation, lastLocation) != 4)){
                        setError("Illegal fanslide endpoint (start: %d wrong end: %d).\nLocation: %d", noteLocation, lastLocation, n);
                        return errorAt(n);
                    }
                    hasLastLocationUpdated = true;
                    break;
                }

                //int slideType = slideTypeEnd;
                switch(symbol){
                    case '[':
                        isGettingDuration = true;
                        break;
                    case 'b':
                        isSlideBreak = true;
                        break;
                    //fan slide is exclusive apparently
                    case 'w':
                        //slideType = slideTypeFan;
                        if(chartFile[n - 1] == '*'){ //does this hack work
                            n--;
                            stage = slidenote;
                            break;
                        }

                        if_unlikely(!hasLastLocationUpdated){
                            setError("Illegal fan slide (missing location).\nLocation: %d", n);
                            return errorAt(n);
                        }
                        hasLastLocationUpdated = false;
                        numberOfFanSlides++;
                        break;
                    case '-':
                    case '^':
                    case '<':
                    case '>':
                    case 'v':
                    case 's':
                    case 'z':
                    case 'p':
                    case 'q':
                    case 'V':
                        if_likely(chartFile[n - 1] == '*'){ //does this hack work
                            n--;
                            stage = slidenote;
                        } else {
                            setError("Illegal connected slides detected.\nLocation: %d", n);
                            return errorAt(n);
                        }
                        break;
                    case ',':
                    case '/':
                    case '`':
                    case '*':
                        hasLastLocationUpdated = false;
                        if(lastLocation != 0){ //error check add here??

                        }
                        numberOfSlidesAtTheSameTime++;
                        const bool isSlash = chartFile[n] == '/';
                        const bool isAsterisk = chartFile[n] == '*';
                        const bool isBacktick = chartFile[n] == '`';
                        if(isSlash){
                            isEachNote = true;
                        } else if(isAsterisk){
                            isMultiSlide = true;
                        } else if(numberOfSlidesAtTheSameTime == 2){
                            //previous slide wasn't marked as each
                            if_unlikely(!markPreviousSlideAsEach()){
                                setError("Failed to mark previous slide as each.\nLocation: %d", n);
                                return errorAt(n);
                            }
                        }

                        double *locationTimes = NULL;
                        if(lenOfSlideTimings >= 2){
                            locationTimes = (double*)SDL_malloc(sizeof(double) * lenOfSlideTimings);
                            locationTimes[0] = slideTimings[0];
                        }

                        double fanEndLength = slideTimings[0];
                        for(int i = 1; i < lenOfSlideTimings; i++){
                            fanEndLength += slideTimings[i];
                            if(lenOfSlideTimings >= 2){
                                locationTimes[i] = fanEndLength;
                            }
                        }

                        lenOfSlideTimings = 0;

                        reallocChartNotesBy(2 - skipSlideTapNote);

                        if(!skipSlideTapNote){
                            double rotationSpeed = 360.0;
                            if(utageTapSkin == tapNoteSpinningStarSkin){ //utage
                                rotationSpeed = 360.0; //place holder rotate speed
                            } else if(chartNotes[chartLength].info.tap.tapNoteSkin >= tapNoteSpinningStarSkin){ //normal
                                rotationSpeed = 360.0 / SDL_max(1.0 / 3.0, fanEndLength); //div by 0 protection
                            }
                            setupGenericNote(chartLength, noteLocation, internalTime, isEachNote, isBreak, isEachNote);
                            setupTapNote(chartLength, utageTapSkin < 0 && isMultiSlide ? tapNoteMultiStarSkin : tapNoteStarSkin, rotationSpeed);
                            isBreak = false;
                            isEXNote = false;
                            chartLength++;
                        }

                        setupGenericNote(chartLength, noteLocation, internalTime,
                                         isMultiSlide || numberOfSlidesAtTheSameTime >= 2, isSlideBreak, false);
                        setupFanSlideNote(chartLength, numberOfFanSlides, locationTimes,
                                          fanDelayLength * gameTicksPerSecond, fanEndLength * gameTicksPerSecond, shouldStarSuddenlyAppear);

                        isSlideBreak = false;
                        shouldStarSuddenlyAppear = false;
                        skipSlideTapNote = false;
                        numberOfFanSlides = 1;
                        lenOfSlidePoints = 0;
                        chartLength++;
                        if(isAsterisk){
                            lastLocation = slidePoints[0].location;
                            skipSlideTapNote = true;
                            numberOfSlidesAtTheSameTime = 0; //is this intended
                        } else {
                            stage = scanning;
                        }
                        isGettingDuration = false;

                        if(!isSlash && !isAsterisk){
                            if(!isBacktick){ //might not be intended
                                lengthPerCommaMultiplier++;
                            }
                            currentNoteID++;
                            isEachNote = false;
                            isMultiSlide = false;
                            numberOfSlidesAtTheSameTime = 0;
                        }
                        break;
                    default:
                        break;
                }
                }
                break;
            default:
                break;
                //error maybe?
        }
    }
    #undef internalTime
    #undef reallocateChartNotesBy
    setError("Not properly terminated.\nStart Index: %d\nLength: %d", startIndex, length);
    return errorAt(length - 1);
}

