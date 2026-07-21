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

//This file contains some code taken from testffmpeg.c in SDL3 tests folder which is under the following license:

/*
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#ifdef Circular8_ENABLE_FFmpeg
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "render.h"
#include "texture.h"
#include "video.h"
#include "miscutils.h"

bool isShowingVideo = false;
bool isVideoEnabled = false;
static bool mustEndVideoThread = false;
int numberOfFrameBuffers = 8;
videoFrameInfo frameBuffers[32] = {0};
static int offsetVideoI = 0;
static int videoHealth = 0;
static SDL_Mutex* videoMutex;
static SDL_Thread* videoThread;
SDL_Texture* currentVideoTexture = NULL;
static AVFormatContext *videoFormatContext = NULL;
static AVCodecContext *videoCodecContext = NULL;
static AVFrame *videoFrame = NULL;
static AVPacket *videoPacket = NULL;
static double nextVideoTime = -9999999;
static int indexOfVideoStream = -1;

static double getVideoNextTimestamp(AVFrame* frame){
    const int64_t pts = frame->best_effort_timestamp;
    const double time_in_seconds = pts * av_q2d(videoFormatContext->streams[indexOfVideoStream]->time_base);
    return time_in_seconds;
}

static int decodeVideoOnAnotherThread(void* data){
    UNUSED(data);
    bool isRunning = true;

    while(isRunning){
        SDL_Delay(clampInt(numberOfFrameBuffers * 2, 4, 16)); //proportional delay for increased battery savings?

        SDL_LockMutex(videoMutex);

        if_unlikely(mustEndVideoThread){ //first condition or else bugs
            SDL_UnlockMutex(videoMutex);
            isRunning = false;
            break;
        }

        if(videoHealth >= numberOfFrameBuffers - 1){
            SDL_UnlockMutex(videoMutex);
            continue;
        }

        int i = (offsetVideoI + videoHealth) % numberOfFrameBuffers; // more bugs caused by forgetting to add a value
        SDL_UnlockMutex(videoMutex);
        int count = 0; //prevent mutex overloading
        bool isSuccess = true;
        while(true){
            if(isSuccess){
                i = (i + 1) % numberOfFrameBuffers;
                isSuccess = false;
            }
            count++;
            while(true){
                const int readFrameCode = av_read_frame(videoFormatContext, videoPacket);
                if(readFrameCode >= 0 || readFrameCode == AVERROR_EOF){
                    if (videoPacket->stream_index == indexOfVideoStream) {
                        avcodec_send_packet(videoCodecContext, readFrameCode == AVERROR_EOF ? NULL : videoPacket);

                        int returnCode = avcodec_receive_frame(videoCodecContext, videoFrame);
                        if(returnCode == AVERROR(EAGAIN)){
                            continue;
                        }

                        if_unlikely(returnCode == AVERROR_EOF){
                            count = 999999;
                            isRunning = false;
                            break;
                        }

                        nextVideoTime = getVideoNextTimestamp(videoFrame);
                        isSuccess = true;
                        const bool isYUV420P = videoFrame->format == AV_PIX_FMT_YUV420P; //different formula if YUV422P or YUV444P
                        const size_t YPlaneSize = videoFrame->linesize[0] * videoFrame->height;
                        const size_t UPlaneSize = videoFrame->linesize[1] * (isYUV420P ? (videoFrame->height / 2) : videoFrame->height);
                        const size_t VPlaneSize = videoFrame->linesize[2] * (isYUV420P ? (videoFrame->height / 2) : videoFrame->height);

                        SDL_LockMutex(videoMutex);
                        frameBuffers[i].displayTime = nextVideoTime;
                        frameBuffers[i].Ypitch = videoFrame->linesize[0];
                        frameBuffers[i].Upitch = videoFrame->linesize[1];
                        frameBuffers[i].Vpitch = videoFrame->linesize[2];

                        //malloc moment
                        //save on needless mallocs
                        if_unlikely(frameBuffers[i].Yplane != NULL && YPlaneSize != frameBuffers[i].oldYPlaneSize){
                            SDL_free(frameBuffers[i].Yplane);
                            frameBuffers[i].Yplane = NULL;
                        }

                        if_unlikely(frameBuffers[i].Uplane != NULL && UPlaneSize != frameBuffers[i].oldUOrVPlaneSize){
                            SDL_free(frameBuffers[i].Uplane);
                            SDL_free(frameBuffers[i].Vplane);
                            frameBuffers[i].Uplane = NULL;
                            frameBuffers[i].Vplane = NULL;
                        }

                        if_unlikely(frameBuffers[i].Yplane == NULL){
                            frameBuffers[i].Yplane = SDL_malloc(YPlaneSize);
                        }

                        if_unlikely(frameBuffers[i].Uplane == NULL){
                            frameBuffers[i].Uplane = SDL_malloc(UPlaneSize);
                        }

                        if_unlikely(frameBuffers[i].Vplane == NULL){
                            frameBuffers[i].Vplane = SDL_malloc(VPlaneSize);
                        }

                        frameBuffers[i].oldYPlaneSize = YPlaneSize;
                        frameBuffers[i].oldUOrVPlaneSize = UPlaneSize;

                        SDL_memcpy(frameBuffers[i].Yplane, videoFrame->data[0], YPlaneSize);
                        SDL_memcpy(frameBuffers[i].Uplane, videoFrame->data[1], UPlaneSize);
                        SDL_memcpy(frameBuffers[i].Vplane, videoFrame->data[2], VPlaneSize);

                        videoHealth++;
                        SDL_UnlockMutex(videoMutex);

                        av_packet_unref(videoPacket);
                        break;
                    }
                    av_packet_unref(videoPacket);
                }
            }

            if(count >= 3 + numberOfFrameBuffers / 2){
                break;
            } else {
                SDL_LockMutex(videoMutex);
                if(videoHealth >= numberOfFrameBuffers){
                    SDL_UnlockMutex(videoMutex);
                    break;
                }
                SDL_UnlockMutex(videoMutex);
            }
        };
    }
    //printf("video thread dying\n");
    return 0;
}

void endVideoThread(void){
    SDL_LockMutex(videoMutex); //null does nothing
    if(videoMutex != NULL){
        mustEndVideoThread = true;
        SDL_UnlockMutex(videoMutex);
        SDL_WaitThread(videoThread, NULL);
        SDL_DestroyMutex(videoMutex);
        videoMutex = NULL;
        mustEndVideoThread = false;
    }
}

void closeVideo(void){
    if(videoCodecContext != NULL){
        avformat_close_input(&videoFormatContext);
    }

    if(videoCodecContext != NULL){
        avcodec_free_context(&videoCodecContext);
    }

    if(videoFrame != NULL){
        av_frame_free(&videoFrame);
    }

    for(unsigned int i = 0; i < stackArraySize(frameBuffers); i++){
        frameBuffers[i].displayTime = INT_MAX;
        frameBuffers[i].oldYPlaneSize = 0;
        frameBuffers[i].oldUOrVPlaneSize = 0;

        //malloc and double free moment
        if(frameBuffers[i].Yplane != NULL){
            SDL_free(frameBuffers[i].Yplane);
            frameBuffers[i].Yplane = NULL;
        }

        if(frameBuffers[i].Uplane != NULL){
            SDL_free(frameBuffers[i].Uplane);
            frameBuffers[i].Uplane = NULL;
        }

        if(frameBuffers[i].Vplane != NULL){
            SDL_free(frameBuffers[i].Vplane);
            frameBuffers[i].Vplane = NULL;
        }
    }

    if(videoPacket != NULL){
        av_packet_free(&videoPacket);
    }

    destroyTextureIfNotNull(currentVideoTexture);
    isShowingVideo = false;
}

void loadVideo(const char* file){
    nextVideoTime = -9999999;
    offsetVideoI = 0;
    videoHealth = 0;
    const int fileOpenResult = avformat_open_input(&videoFormatContext, file, NULL, NULL);
    if(fileOpenResult < 0){
        printf("video opening error: %d\n", fileOpenResult);
        closeVideo();
        return;
    }

    const AVCodec* videoDecoder = NULL;
    indexOfVideoStream = av_find_best_stream(videoFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
    if(indexOfVideoStream < 0){
        printf("best video stream not found: %d\n", indexOfVideoStream);
        closeVideo();
        return;
    }

    videoCodecContext = avcodec_alloc_context3(videoDecoder);
    if(videoCodecContext == NULL){
        printf("context3 failed\n");
        closeVideo();
        return;
    }

    videoCodecContext->thread_count = 0;
    videoCodecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    const int parametersToContextResult = avcodec_parameters_to_context(videoCodecContext, videoFormatContext->streams[indexOfVideoStream]->codecpar);
    if(parametersToContextResult < 0){
        printf("parameters to context failed: %d\n", parametersToContextResult);
        closeVideo();
        return;
    }

    const int streamInfoResult = avformat_find_stream_info(videoFormatContext, NULL);

    if(streamInfoResult < 0){
        printf("find stream info failed: %d\n", streamInfoResult);
        closeVideo();
        return;
    }

    const int open2Result = avcodec_open2(videoCodecContext, videoDecoder, NULL);

    if(open2Result < 0){
        printf("open2 failed: %d\n", open2Result);
        closeVideo();
        return;
    }


    videoPacket = av_packet_alloc();
    if(videoPacket == NULL){
        puts("packet allocation failed");
        closeVideo();
        return;
    }

    videoFrame = av_frame_alloc();
    if(videoFrame == NULL){
        puts("frame allocation failed");
        closeVideo();
        return;
    }

    for(unsigned int i = 0; i < stackArraySize(frameBuffers); i++){
        frameBuffers[i].displayTime = INT_MAX;
    }
    isShowingVideo = true;

    videoMutex = SDL_CreateMutex();
    if(videoMutex == NULL){
        printf("video mutex failed to create: %s\n", SDL_GetError());
        SDL_DestroyMutex(videoMutex);
        closeVideo();
        return;
    }

    currentVideoTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, // matches YUV420p
    videoCodecContext->width, videoCodecContext->height); //todo: add support for textures that are not SDL_PIXELFORMAT_IYUV
    if(currentVideoTexture == NULL){
        printf("currentVideoTexture ERROR: %s\n", SDL_GetError());
        SDL_DestroyMutex(videoMutex);
        closeVideo();
        return;
    }

    videoThread = SDL_CreateThread(decodeVideoOnAnotherThread, "Video", NULL);
}

static void updateOffsetVideoI(double timestampInSeconds){
    int damage = 1;
    for(int i = (offsetVideoI + 1) % numberOfFrameBuffers; i != offsetVideoI && (videoHealth - damage) >= 0; i = (i + 1) % numberOfFrameBuffers){
        if(frameBuffers[i].displayTime - timestampInSeconds <= 0){
            offsetVideoI = i;
            videoHealth -= damage;
            SDL_UpdateYUVTexture(currentVideoTexture, NULL,
                    frameBuffers[offsetVideoI].Yplane, frameBuffers[offsetVideoI].Ypitch,
                    frameBuffers[offsetVideoI].Uplane, frameBuffers[offsetVideoI].Upitch,
                    frameBuffers[offsetVideoI].Vplane, frameBuffers[offsetVideoI].Vpitch);
            break;
        }
        damage++;
    }
    //printf("%d %d\n", videoHealth, offsetVideoI);
}

void renderVideoFit(double timestampInSeconds, Uint8 brightness){
    SDL_LockMutex(videoMutex);
    updateOffsetVideoI(timestampInSeconds);

    if(offsetVideoI < 0){
        SDL_UnlockMutex(videoMutex);
        return;
    }

    SDL_UnlockMutex(videoMutex);

    setTextureColor(currentVideoTexture, brightness, brightness, brightness);
    float texW, texH;
    if(SDL_GetTextureSize(currentVideoTexture, &texW, &texH)){
        const float multiplier = screenSize / SDL_max(texW, texH);

        const SDL_FRect videoFRect = FRectWithTexSimple(currentVideoTexture, getCenterX(), getCenterY(), multiplier, true);
        SDL_RenderTexture(renderer, currentVideoTexture, NULL, &videoFRect);
    }
}
#else
typedef const int nothing; //hack to stop empty translation unit warnings
#endif // Circular8_ENABLE_FFmpeg
