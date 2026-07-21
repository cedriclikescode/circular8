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
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "main.h"
#include "gameinit.h"
#include "miscutils.h"
#include "render.h"
#include "texture.h"
#include "language.h"

#include "events.h"
#include "load.h"

#include "game/ingame.h"

static fileItem *files = NULL;
static int filesIndex = 0;
static char* loadErrorStr = NULL;

static void reallocFilesIfFull(void){
    if(filesIndex % 64 == 0){
        files = reallocfTypeSDL(files, sizeof(fileItem) * (filesIndex + 64));
    }
}

SDL_Texture *atlasTextures[2][2] = {0};
atlasItem *atlasInfo[2] = {0};
SDL_Surface* atlaSurfaces[2][128] = {0};
static int lenOfAtlaSurfaces[2] = {0};
static int atlaSurfacePaddings[2] = {0};
static int currentAtlasID = -1;


static bool hasErrorOccurred = false;
static bool showMessageBoxOnError = true; //used during early initialization
bool isLoadImagePart = true;

void setAtlas(int id, int padding){ //-1 id to disable
    currentAtlasID = id;
    if(id >= 0){
        lenOfAtlaSurfaces[id] = 0;
        atlaSurfacePaddings[id] = padding;
        destroyTextureIfNotNull(atlasTextures[id][0]);
        destroyTextureIfNotNull(atlasTextures[id][1]);
    }
}

void getAtlasTextureAndSrc(int atlasID, int textureID, SDL_Texture** texture, SDL_FRect* src){
    if(texture != NULL){
        *texture = atlasTextures[atlasID][atlasInfo[atlasID][textureID].subAtlasID];
    }

    if(src != NULL){
        *src = atlasInfo[atlasID][textureID].srcFRect;
    }
}

int setError(const char *fmt, ...){
    hasErrorOccurred = true;

    va_list args, copyOfArgs;
    va_start(args, fmt);

    va_copy(copyOfArgs, args);
    int sizeOfResultingString = vsnprintf(NULL, 0, fmt, copyOfArgs);
    va_end(copyOfArgs);

    if(sizeOfResultingString < 0){ //vsnprintf somehow fails
        va_end(args);
        return -1;
    }

    char* str = (char*)SDL_calloc((size_t)sizeOfResultingString + 2, sizeof(char)); //try to make it safer

    if(str == NULL){ //where memory
        va_end(args);
        return -1;
    }

    vsnprintf(str, (size_t)sizeOfResultingString + 1, fmt, args);
    va_end(args);

    printf("%s", str);
    if(showMessageBoxOnError){
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Early initialization error", str, NULL);
    }

    loadErrorStr = SDL_strdup(str);
    SDL_free(str);
    return 0;
}

SDL_Texture *loadText(TTF_Font *font, const char* text, SDL_Color fontColor){
    SDL_Surface *surface;
    surface = TTF_RenderText_Blended_Wrapped(font, text, 0, fontColor, 0);
    if(surface == NULL){
        setError("An error occurred while rendering text.\nError: %s\ntext: %s", SDL_GetError(), text);
        return NULL;
    }

    SDL_Texture *textToLoad = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if(textToLoad == NULL){
        setError("An error occurred while creating texture from surface.\nError: %s", SDL_GetError());
        return NULL;
    }

    return textToLoad;
}

