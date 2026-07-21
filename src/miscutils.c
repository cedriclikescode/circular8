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
#include <stdbool.h>
#include <SDL3/SDL.h>
#include "miscutils.h"

float interpolateFloat(float startFloat, float endFloat, float progress){
    const float distance = endFloat - startFloat;
    return startFloat + distance * progress;
}

double interpolateDouble(double startFloat, double endFloat, double progress){
    const double distance = endFloat - startFloat;
    return startFloat + distance * progress;
}

int shortestDirection(float startDir, float endDir){
    float result = endDir - startDir;
    while(result > 180.f){
        result -= 360.f;
    }

    while(result < -180.f){
        result += 360.f;
    }

    if(result < 0.f){
        return -1;
    }
    return 1;
}

float interpolateDirections(float startDir, float endDir, float progress){
    while(startDir < 0.f || endDir < 0.f){
        startDir += 360.0f;
        endDir += 360.0f;
    }

    startDir = SDL_fmodf(startDir, 360.f);
    endDir = SDL_fmodf(endDir, 360.f);
    const int shortestDir = shortestDirection(startDir, endDir);
    const float difference = circularFloatDistance(startDir, endDir, 360.f);
    float newDirection = 0.f;
    if(shortestDir == -1){
        newDirection = startDir - difference * progress;
    } else {
        newDirection = startDir + difference * progress;
    }

    //faster? may break if progress is not in the range of 0 to 1
    return newDirection < 0.f ? newDirection + 360.f : newDirection >= 360.f ? newDirection - 360.f : newDirection;
}

SDL_FPoint interpolateFPoint(SDL_FPoint startFPoint, SDL_FPoint endFPoint, float progress) {
    const float distanceX = endFPoint.x - startFPoint.x;
    const float distanceY = endFPoint.y - startFPoint.y;
    SDL_FPoint result = {startFPoint.x + distanceX * progress, startFPoint.y + distanceY * progress};
    return result;
}

float distanceBetweenTwoFPoints(const SDL_FPoint* FPoint1, const SDL_FPoint* FPoint2) {
    const float FP2XminusFP1X = FPoint2->x - FPoint1->x;
    const float FP2YminusFP1Y = FPoint2->y - FPoint1->y;
    return SDL_sqrtf(FP2XminusFP1X * FP2XminusFP1X + FP2YminusFP1Y * FP2YminusFP1Y);
}

