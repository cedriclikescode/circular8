#pragma once
#ifndef Circular8_VIDEO_H
#define Circular8_VIDEO_H
#ifdef Circular8_ENABLE_FFmpeg
#include <SDL3/SDL.h>
#include <stdbool.h>
extern bool isShowingVideo;
extern bool isVideoEnabled;
extern SDL_Texture* currentVideoTexture;
extern int numberOfFrameBuffers;

void loadVideo(const char* file);

void endVideoThread(void);
void closeVideo(void);

void renderVideoFit(double timestampInSeconds, Uint8 brightness);
typedef struct videoFrameInfo {
    double displayTime;
    size_t oldYPlaneSize;
    size_t oldUOrVPlaneSize;
    Uint8 *Yplane;
    Uint8 *Uplane;
    Uint8 *Vplane;
    int Ypitch;
    int Upitch;
    int Vpitch;
} videoFrameInfo;
extern videoFrameInfo frameBuffers[32];
#endif // Circular8_ENABLE_FFmpeg
#endif // Circular8_VIDEO_H