SDL_Texture *autoCropLoadText(TTF_Font *font, const char* text, SDL_Color fontColor){ //use not recommended.
    SDL_Surface *surface;
    surface = TTF_RenderText_Blended_Wrapped(font, text, 0, fontColor, 0);
    if(surface == NULL){
        setError("An error occurred while rendering text.\nError: %s", SDL_GetError());
        return NULL;
    }

    SDL_Rect surfaceRange = {INT_MAX, INT_MAX, INT_MIN, INT_MIN};
    const Uint32 *pixels = (Uint32 *)surface->pixels;
    for(int y = 0; y < surface->h; y++){
        for(int x = 0; x < surface->w; x++){
            Uint8 a;//,r,g,b;
            int result = y * (surface->pitch / 4) + x;
            //printf("autocropIndex: %d\n", result);
            Uint32 pixel = pixels[result];
            a = pixel >> 24 & 0xFF;
            //r = pixel >> 16 & 0xFF;
            //g = pixel >> 8 & 0xFF;
            //b = pixel & 0xFF;
            if(a > 0){
                surfaceRange.x = SDL_min(x, surfaceRange.x);
                surfaceRange.y = SDL_min(y, surfaceRange.y);
                surfaceRange.w = SDL_max(x, surfaceRange.w);
                surfaceRange.h = SDL_max(y, surfaceRange.h);
            }
        }
    }

    //increase max size by 1
    surfaceRange.x = SDL_max(surfaceRange.x - 1, 0);
    surfaceRange.y = SDL_max(surfaceRange.y - 1, 0);
    surfaceRange.w = SDL_min(surfaceRange.w + 1, surface->w - 1);
    surfaceRange.h = SDL_min(surfaceRange.h + 1, surface->h - 1);

    if(surfaceRange.x == INT_MAX){
        surfaceRange.x = 0;
        surfaceRange.y = 0;
        surfaceRange.w = surface->w;
        surfaceRange.h = surface->h;
    } else {
        surfaceRange.w += 1;
        surfaceRange.h += 1;
    }


    //two line of code fix moment
    surfaceRange.w -= surfaceRange.x;
    surfaceRange.h -= surfaceRange.y;

    SDL_Surface *croppedSurface = SDL_CreateSurface(surfaceRange.w, surfaceRange.h, SDL_PIXELFORMAT_ARGB8888);
    if(croppedSurface == NULL){
        SDL_DestroySurface(surface);
        setError("An error occurred with croppedSurface.\nError: %s", SDL_GetError());
        return NULL;
    }

    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(surface, &surfaceRange, croppedSurface, NULL);

    SDL_DestroySurface(surface);
    SDL_Texture *textToLoad = SDL_CreateTextureFromSurface(renderer, croppedSurface);
    SDL_DestroySurface(croppedSurface);
    /*SDL_PropertiesID property = SDL_GetTextureProperties(textToLoad);
    Uint32 format;
    if (SDL_GetNumberProperty(property, SDL_PROP_TEXTURE_FORMAT_NUMBER, &format)) {
        printf("Format: %s (%u)\n", SDL_GetPixelFormatName(format), format);
    }*/

    if(textToLoad == NULL){
        setError("An error occurred while creating texture from surface.\nError: %s", SDL_GetError());
        return NULL;
    }

    return textToLoad;
}

int loadTextSimple(SDL_Texture **textToLoad, TTF_Font *font, const char* text, SDL_Color fontColor, bool autoCrop){
    destroyTextureIfNotNull(*textToLoad);
    if(autoCrop){
        *textToLoad = autoCropLoadText(font, text, fontColor);
    } else {
        *textToLoad = loadText(font, text, fontColor);
    }
    return *textToLoad == NULL ? -1 : 0;
}

int loadTextLocalized(SDL_Texture **textToLoad, TTF_Font *font, const char* text, SDL_Color fontColor, bool autoCrop){
    return loadTextSimple(textToLoad, font, langGetStr(text), fontColor, autoCrop);
}

//files
int loadImageEx(SDL_Texture **imageToLoad, const char *file, int scalerType, SDL_Rect rect, int atlasID, bool grayScale, Uint8 fadeBy){
    if(file == NULL){
        printf("file in loadImage is NULL!\n");
        return -2;
    } else if(*imageToLoad == NULL){
        reallocFilesIfFull();
        files[filesIndex].file = SDL_strdup(file);
        files[filesIndex].fileType = fileTypeImage;
        files[filesIndex].options.image.texture = imageToLoad;
        files[filesIndex].options.image.imageScaler = scalerType;
        files[filesIndex].options.image.id = atlasID;

        files[filesIndex].options.image.rect = rect;
        files[filesIndex].options.image.grayScale = grayScale;
        files[filesIndex].options.image.fadeBy = fadeBy;
        filesIndex++;
        return 0;
    }
    printf("imageToLoad is not NULL!\n");
    return -1;
}