double distanceBetweenTwoPositions(double x1, double y1, double x2, double y2){
    return SDL_sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

float distanceBetweenTwoPositionsFloat(float x1, float y1, float x2, float y2){
    return SDL_sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

float squareDistanceBetweenTwoFPoints(const SDL_FPoint* FPoint1, const SDL_FPoint* FPoint2){
    return squareFloat(FPoint1->x - FPoint2->x) + squareFloat(FPoint1->y - FPoint2->y);
}

float startFPPointsTowardEndFP(const SDL_FPoint* startFPoint, const SDL_FPoint* endFPoint) {
    const float result = SDL_atan2f(endFPoint->y - startFPoint->y, endFPoint->x - startFPoint->x) * 57.2957795131f + 90.f; //for some reason

    return result < 0.f ? result + 360.f : result; //unmodified result range is -90.f to 270.f
}

float circularFloatDistance(float first, float second, float length){
    const float diff = SDL_max(first, second) - SDL_min(first, second);

    return SDL_min(diff, length - diff);
}

SDL_FPoint moveXYWithDirAndDist(float x, float y, double direction, double distance){ //expensive
    direction = makeAngleInRange(direction);

    direction = degToRad(direction);
    SDL_FPoint resultFPoint = {x + (float)(SDL_sin(direction) * distance), y - (float)(SDL_cos(direction) * distance)};


    return resultFPoint;
}

SDL_FPoint moveXYWithDirAndDistFloat(float x, float y, float direction, float distance){ //slightly cheaper
    direction = makeAngleInRangeFloat(direction);

    direction = degToRadFloat(direction);
    SDL_FPoint resultFPoint = {x + SDL_sinf(direction) * distance, y - SDL_cosf(direction) * distance};

    return resultFPoint;
}

char* joinTwoCharArraysToOne(const char* charArray1, const char* charArray2){
    const size_t newCharArraySize = (SDL_strlen(charArray1) + SDL_strlen(charArray2) + 2) * sizeof(char);
    char* newCharArray = SDL_malloc(newCharArraySize);
    snprintf(newCharArray, newCharArraySize, "%s%s", charArray1, charArray2);
    return newCharArray;
}

char* joinPrefixMiddleEndCharArrays(const char* charArrayPrefix, const char* charArrayMiddle, const char* charArrayRight){
    const size_t newCharArraySize = (SDL_strlen(charArrayPrefix) + SDL_strlen(charArrayMiddle) + SDL_strlen(charArrayRight) + 2) * sizeof(char);
    char* newCharArray = SDL_malloc(newCharArraySize);
    snprintf(newCharArray, newCharArraySize, "%s%s%s", charArrayPrefix, charArrayMiddle, charArrayRight);
    return newCharArray;
}

extern inline float squareFloat(float x);
extern inline double squareDouble(double x);

extern inline int clampInt(int value, int minValue, int maxValue);
extern inline double clampDouble(double value, double minValue, double maxValue);

SDL_FPoint rotateFPointBy(float degrees, SDL_FPoint FPointInput){
    const float x = FPointInput.x * cosfDeg(degrees) - FPointInput.y * sinfDeg(degrees);
    const float y = FPointInput.x * sinfDeg(degrees) + FPointInput.y * cosfDeg(degrees);
    return (SDL_FPoint){x, y};
}

char* loadStrFromFile(const char* file, size_t* fileSize){
    SDL_IOStream* fileRWops = SDL_IOFromFile(file, "rb");
    if(fileRWops == NULL) {
        return NULL;
    }

    const size_t sizeOfFile = (size_t)SDL_GetIOSize(fileRWops);
    char* strFile = SDL_calloc(sizeOfFile + 1, 1); //i love magic
    if(strFile == NULL){
        return NULL;
    }

    SDL_ReadIO(fileRWops, strFile, sizeOfFile);
    if_unlikely(!SDL_CloseIO(fileRWops)){
        return NULL;
    }

    if(fileSize != NULL){
        (*fileSize) = sizeOfFile;
    }
    return strFile;
}

float progressivelyMoveBy(Uint64 delta, Uint64* timer, Uint64 duration, int* index, int* changeDirection, int len, float moveValue){
    const unsigned int repeatLen = 3 + (*timer + delta) / duration; //magic formula

    for(unsigned int i = 0; i < repeatLen; i++){
        if((*changeDirection) != 0 && ((*changeDirection) > 0 ? (*index) < len - 1 : (*index) > 0)){
            (*timer) += delta;
            if((*timer) >= duration){
                (*timer) -= duration;
                if(*changeDirection > 0){
                    (*changeDirection)--;
                    (*index)++;
                } else {
                    (*changeDirection)++;
                    (*index)--;
                }

                //printf("song change length %llu\n", duration);
                //*changeDirection -= upDownDir;
                //upDownDir = 0;
            }
        } else {
            (*changeDirection) = 0;
        }

        if((*changeDirection) == 0){
            (*timer) = 0;
        }
        delta = 0;//weird fix
    }

    const float distance = (*changeDirection) > 0 ? -moveValue : moveValue;
    return distance * (*timer) / duration;
}

bool compareTwoColors(SDL_Color a, SDL_Color b){
    return a.a == b.a && a.r == b.r && a.g == b.g && a.b == b.b;
}

void *reallocfTypeSDL(void *mem, size_t size){
    void* newMem = SDL_realloc(mem, size);
    if_unlikely(newMem == NULL){
        SDL_free(mem);
        return NULL;
    }
    return newMem;
}

char* base64URLEncoding(const unsigned char* byteArray, size_t len){
    const size_t encodedLen = len + len / 3;
    char* encodedArray = SDL_calloc(encodedLen + 4, sizeof(char)); //4 characters per 3 bytes

    const char base64Table[64] = {
        'A', 'B', 'C', 'D',
        'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b',
        'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j',
        'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r',
        's', 't', 'u', 'v',
        'w', 'x', 'y', 'z',
        '0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', '-', '_', //+ is -, / is _
    };

    size_t realI = 0;

    for(size_t i = 0; i < len; i++){
        switch(i % 3){
            case 0:
                encodedArray[realI] = base64Table[byteArray[i] / 4];
                break;
            case 1:
                encodedArray[realI] = base64Table[(byteArray[i - 1] % 4) * 16 + byteArray[i] / 16];
                break;
            case 2:
                encodedArray[realI] = base64Table[(byteArray[i - 1] % 16) * 4 + byteArray[i] / 64];
                realI++;
                encodedArray[realI] = base64Table[byteArray[i] % 64];
                break;
            default:
                break;
        }
        realI++;
    }

    if(len % 3 == 1){
        encodedArray[realI] = base64Table[(byteArray[len - 1] % 4) * 16];
    } else if(len % 3 == 2){
        encodedArray[realI] = base64Table[(byteArray[len - 1] % 16) * 4];
    }

    return encodedArray;
}

SDL_Color hexCodeToColor(const char* hexCode){
    const size_t hexLen = SDL_strnlen(hexCode, 7);
    const bool halfHex = hexLen == 4;
    SDL_Color result = {0, 0, 0, SDL_ALPHA_OPAQUE};
    Uint8 color = 0;

    for(unsigned int i = 1; i < (halfHex ? 7 : hexLen); i++){
        const char letter = hexCode[i / (1 + halfHex) + halfHex];
        if(SDL_isdigit(letter)){
            color += letter - '0';
        } else {
            const char validHex[] = "AaBbCcDdEeFf";
            for(int n = 0; n < stackArraySizeInt(validHex); n++){
                if(letter == validHex[n]){
                    color += n / 2 + 10;
                    break;
                }
            }
        }

        if(i % 2 == 1){
            color *= 16;
        } else {
            switch((i - 1) / 2){
                case 0:
                    result.r = color;
                    break;
                case 1:
                    result.g = color;
                    break;
                case 2:
                    result.b = color;
                    break;
                default:
                    return result;
            }
            color = 0;
        }
    }
    return result;
}

bool isValidHex(const char* hexCode){
    const size_t hexLen = SDL_strnlen(hexCode, 8);
    if((hexLen != 4 && hexLen != 7) || hexCode[0] != '#'){
        return false;
    }

    for(unsigned int i = 1; i < hexLen; i++){
        if(SDL_isdigit(hexCode[i])){
            continue;
        }

        const char validHex[] = "ABCDEFabcdef";
        int n = 0;
        for(n = 0; n < stackArraySizeInt(validHex); n++){
            if(hexCode[i] == validHex[n]){
                break;
            }
        }

        if(n < stackArraySizeInt(validHex)){
            continue;
        }
        return false;
    }
    return true;
}

double safeStrtod(const char* str, char** endP, bool* isValid){ //is this undefined?
    (*isValid) = false;
    bool seenPeriod = false;
    if(!SDL_isdigit(*str)){
        (*endP) = (char*)str;
        return 0.0;
    }

    for((*endP) = (char*)str; (**endP) != '\0'; (*endP)++){
        if((**endP) == '.'){
            if(seenPeriod){
                return 0.0;
            } else {
                seenPeriod = true;
            }
        } else if(!SDL_isdigit(**endP)){
            (*isValid) = !seenPeriod || SDL_isdigit(*((*endP) - 1));
            break;
        }
    }
    return SDL_strtod(str, endP);
}
