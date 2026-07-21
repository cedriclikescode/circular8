#pragma once
#ifndef Circular8_RENDER_H
#define Circular8_RENDER_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "render.h"

#define convertTimerAndDurationToBrightness(timer, duration) ((Uint8)((timer) <= 0 ? 0 : ((duration) <= 0 || (timer) >= (duration)) ? 255 : 255.0 * (double)(timer) / (double)(duration)))
extern SDL_Renderer *renderer;

SDL_Renderer* initializeRenderer(SDL_Window* window);
int rendererThing(int argc, char *argv[]);

void setRenderDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);

void renderDrawRect(const SDL_Rect *rect);
void renderFillRect(const SDL_Rect *rect);
void renderFillRectF(const SDL_FRect *rect);

//SDL_FRect FRectWithTexSizeEX(SDL_Texture *texture, float x, float y, float scaleW, float scaleH, double direction, double distance, bool centered);
SDL_FRect FRectWithTexSimple(SDL_Texture *texture, float x, float y, float scale, bool centered);
SDL_FRect FRectSimple(const SDL_FRect *FRect, float scale, bool centered);
void centerFRect(SDL_FRect *FRect, bool roundXandY);

bool renderTextureRotated(SDL_Texture *texture, const SDL_FRect *srcrect, const SDL_FRect *dstrect, double angle, const SDL_FPoint *center, SDL_FlipMode flip);
bool renderTexture(SDL_Texture *texture, const SDL_FRect *srcrect, const SDL_FRect *dstrect);
bool renderTextureSimple(SDL_Texture *texture, float x, float y, float scale, bool centered);
bool renderTextureSimpler(SDL_Texture *texture, float x, float y);
bool renderTextureSimpleManual(SDL_Texture *texture, const SDL_FRect* FRect, bool centered);
void renderSprite(int atlasID, int textureID, float x, float y, bool centered);
void renderSpritePixelCenter(int atlasID, int textureID, float x, float y);
void renderSpriteEX(int atlasID, int textureID, float x, float y, float scale, bool centered, double angle, SDL_FlipMode flip);
void setAlphaOfAtlas(int atlasID, int textureID, Uint8 alpha);
void setColorOfAtlas(int atlasID, int textureID, Uint8 red, Uint8 green, Uint8 blue);

enum {
    leftAlign = 0,
    middleAlign = 1,
    rightAlign = 2
};

bool renderNumbersEx(char* charArray, float x, float y, int align, bool verticalCentered, SDL_Color backColor, int moveValue);
bool renderNumbers(char* charArray, float x, float y, int align, bool verticalCentered);

void setTextureColor(SDL_Texture *texture, Uint8 r, Uint8 g, Uint8 b);
void setTextureAlpha(SDL_Texture *texture, Uint8 alpha);
//bool simpleQueryTexture(SDL_Texture *texture, int *w, int *h);

void setRenderClipRect(const SDL_Rect *rect);

void changeFRectSizeBy(int moveValue, SDL_FRect *currentRect);
void changeRectSizeBy(int moveValue, SDL_Rect *currentRect);

extern float guiScale;
extern int windowWidth, windowHeight;
extern int exclusiveWidth, exclusiveHeight;
extern double exclusiveHz;
extern int screenWidth, screenHeight, fullscreenMode;
extern int screenMainHeight;
extern const float guiWidth, subGuiHeight;
extern const float guiHeight;

#define screenSize (720.f * guiScale)

#define screenXOffset (SDL_max(screenWidth - screenSize, 0) / 2.f)
#define screenYOffset (SDL_max(screenMainHeight - screenSize, 0) / 2.f)

typedef struct {
    int w;
    int h;
} sizeInt;

typedef struct {
    float w;
    float h;
} sizeFloat;

float getCenterX(void);
float getCenterY(void);
float getGuiCenterX(void);

SDL_FPoint getCenterFPoint(void);
SDL_FRect xy(float x, float y);
SDL_FRect xywh(float x, float y, float w, float h);
SDL_Rect wh(float w, float h);
SDL_Rect whScaled(float w, float h);
SDL_FRect xywhScaled(float x, float y, float w, float h);

Uint8 fadeInFadeOutAlpha(double timer, double fadeInEnd, double fadeOutStart, double fadeOutEnd);
bool isScreenRotatingInPortrait(void);
#endif // Circular8_RENDER_H