int loadImage(SDL_Texture **imageToLoad, const char *file, int scalerType, SDL_Rect rect){
    return loadImageEx(imageToLoad, file, scalerType, rect, currentAtlasID, false, 0);
}

static bool simpleQueryTexture(SDL_Texture *texture, int *w, int *h){
    float texW = 0.f;
    float texH = 0.f;
    const bool result = SDL_GetTextureSize(texture, &texW, &texH);
    if(!result){
        return result;
    }

    if(w != NULL){
        *w = texW;
    }

    if(h != NULL){
        *h = texH;
    }

    return result;
}

int loadImageSimple(SDL_Texture **imageToLoad, const char *file, SDL_Rect rect){
    if(*imageToLoad != NULL){
        int w = 0;
        int h = 0;
        simpleQueryTexture(*imageToLoad, &w, &h);
        if(rect.w == w && rect.h == h){
            return 0;
        }
    }
    destroyTextureIfNotNull(*imageToLoad);
    return loadImage(imageToLoad, file, linear, rect);
}

int loadRawSVGSimple(SDL_Texture **imageToLoad, const char *SVGCode, SDL_Rect rect){
    if(SVGCode == NULL){
        printf("SVGCode in loadRawSVGSimple is NULL!\n");
        return -1;
    } else if(*imageToLoad != NULL){
        int w = 0;
        int h = 0;
        simpleQueryTexture(*imageToLoad, &w, &h);
        if(rect.w == w && rect.h == h){
            return 0;
        }
    }

    destroyTextureIfNotNull(*imageToLoad);
    reallocFilesIfFull();
    files[filesIndex].file = SDL_strdup(SVGCode);
    files[filesIndex].fileType = fileTypeRawSVG;
    files[filesIndex].options.rawSVG.texture = imageToLoad;
    files[filesIndex].options.rawSVG.rect = rect;
    files[filesIndex].options.rawSVG.id = currentAtlasID;
    filesIndex++;
    return 0;
}

int loadSound(MIX_Audio **sfxToLoad, const char *file){
    if(file == NULL){
        printf("file in loadSound is NULL!\n");
        return -2;
    } else if(*sfxToLoad == NULL){
        reallocFilesIfFull();
        files[filesIndex].file = SDL_strdup(file);
        files[filesIndex].fileType = fileTypeSound;
        files[filesIndex].options.audio.sfx = sfxToLoad;
        filesIndex++;
        return 0;
    }
    printf("sfxToLoad is not NULL!\n");
    return -1;
}

