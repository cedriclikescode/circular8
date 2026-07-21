#pragma once
#ifndef Circular8_TEXTURE_H
#define Circular8_TEXTURE_H

#include <SDL3/SDL.h>
#include "miscutils.h"
int modifySurface(SDL_Surface **image, double scaleX, double scaleY, bool grayScale, Uint8 fadeBy);
#define destroyTextureIfNotNull(tex) do {if((tex) != NULL){SDL_DestroyTexture(tex);tex = NULL;}} while(0)
#define destroyStackArrayOfTextures(array) do {for(unsigned int i = 0; i < stackArraySize(array); i++){destroyTextureIfNotNull(array[i]);} } while(0)
//bool packRectangles(SDL_Rect rectangles[], int rectCount, int minSize, int maxSize);
SDL_Surface* packSurfaces(SDL_Surface* surfaces[], SDL_Rect* surfaceLocations[], int rectCount, int minSize, int maxSize, int padding);
extern SDL_Texture* nullTexture;
extern int maxTextureSize;
#endif // Circular8_TEXTURE_H
