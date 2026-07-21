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

//external libs
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>


#include "main.h"
#include "load.h"
#include "render.h"
#include "miscutils.h"
#include "game/options.h"
SDL_Renderer *renderer;



//int rendererIndex = -1;

/*SDL_Renderer* initializeRenderer(SDL_Window* window) {
    SDL_Renderer* renderer2 = SDL_CreateRenderer(window, rendererIndex, 0);

    if(renderer2 == NULL){
        setError("An error occurred while creating the renderer.\nError: %s", SDL_GetError());
        return NULL;
    }

    return renderer2;
}*/

//see render.h
/*Uint8 convertTimerAndDurationToBrightness(int timer, int duration){
    if(timer == 0){
        return 0;
    } else if(duration == 0 || timer >= duration){
        return 255;
    }
    Uint8 result = (255 * 1000) / ((duration * 1000) / timer);
    return result;
}*/

float guiScale = 1.f;
const float guiWidth = 720.f;
const float guiHeight = 720.f;
const float subGuiHeight = 720.0 / (17.0/7.0);

int windowWidth = 720;
int windowHeight = 720;

int exclusiveWidth = 0;
int exclusiveHeight = 0;
double exclusiveHz = 0;

int screenWidth = 720;
int screenHeight = 720;
int screenMainHeight = 720;
int fullscreenMode = 0;

float getCenterX(void){
    return (screenWidth - 1.f) / 2.f;
}

float getCenterY(void){
    return ((useSubscreen ? screenWidth : screenHeight) - 1.f) / 2.f;
}

float getGuiCenterX(void){
    return (guiWidth - 1.f) / 2.f;
}

//unused
/*float getGuiCenterY(void){
    return (guiHeight - 1.f) / 2.f;
}

SDL_FPoint getCenterFPoint(void){
    return (SDL_FPoint){getCenterX(), getCenterY()};
}

SDL_FRect xy(float x, float y){
    SDL_FRect xyRect = {x, y, 1.f, 1.f};
    return xyRect;
}*/

SDL_FRect xywh(float x, float y, float w, float h){
    SDL_FRect xywhRect = {x, y, w, h};
    return xywhRect;
}

SDL_FRect xywhScaled(float x, float y, float w, float h){
    return xywh(x * guiScale, y * guiScale, w * guiScale, h * guiScale);
}

SDL_Rect wh(float w, float h){
    SDL_Rect whRect = {0, 0, SDL_roundf(w), SDL_roundf(h)};
    return whRect;
}

SDL_Rect whScaled(float w, float h){
    return wh(w * guiScale, h * guiScale);
}

void changeFRectSizeBy(int moveValue, SDL_FRect *currentRect){
    currentRect->x -= moveValue;
    currentRect->y -= moveValue;
    currentRect->w += moveValue * 2.f;
    currentRect->h += moveValue * 2.f;
}

void changeRectSizeBy(int moveValue, SDL_Rect *currentRect){
    currentRect->x -= moveValue;
    currentRect->y -= moveValue;
    currentRect->w += moveValue * 2;
    currentRect->h += moveValue * 2;
}



//unused
/*SDL_FRect FRectWithTexSizeEX(SDL_Texture *texture, float x, float y, float scaleW, float scaleH, double direction, double distance, bool centered){
    float texW = 0.f;
    float texH = 0.f;
    if(texture != NULL){
        SDL_GetTextureSize(texture, &texW, &texH);
    } else {
        texW = 1.f;
        texH = 1.f;
    }
    //math
    direction = degToRad(direction); //M_PI bad now?
    SDL_FRect resultRect = {x + (float)(SDL_sin(direction) * distance), y - (float)(SDL_cos(direction) * distance), texW, texH};
    resultRect.w *= scaleW;
    resultRect.h *= scaleH;
    if(centered){
        resultRect.x -= resultRect.w / 2.f;
        resultRect.y -= resultRect.h / 2.f;
    }
    return resultRect;
}*/

SDL_FRect FRectSimple(const SDL_FRect *FRect, float scale, bool centered){
    SDL_FRect resultRect = {FRect->x, FRect->y, FRect->w, FRect->h};
    resultRect.w *= scale;
    resultRect.h *= scale;
    if(centered){
        resultRect.x -= resultRect.w / 2.f;
        resultRect.y -= resultRect.h / 2.f;
    }
    return resultRect;
}

