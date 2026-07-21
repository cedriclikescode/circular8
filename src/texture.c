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
#include <stdbool.h>

#include <SDL3/SDL.h>

#include "texture.h"
#include "miscutils.h"


//use sdl's malloc and free
#define STBIR_MALLOC(size,user_data) ((void)(user_data), SDL_malloc(size))
#define STBIR_FREE(ptr,user_data)    ((void)(user_data), SDL_free(ptr))

//prevent useless warnings (afaik)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wshift-overflow"
#pragma GCC diagnostic ignored "-Woverflow"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif


#define STB_IMAGE_RESIZE_IMPLEMENTATION

//more compile time wasted
#include "thirdparty/stb/stb_image_resize2.h"


#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define STB_RECT_PACK_IMPLEMENTATION
#include "thirdparty/stb/stb_rect_pack.h"

int maxTextureSize = 0;
SDL_Texture* nullTexture = NULL;

int modifySurface(SDL_Surface **image, double scaleX, double scaleY, bool grayScale, Uint8 fadeBy){
    const int oldImageWidth = (*image)->w;
    const int oldImageHeight = (*image)->h;
    const int newImageWidth = SDL_round(oldImageWidth * scaleX);
    const int newImageHeight = SDL_round(oldImageHeight * scaleY);
    if(oldImageWidth == newImageWidth &&
       oldImageHeight == newImageHeight &&
       !grayScale &&
       fadeBy == 0){ //do nothing
        return 0;
    }

    if_unlikely(image == NULL){
        return -1;
    }

    SDL_Surface *oldRGBASurface = SDL_ConvertSurface(*image, SDL_PIXELFORMAT_ARGB8888);

    SDL_DestroySurface(*image); //replacement with new surface

    //use pitch, not 0 to prevent errors?
    SDL_Surface *newSurface = SDL_CreateSurface(newImageWidth, newImageHeight, SDL_PIXELFORMAT_ARGB8888);

    stbir_resize(oldRGBASurface->pixels, oldRGBASurface->w, oldRGBASurface->h, oldRGBASurface->pitch,
                              newSurface->pixels, newSurface->w, newSurface->h, newSurface->pitch,
                              STBIR_4CHANNEL, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_MITCHELL);
    SDL_DestroySurface(oldRGBASurface);



    *image = newSurface;
    return 0;
}

SDL_Surface* packSurfaces(SDL_Surface* surfaces[], SDL_Rect* surfaceLocations[], int rectCount, int minSize, int maxSize, int padding){
    int mainSize = minSize;

    for(int i = 0; mainSize <= maxSize; i++){
        int width = mainSize;
        int height = mainSize;

        switch((i / 2) % 4){ //try different width and height of powers of 2 to reduce memory while being 2^x
            case 0:
                break;
            case 1:
                width *= 2;
                break;
            case 2:
                height *= 2;
                break;
            case 3:
                width *= 2;
                height *= 2;
                mainSize *= 2;
                break;
            default:
                break;
        }

        if(width > maxSize || height > maxSize){
            break;
        }

        stbrp_context context = {0};
        stbrp_node* nodes = SDL_malloc(sizeof(stbrp_node) * width);
        stbrp_init_target(&context, width, height, nodes, width);

        if(i % 2 == 0){ //try different heuristics to fit texture better sometimes
            stbrp_setup_heuristic(&context, STBRP_HEURISTIC_Skyline_BL_sortHeight);
        } else {
            stbrp_setup_heuristic(&context, STBRP_HEURISTIC_Skyline_BF_sortHeight);
        }

        stbrp_rect* packedRects = SDL_malloc(rectCount * sizeof(stbrp_rect)); //is malloc fine?
        for(int n = 0; n < rectCount; n++){
            packedRects[n].w = surfaces[n]->w + padding * 2;
            packedRects[n].h = surfaces[n]->h + padding * 2;
        }

        bool isSuccessful = stbrp_pack_rects(&context, packedRects, rectCount);
        SDL_free(nodes);
        if(isSuccessful){
            SDL_Surface* packedSurface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_ARGB8888);
            for(int n = 0; n < rectCount; n++){
                SDL_SetSurfaceBlendMode(surfaces[n], SDL_BLENDMODE_NONE);
                const SDL_Rect surfaceRect = {packedRects[n].x + padding, packedRects[n].y + padding, packedRects[n].w - padding * 2, packedRects[n].h - padding * 2};
                if(surfaceLocations != NULL){
                    (*surfaceLocations)[n] = surfaceRect;
                }
                SDL_BlitSurface(surfaces[n], NULL, packedSurface, &surfaceRect);
                SDL_DestroySurface(surfaces[n]);
            }
            SDL_free(packedRects);
            return packedSurface;
        }
        SDL_free(packedRects);
    }
    return NULL;
}
