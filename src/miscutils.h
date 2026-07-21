#pragma once
#ifndef Circular8_MISCUTILS_H
#define Circular8_MISCUTILS_H

#include <SDL3/SDL.h>
//faster math
#include <math.h>
#define SDL_sqrt(x) sqrt(x)
#define SDL_sqrtf(x) sqrtf(x)
#define SDL_floor(x) floor(x)
#define SDL_ceil(x) ceil(x)
#define SDL_fmodf(x, y) fmodf(x, y)
#define SDL_fmod(x, y) fmod(x, y)
#define SDL_sin(x) sin(x)
#define SDL_cos(x) cos(x)
#define SDL_atan2f(y, x) atan2f(y, x)
#define SDL_abs(x) abs(x)
#define SDL_fabs(x) fabs(x)
#define SDL_fabsf(x) fabsf(x)
//faster strings
#define SDL_strlen(s) strlen(s)
#define SDL_strnlen(s, m) strnlen(s, m)
#define SDL_strcmp(s1, s2) strcmp(s1, s2)
#define SDL_strncmp(s1, s2, m) strncmp(s1, s2, m)

//int charsCompare(char** charArray, char* stringToCompare, int lengthOfCharArray);

float interpolateFloat(float startFloat, float endFloat, float progress);

double interpolateDouble(double startFloat, double endFloat, double progress);

SDL_FPoint interpolateFPoint(SDL_FPoint startFPoint, SDL_FPoint endFPoint, float progress);

float interpolateDirections(float startDir, float endDir, float progress);

float distanceBetweenTwoFPoints(const SDL_FPoint* FPoint1, const SDL_FPoint* FPoint2);

float distanceBetweenTwoPositionsFloat(float x1, float y1, float x2, float y2);

double distanceBetweenTwoPositions(double x1, double y1, double x2, double y2);

float squareDistanceBetweenTwoFPoints(const SDL_FPoint* FPoint1, const SDL_FPoint* FPoint2);

float startFPPointsTowardEndFP(const SDL_FPoint* startFPoint, const SDL_FPoint* endFPoint);

int shortestDirection(float startDir, float endDir);

float circularFloatDistance(float first, float second, float length);

SDL_FPoint moveXYWithDirAndDist(float x, float y, double direction, double distance);

SDL_FPoint moveXYWithDirAndDistFloat(float x, float y, float direction, float distance);

#define simple_snprintf(string, format, ...) SDL_snprintf((string), sizeof(string), format, __VA_ARGS__)

bool isPointInsideFRect(int pointX, int pointY, SDL_FRect rect);

#define subtractUnsignedNumberToAtLeast0(unsignedNumber, subtractBy) do {if((subtractBy) >= (unsignedNumber)){unsignedNumber = 0;} else {unsignedNumber -= (subtractBy);}} while(0);

#define stackArraySize(array) (sizeof(array) == 0 ? 0 : sizeof(array) / sizeof(array[0]))
#define stackArraySizeInt(array) ((int)stackArraySize(array))

char* joinTwoCharArraysToOne(const char* charArray1, const char* charArray2);

char* joinPrefixMiddleEndCharArrays(const char* charArrayPrefix, const char* charArrayMiddle, const char* charArrayRight);

#define cot(x) (1.0/SDL_tan(x))

#define degToRad(x) ((x) / 57.2957795131)
#define degToRadFloat(x) ((x) * 0.01745329251f)
#define radToDeg(x) ((x) * 57.2957795131)

#define sinDeg(x) (SDL_sin(degToRad(x)))
#define sinfDeg(x) (SDL_sinf(degToRadFloat(x)))
#define cosDeg(x) (SDL_cos(degToRad(x)))
#define cosfDeg(x) (SDL_cosf(degToRadFloat(x)))
#define cotDeg(x) (cot(degToRad(x)))
#define tanDeg(x) (SDL_tan(degToRad(x)))
#define tanSquaredDeg(x) (squareDouble(tanDeg(x)))


#define atanDeg(x) (radToDeg(SDL_atan(x)))
#define asinDeg(x) (radToDeg(SDL_asin(x)))


#define squareMacro(x) ((x) * (x))

#define absMacro(x) ((x) < 0 ? (-(x)) : x)

inline float squareFloat(float x){
    return x * x;
}

inline double squareDouble(double x){
    return x * x;
}

inline int clampInt(int value, int minValue, int maxValue){
    return SDL_clamp(value, minValue, maxValue);
}

inline double clampDouble(double value, double minValue, double maxValue){
    return SDL_clamp(value, minValue, maxValue);
}

#define boolToInt(x) ((x) * 2 - 1)
SDL_FPoint rotateFPointBy(float degrees, SDL_FPoint FPointInput);

char* loadStrFromFile(const char* file, size_t* fileSize);

#define white ((SDL_Color){255, 255, 255, 255})
float progressivelyMoveBy(Uint64 delta, Uint64* timer, Uint64 duration, int* index, int* changeDirection, int len, float moveValue);

//#define makeAngleInRange(angle) ((angle) - (SDL_floor((angle) / 360.0) * 360.0))
//no clue if this is faster but it avoids a division
#define makeAngleInRange(angle) ((angle) - (SDL_floor((angle) * 0.00277777777) * 360.0))
#define makeAngleInRangeFloat(angle) ((angle) - (SDL_floorf((angle) * 0.00277777777f) * 360.f))
bool compareTwoColors(SDL_Color a, SDL_Color b);
//int wrapAroundModulus(int i, int n);

#define UNUSED(var) (void)(var)
void *reallocfTypeSDL(void *mem, size_t size);

char* base64URLEncoding(const unsigned char* byteArray, size_t len);

#if defined(__GNUC__) || defined(__clang__)
#define if_likely(x) if(__builtin_expect(!!(x), 1))
#define if_unlikely(x) if(__builtin_expect(!!(x), 0))
#else
#define if_likely(x) if(x)
#define if_unlikely(x) if(x)
#endif

SDL_Color hexCodeToColor(const char* hexCode);
bool isValidHex(const char* hexCode);
#define assertStackArrayBounds(data, index) assert((index) >= 0 && (size_t)(index) < stackArraySize(data))
double safeStrtod(const char* str, char** endP, bool* isValid);

#endif // Circular8_MISCUTILS_H