void centerFRect(SDL_FRect *FRect, bool roundXandY){
    FRect->x -= FRect->w / 2.f;
    FRect->y -= FRect->h / 2.f;

    if(roundXandY){
        FRect->x = SDL_roundf(FRect->x);
        FRect->y = SDL_roundf(FRect->y);
    }
}

SDL_FRect FRectWithTexSimple(SDL_Texture* texture, float x, float y, float scale, bool centered){
    float texW = 1;
    float texH = 1;
    if(texture != NULL){
        SDL_GetTextureSize(texture, &texW, &texH);
    } else {
        texW = 1;
        texH = 1;
    }

    SDL_FRect resultRect = {x, y, texW, texH};
    resultRect.w *= scale;
    resultRect.h *= scale;
    if(centered){
        resultRect.x -= resultRect.w / 2.f;
        resultRect.y -= resultRect.h / 2.f;
    }
    return resultRect;
}

int rendererThing(int argc, char *argv[]){
    int rendererIndex = -1;

    const int numDrivers = SDL_GetNumRenderDrivers();
    for(int i = 0; i < numDrivers; i++){
        const char* driverAtI = SDL_GetRenderDriver(i);

        if(driverAtI == NULL){
            setError("SDL_GetRenderDriver failed.\nError: %s", SDL_GetError());
            return -1;
        } else {
            puts(driverAtI);
        }

        for(int n = 0; n < argc; n++){
            if(SDL_strcmp(driverAtI, argv[n]) == 0){
                if(rendererIndex != -1){
                    printf("Too many renderer args, using last one. %d %s\n", n, driverAtI);
                }
                printf("Selected renderer %s. index: %d\n", driverAtI, n);
                rendererIndex = i;
            }
        }
    }
    return rendererIndex;
}

void setRenderDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 alpha){
    SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
}

void setRenderClipRect(const SDL_Rect *rect){
    SDL_SetRenderClipRect(renderer, rect);
}

void setTextureColor(SDL_Texture *texture, Uint8 r, Uint8 g, Uint8 b){
    SDL_SetTextureColorMod(texture, r, g, b);
}

void setTextureAlpha(SDL_Texture *texture, Uint8 alpha){
    SDL_SetTextureAlphaMod(texture, alpha);
}

void renderFillRectF(const SDL_FRect *rect){
    //cringe
    //renderFillRect(&(const SDL_Rect){SDL_roundf(rect->x), SDL_roundf(rect->y), SDL_roundf(rect->w), SDL_roundf(rect->h)});
    SDL_RenderFillRect(renderer, rect);
}

void renderFillRect(const SDL_Rect *rect){
    if(rect == NULL){
        renderFillRectF(NULL);
    } else {
        const SDL_FRect fillFRect = {rect->x, rect->y, rect->w, rect->h};
        renderFillRectF(&fillFRect);
    }
}

/*void renderDrawRect(const SDL_Rect *rect){
    SDL_RenderDrawRect(renderer, rect);
}*/

bool renderTextureRotated(SDL_Texture *texture, const SDL_FRect *srcrect, const SDL_FRect *dstrect, double angle, const SDL_FPoint *center, SDL_FlipMode flip){
    return SDL_RenderTextureRotated(renderer, texture, srcrect, dstrect, angle, center, flip);
}

void renderSpriteEX(int atlasID, int textureID, float x, float y, float scale, bool centered, double angle, SDL_FlipMode flip){
    SDL_Texture* texture;
    SDL_FRect src;
    getAtlasTextureAndSrc(atlasID, textureID, &texture, &src);
    SDL_FRect dst = xywh(x, y, src.w * scale, src.h * scale);
    if(centered){
        centerFRect(&dst, false);
    }
    renderTextureRotated(texture, &src, &dst, angle, NULL, flip);
}

void renderSprite(int atlasID, int textureID, float x, float y, bool centered){
    renderSpriteEX(atlasID, textureID, x, y, 1.f, centered, 0.0, SDL_FLIP_NONE);
}

void setAlphaOfAtlas(int atlasID, int textureID, Uint8 alpha){
    SDL_Texture* texture;
    getAtlasTextureAndSrc(atlasID, textureID, &texture, NULL);
    setTextureAlpha(texture, alpha);
}