int loadAllThings(void){
    static int currentIndex = 0;

    if(hasErrorOccurred){
        return -1;
    }

    if(currentIndex == filesIndex){
        return 100;
    }

    showMessageBoxOnError = false;

    if_unlikely(currentIndex >= filesIndex){
        return setError("currentIndex (%d) is larger than filesIndex (%d).", currentIndex, filesIndex);
    }

    const size_t prefixResultLen = SDL_strlen(basePathLocation) + SDL_strlen(files[currentIndex].file) + 4;
    char *prefixResult = SDL_calloc(prefixResultLen, sizeof(char));
    switch(files[currentIndex].fileType){
        case fileTypeImage:
            SDL_snprintf(prefixResult, prefixResultLen, "%s%s", basePathLocation, files[currentIndex].file);

            const bool isSVG = !SDL_strcmp(strrchr(prefixResult, '\0') - 4, ".svg");
            const int imageWidth = files[currentIndex].options.image.rect.w;
            const int imageHeight = files[currentIndex].options.image.rect.h;
            int svgMultiplier = 1;
            if(isSVG){ //try to reduce gradient banding
                const int maxSize = SDL_max(imageWidth, imageHeight);
                if(maxSize < 115){
                    svgMultiplier = 3;
                } else if(maxSize < 250){
                    svgMultiplier = 2;
                }
            }

            SDL_Surface *surface = NULL;
            if(isSVG){
                SDL_IOStream* svgRWops = SDL_IOFromFile(prefixResult, "rb");
                surface = IMG_LoadSizedSVG_IO(svgRWops, imageWidth * svgMultiplier, imageHeight * svgMultiplier);
                SDL_CloseIO(svgRWops);
            } else {
                surface = IMG_Load(prefixResult);
            }

            if_unlikely(surface == NULL){
                hasErrorOccurred = true;
                SDL_free(prefixResult);
                return setError("An error occurred while loading images.\nFile: %s\nError: %s", files[currentIndex].file, SDL_GetError());
            }

            double scaleX = 1.0 / svgMultiplier;
            double scaleY = 1.0 / svgMultiplier;
            if(!isSVG){
                scaleX = (double)surface->w / (double)imageWidth;
                scaleY = (double)surface->h / (double)imageHeight;
                double finalScale = SDL_min(scaleX, scaleY); //scaleX > scaleY ? yScale : xScale;
                if(finalScale <= 1){
                    finalScale = 1;
                }
                scaleX = 1 / finalScale;
                scaleY = 1 / finalScale;
            }

            modifySurface(&surface, scaleX, scaleY, files[currentIndex].options.image.grayScale, files[currentIndex].options.image.fadeBy);

            const int id = files[currentIndex].options.image.id;
            if(id >= 0){
                atlaSurfaces[id][lenOfAtlaSurfaces[id]] = surface;
                lenOfAtlaSurfaces[id]++;
            } else {
                *files[currentIndex].options.image.texture = SDL_CreateTextureFromSurface(renderer, surface);

                SDL_DestroySurface(surface); //nightmare has ended
                SDL_SetTextureScaleMode(*files[currentIndex].options.image.texture, files[currentIndex].options.image.imageScaler);
            }
            break;
        case fileTypeSound:
            SDL_snprintf(prefixResult, prefixResultLen, "%s%s", basePathLocation, files[currentIndex].file);

            MIX_Audio* SfxResult = MIX_LoadAudio(NULL, prefixResult, true); //api moment

            if_unlikely(SfxResult == NULL){
                SDL_free(prefixResult);
                return setError("An error occurred while loading sound effect.\nFile: %s\nError: %s", files[currentIndex].file, SDL_GetError());
            }

            *files[currentIndex].options.audio.sfx = SfxResult;
            break;
        case fileTypeRawSVG:{
            SDL_IOStream *svgRWops = SDL_IOFromConstMem(files[currentIndex].file, SDL_strlen(files[currentIndex].file));
            SDL_Surface* SVGSurface = IMG_LoadSizedSVG_IO(svgRWops, files[currentIndex].options.rawSVG.rect.w, files[currentIndex].options.rawSVG.rect.h);
            SDL_CloseIO(svgRWops);

            if_unlikely(SVGSurface == NULL){
                hasErrorOccurred = true;
                const int result = setError("An error occurred while loading images.\nFile: %s\nError: %s", prefixResult, SDL_GetError());
                SDL_free(prefixResult);
                return result;
            }

            const int svgID = files[currentIndex].options.rawSVG.id;
            if(svgID >= 0){
                atlaSurfaces[svgID][lenOfAtlaSurfaces[svgID]] = SVGSurface;
                lenOfAtlaSurfaces[svgID]++;
            } else {
                *files[currentIndex].options.rawSVG.texture = SDL_CreateTextureFromSurface(renderer, SVGSurface);

                SDL_DestroySurface(SVGSurface); //nightmare has ended
            }
            break;
        }
        case fileTypeAtlasText:
            //todo:
            break;
        default:
            SDL_free(prefixResult);
            SDL_free(files[currentIndex].file);
            return setError("Invalid file type.");
    }

    SDL_free(prefixResult);
    SDL_free(files[currentIndex].file);
    currentIndex++;
    if(currentIndex == filesIndex){
        currentIndex = 0;
        return 100;
    } else {
        return SDL_min(currentIndex * 100 / filesIndex, 99); //prevent starting with some things not loaded
    }

    return setError("percentage not returned");
}

SDL_Texture *loadingScreenText = NULL;
SDL_Texture *errorText = NULL;

