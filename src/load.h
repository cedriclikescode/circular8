#pragma once
#ifndef Circular8_LOAD_H
#define Circular8_LOAD_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

enum {
    nearestNeighbor = 0,
    linear = 1
};

enum {
    loadError = -1
};

enum {
    fileTypeImage = 0,
    fileTypeSound = 1,
    fileTypeAtlasText = 2,
    fileTypeRawSVG = 3
};

typedef struct fileItem {
    char *file;
    int fileType;
    union options {
        struct image {
            SDL_Rect rect;
            SDL_Texture **texture;
            int imageScaler;
            int id;
            bool grayScale;
            Uint8 fadeBy;
        } image;

        struct audio {
            MIX_Audio **sfx;
        } audio;

        struct atlasText {
            TTF_Font** font;
            SDL_Texture **texture;
            char* text;
            int id;
            bool autoCrop;
        } atlasText;

        struct rawSVG {
            SDL_Texture **texture;
            SDL_Rect rect;
            int id;
        } rawSVG;
    } options;
} fileItem;

typedef struct atlasItem {
    SDL_FRect srcFRect;
    int subAtlasID;
} atlasItem;

struct fileItem;
struct atlasItem;
extern bool isLoadImagePart;
int setError(const char *fmt, ...);
int loadSound(MIX_Audio **sfxToLoad, const char *file);
int loadImage(SDL_Texture **imageToLoad, const char *file, int scalerType, SDL_Rect rect);
int loadImageEx(SDL_Texture **imageToLoad, const char *file, int scalerType, SDL_Rect rect, int atlasID, bool grayScale, Uint8 fadeBy);
int loadImageSimple(SDL_Texture **imageToLoad, const char *file, SDL_Rect rect);
int loadRawSVGSimple(SDL_Texture **imageToLoad, const char *SVGCode, SDL_Rect rect);
SDL_Texture *loadText(TTF_Font *font, const char* text, SDL_Color fontColor);
SDL_Texture *autoCropLoadText(TTF_Font *font, const char* text, SDL_Color fontColor);
int loadTextSimple(SDL_Texture **textToLoad, TTF_Font *font, const char* text, SDL_Color fontColor, bool autoCrop);
int loadTextLocalized(SDL_Texture **textToLoad, TTF_Font *font, const char* text, SDL_Color fontColor, bool autoCrop);
int loadAllThings(void);

void runLoadFunction(void (*loadFunc)(int slot), int slot);

void setAtlas(int id, int padding);
void getAtlasTextureAndSrc(int atlasID, int textureID, SDL_Texture** texture, SDL_FRect* src);
void runLoadOnly(int slot, bool shouldShowProgress);
#endif // Circular8_LOAD_H