void setColorOfAtlas(int atlasID, int textureID, Uint8 red, Uint8 green, Uint8 blue){
    SDL_Texture* texture;
    getAtlasTextureAndSrc(atlasID, textureID, &texture, NULL);
    setTextureColor(texture, red, green, blue);
}

void renderSpritePixelCenter(int atlasID, int textureID, float x, float y){
    SDL_Texture* texture;
    SDL_FRect src;
    getAtlasTextureAndSrc(atlasID, textureID, &texture, &src);
    SDL_FRect dst = xywh(x, y, src.w, src.h);
    centerFRect(&dst, true);
    renderTexture(texture, &src, &dst);
}

bool renderTexture(SDL_Texture *texture, const SDL_FRect *srcrect, const SDL_FRect *dstrect){
    return SDL_RenderTexture(renderer, texture, srcrect, dstrect);
}

bool renderTextureSimpleManual(SDL_Texture *texture, const SDL_FRect* FRect, bool centered){
    const SDL_FRect textureFRect = FRectSimple(FRect, 1.f, centered);
    return renderTexture(texture, NULL, &textureFRect);
}

bool renderTextureSimple(SDL_Texture *texture, float x, float y, float scale, bool centered){
    SDL_FRect textureFRect = FRectWithTexSimple(texture, x, y, scale, centered);
    //textureFRect.x = SDL_roundf(textureFRect.x);
    //textureFRect.y = SDL_roundf(textureFRect.y);
    return renderTexture(texture, NULL, &textureFRect);
}

bool renderTextureSimpler(SDL_Texture *texture, float x, float y){
    return renderTextureSimple(texture, x, y, 1.f, true);
}

bool isScreenRotatingInPortrait(void){
    return rotateScreenBy == 90 || rotateScreenBy == 270;
}

bool renderNumbersEx(char* charArray, float x, float y, int align, bool verticalCentered, SDL_Color backColor, int moveValue){
    const int numberOfChars = SDL_strnlen(charArray, 32); //32 is probably enough
    if(numberOfChars <= 0){ //nothing to render
        return true;
    }

    SDL_FRect FRects[32] = {0};
    float totalWidth = 0.f;
    for(int i = 0; i < numberOfChars; i++){
        const char* location = SDL_strchr(numberChars, charArray[i]);
        if(location == NULL){ //prevent crash
            continue;
        }
        int index = location - numberChars; //is this valid
        FRects[i] = numbersTextFRects[index];
        totalWidth += FRects[i].w;
    }

    const float startingX = x - (align == middleAlign ? totalWidth / 2.f : align == rightAlign ? totalWidth : 0.f);
    const float startingY = verticalCentered ? y - FRects[0].h / 2.f : y;
    float newX = startingX;
    if(backColor.a > 0){
        setRenderDrawColor(backColor.r, backColor.g, backColor.b, backColor.a);
        SDL_FRect backColorFRect = xywh(startingX, startingY, totalWidth, FRects[0].h);
        changeFRectSizeBy(moveValue, &backColorFRect);
        renderFillRectF(&backColorFRect);
    }

    for(int i = 0; i < numberOfChars; i++){
        const SDL_FRect dstFRect = xywh(newX, startingY, FRects[i].w, FRects[i].h);
        renderTexture(numbersText, &FRects[i], &dstFRect);
        newX += dstFRect.w;
    }
    return true;
}

bool renderNumbers(char* charArray, float x, float y, int align, bool verticalCentered){
    return renderNumbersEx(charArray, x, y, align, verticalCentered, ((SDL_Color){0, 0, 0, 0}), 0);
}

Uint8 fadeInFadeOutAlpha(double timer, double fadeInEnd, double fadeOutStart, double fadeOutEnd){
    if(timer > fadeInEnd && timer < fadeOutStart){
        return SDL_ALPHA_OPAQUE;
    } else if(timer < 0 || timer > fadeOutEnd){
        return SDL_ALPHA_TRANSPARENT;
    }

    return timer <= fadeInEnd ? convertTimerAndDurationToBrightness(timer, fadeInEnd) :
                                SDL_ALPHA_OPAQUE - convertTimerAndDurationToBrightness(timer - fadeOutStart, fadeOutEnd - fadeOutStart);
}