int percentage = 0;
int slotAfterLoad = 0;
bool showProgress = false;

static int renderLoadScreen(void){
    destroyTextureIfNotNull(loadingScreenText);
    if(errorText == NULL){
        char loadScreen[32];
        simple_snprintf(loadScreen, "Loading %d%%", percentage);
        loadingScreenText = loadText(mediumFont, loadScreen, white);
        renderTextureSimpler(loadingScreenText, getCenterX(), getCenterY());
    } else {
        renderTextureSimpler(errorText, getCenterX(), getCenterY());
    }
    return 0;
}

static Uint64 loadingScreen(Uint64 delta) {
    UNUSED(delta);
    renderFunction = renderLoadScreen;
    static Uint64 oldTime = 0;
    static Uint64 newTime = 0;

    if(errorText == NULL){
        percentage = loadAllThings();
        if(percentage == loadError){
            errorText = loadText(smallFont, loadErrorStr, white);
            return 0;
        }

        if(percentage == 100){
            isLoadImagePart = false;
            if(filesIndex > 0){
                SDL_free(files);
                files = NULL; //real moment
            }
            filesIndex = 0;

            for(int i = 0; i < stackArraySizeInt(lenOfAtlaSurfaces); i++){
                if(lenOfAtlaSurfaces[i] <= 0){
                    continue;
                }

                SDL_free(atlasInfo[i]);
                SDL_Rect *rects = SDL_malloc(sizeof(SDL_Rect) * lenOfAtlaSurfaces[i]);
                SDL_Surface* packedSurface = packSurfaces(atlaSurfaces[i], &rects, lenOfAtlaSurfaces[i], 256, maxTextureSize, atlaSurfacePaddings[i]);
                if(packedSurface == NULL){
                    SDL_free(rects);
                    char packError[64] = {0};
                    simple_snprintf(packError, "Atlas required is too big (max texture size: %d).", maxTextureSize);
                    errorText = loadText(smallFont, packError, white);
                    return 0;
                }

                /*char fileName[64] = {0};
                simple_snprintf(fileName, "/tmp/garbage%u%u.png", SDL_rand(99999), SDL_rand(99999));
                IMG_SavePNG(packedSurface, fileName);*/

                atlasInfo[i] = SDL_malloc(sizeof(atlasItem) * lenOfAtlaSurfaces[i]);

                for(int n = 0; n < lenOfAtlaSurfaces[i]; n++){
                    atlasInfo[i][n].subAtlasID = 0;
                    atlasInfo[i][n].srcFRect.x = rects[n].x;
                    atlasInfo[i][n].srcFRect.y = rects[n].y;
                    atlasInfo[i][n].srcFRect.w = rects[n].w;
                    atlasInfo[i][n].srcFRect.h = rects[n].h;
                }

                SDL_free(rects);
                lenOfAtlaSurfaces[i] = 0;
                atlasTextures[i][0] = SDL_CreateTextureFromSurface(renderer, packedSurface);
                SDL_DestroySurface(packedSurface);
            }
            currentSlot = slotAfterLoad;
            destroyTextureIfNotNull(loadingScreenText);
            oldTime = 0;
            return 0;
        }
    }

    newTime = SDL_GetTicks();
    if(oldTime == 0){
        oldTime = newTime;
    }

    if(errorText == NULL && ((newTime - oldTime) < 500 || !showProgress)){
        reRunGameFunction = true;
    } else {
        oldTime = newTime;
    }
    return 0;
}

void runLoadOnly(int slot, bool shouldShowProgress){
    //hack
    currentSlot = loadingScreenSlot;
    addGameFunctionToCurrentSlot(loadingScreen);
    slotAfterLoad = slot;
    showProgress = shouldShowProgress;
}

void runLoadFunction(void (*loadFunc)(int slot), int slot){
    currentSlot = slot;
    reloadFunction = loadFunc;
    isLoadImagePart = true;
    (*loadFunc)(slot);
    isLoadImagePart = false;
    (*loadFunc)(slot);

    runLoadOnly(slot, slot != mainMenuSlot);
}
