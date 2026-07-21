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

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "main.h"
#include "render.h"
#include "events.h"
#include "texture.h"
#include "gameinit.h"
#include "load.h"
#include "miscutils.h"
#ifdef Circular8_ENABLE_FFmpeg
#include "video.h"
#endif // Circular8_ENABLE_FFmpeg

#include "game/songselect.h"
#include "game/chart.h"
#include "game/ingame.h"
#include "game/profileselect.h"
#include "game/options.h"
#include "game/difficultyselect.h"

#ifdef Circular8_ENABLE_CONTROLLER
#include "controller/touch.h"
#endif // Circular8_ENABLE_CONTROLLER

SDL_Texture* difficultyCircle = NULL;
SDL_Texture* difficultyHighlight = NULL;

SDL_Texture* difficultyTextures[7] = {0};
SDL_Texture* textLV = NULL;
int selectedDifficulty = 1;
static int previousDifficulty = 0;
static int nextDifficulty = 2;
static char* difficultyLevelsCharCache[7];

static const int decreaseDiffButton = 7;
static const int increaseDiffButton = 2;
SDL_Texture* decreaseDifficultyText = NULL;
SDL_Texture* increaseDifficultyText = NULL;

static Uint64 highlightRotationTime = 0;
//is this bad
#define highlightRotationMax (Uint64)(5 * gameTicksPerSecond)

static int renderDifficultySelect(void) {
    renderTexture(menuBackground, NULL, NULL);
    renderSongBorder(0.45f, currentSongIndex, true);

    const SDL_FPoint difficultyCirclesFPoints[7] = {
        {-162, -180}, //easy
        {-162, -72}, //basic
        {-54, -72}, //advanced
        {54, -72}, //expert
        {162, -72}, //master
        {162, -180}, //re:master
        {0, -228} //original
    };

    const int len = stackArraySizeInt(difficultyCirclesFPoints);
    bool isThereAChart = false;

    //difficulty circles
    for(int i = 0; i < len; i++){
        if(songs[currentSongIndex].difficultyStartIndexes[i] < 2){
            continue;
        }

        isThereAChart = true;

        const float scaledX = getCenterX() + (difficultyCirclesFPoints[i].x) * guiScale;
        const float scaledY = getCenterY() + (difficultyCirclesFPoints[i].y) * guiScale;
        const SDL_Color diffColor = songs[currentSongIndex].difficultyColors[i];
        setTextureColor(difficultyCircle, diffColor.r, diffColor.g, diffColor.b);
        renderTextureSimpler(difficultyCircle, scaledX, scaledY);
    }

    //lv text
    for(int i = 0; i < len; i++){
        if(songs[currentSongIndex].difficultyStartIndexes[i] < 2){
            continue;
        }

        const char* diffLevel = songs[currentSongIndex].difficultyLevels[i];
        if(diffLevel != NULL && SDL_strnlen(diffLevel, 1)){
            const float scaledX = getCenterX() + (difficultyCirclesFPoints[i].x) * guiScale;
            const float scaledY = getCenterY() + (difficultyCirclesFPoints[i].y) * guiScale;
            const SDL_FRect LVFRect = FRectWithTexSimple(textLV, scaledX, scaledY - 21.f * guiScale, 1.f, true);
            renderTexture(textLV, NULL, &LVFRect);
        }
    }

    //difficulty lv
    for(int i = 0; i < len; i++){
        if(songs[currentSongIndex].difficultyStartIndexes[i] < 2){
            continue;
        }

        const float scaledX = getCenterX() + (difficultyCirclesFPoints[i].x) * guiScale;
        const float scaledY = getCenterY() + (difficultyCirclesFPoints[i].y) * guiScale;

        const char* diffLevel = songs[currentSongIndex].difficultyLevels[i];
        if(diffLevel != NULL && SDL_strnlen(diffLevel, 1)){
            bool isNew = difficultyTextures[i] == NULL;
            if(!isNew && SDL_strcmp(difficultyLevelsCharCache[i], diffLevel) == 0){
                destroyTextureIfNotNull(difficultyTextures[i]);
                SDL_free(difficultyLevelsCharCache[i]);
                isNew = true;
            }

            if(isNew){
                difficultyLevelsCharCache[i] = SDL_strdup(diffLevel);
                //const SDL_Color gray = {72, 72, 72, 255};
                difficultyTextures[i] = loadText(mediumFont, difficultyLevelsCharCache[i], white);
            }


            const SDL_FRect diffFRect = FRectWithTexSimple(difficultyTextures[i], scaledX, scaledY + 7.f * guiScale, 1.f, true);
            renderTexture(difficultyTextures[i], NULL, &diffFRect);
        }

        if(i == selectedDifficulty){
            const SDL_FRect diffHighlightFRect = FRectWithTexSimple(difficultyHighlight, scaledX, scaledY, 1.f, true);
            renderTextureRotated(difficultyHighlight, NULL, &diffHighlightFRect, (double)highlightRotationTime / (double)highlightRotationMax * 360.0, NULL, SDL_FLIP_NONE);
        }
    }

    if(selectedDifficulty > 0 && previousDifficulty >= 0){ //Basic or higher
        const SDL_Color easierColor = songs[currentSongIndex].difficultyColors[previousDifficulty];
        renderButtonTextBox(decreaseDiffButton, decreaseDifficultyText, easierColor.r, easierColor.g, easierColor.b);
    }

    if(selectedDifficulty < 6 && nextDifficulty <= 6){ // Re:Master or lower
        const SDL_Color harderColor = songs[currentSongIndex].difficultyColors[nextDifficulty];
        renderButtonTextBox(increaseDiffButton, increaseDifficultyText, harderColor.r, harderColor.g, harderColor.b);
    }

    const SDL_Color backColor = buttonBoxColors[backButtonBoxI - 1];
    renderButtonTextBox(backButtonBoxI, buttonBoxTexts[backButtonBoxI - 1], backColor.r, backColor.g, backColor.b);

    if(isThereAChart){ //in case there is chart with 0 difficulties
        const SDL_Color nextColor = buttonBoxColors[nextButtonBoxI - 1];
        renderButtonTextBox(nextButtonBoxI, buttonBoxTexts[nextButtonBoxI - 1], nextColor.r, nextColor.g, nextColor.b);
    }
    return 0;
}

Uint64 difficultySelect(Uint64 delta){
    static Uint64 timeInDifficultySelect = 0;
    timeInDifficultySelect += delta;
    renderFunction = renderDifficultySelect;
    subscreenRenderFunction = renderUserInfoOnSubscreen;

    highlightRotationTime += delta;
    highlightRotationTime %= highlightRotationMax;

    int diffChange = 0;

    if(isButtonOrTouchPressed(nextButtonBoxI) && timeInDifficultySelect >= (Uint64)gameTicksPerSecond / 2 && songs[currentSongIndex].difficultyStartIndexes[selectedDifficulty] > 2){ //last condition stops crashes when no difficulty in maidata.txt
        timeInDifficultySelect = 0;
        highlightRotationTime = 0;
        printf("diff selected: %d\n", selectedDifficulty);

        MIX_DestroyAudio(chartAudio);
        MIX_DestroyTrack(chartSong);
        chartAudio = NULL;
        chartSong = NULL;
        for(unsigned int i = 0; i < stackArraySize(ingameTracks); i++){
            MIX_DestroyTrack(ingameTracks[i]);
            ingameTracks[i] = MIX_CreateTrack(mainMixer);
            MIX_SetTrackGain(ingameTracks[i], gameplaySfxVolume);
        }

        const char* chartLocation = songs[currentSongIndex].chartLocation;

        //chart loading benchmark
        /*Uint64 test = SDL_GetTicksNS();
        for(int i = 0; i <= 0; i++){
            chartErrorRange errorTest = readChart(songs[currentSongIndex].difficultyStartIndexes[selectedDifficulty],
                                                  chartLocation,
                                                  songs[currentSongIndex].checksumOfFile);
            if_unlikely(errorTest.length > 0){
                printf("ERROR\n");
                runLoadFunction(loadInGame, inGameSlot); //hack
                return 0;
            } else if(i % 1000 == 0 && i != 0){
                const Uint64 newTicks = SDL_GetTicksNS();
                printf("i %d (%.2f)\n", i, (newTicks - test) / 1000000000.0);
                test = newTicks;
                //SDL_Delay(1200);
                //printf("next\n");
            }
        }*/

        chartErrorRange chartError = readChart(songs[currentSongIndex].difficultyStartIndexes[selectedDifficulty],
                                               chartLocation,
                                               songs[currentSongIndex].checksumOfFile);
        if_unlikely(chartError.length > 0){
            printf("ERROR\n");
            runLoadFunction(loadInGame, inGameSlot); //hack
            return 0;
        }

        //audio
        const size_t locationMaxSize = SDL_strlen(chartLocation) + 32;
        char* fileLocation = SDL_malloc(locationMaxSize * sizeof(char));
        if(songs[currentSongIndex].hasOGG){
            SDL_snprintf(fileLocation, locationMaxSize, "%s/track.ogg", chartLocation);
            chartAudio = MIX_LoadAudio(NULL, fileLocation, false); //what
        }

        if(songs[currentSongIndex].hasMP3 && chartAudio == NULL){
            SDL_snprintf(fileLocation, locationMaxSize, "%s/track.mp3", chartLocation);
            chartAudio = MIX_LoadAudio(NULL, fileLocation, false); //what
        }

        if(chartAudio != NULL){
            chartSong = MIX_CreateTrack(mainMixer);
            if_unlikely(chartSong == NULL) {
                setError("MIX_CreateTrack error:\n%s\n", SDL_GetError());
                runLoadFunction(loadInGame, inGameSlot); //hack
                return 0;
            }

            MIX_SetTrackAudio(chartSong, chartAudio);
            MIX_SetTrackGain(chartSong, userMusicVolume);
        }

        #ifdef Circular8_ENABLE_FFmpeg
        if(isVideoEnabled && SDL_round(backgroundBrightness * 255.0) > 0){
            if(songs[currentSongIndex].hasMP4filePV){
                SDL_snprintf(fileLocation, locationMaxSize, "%s/pv.mp4", chartLocation);
                loadVideo(fileLocation);
            } else if(songs[currentSongIndex].hasMP4fileMV){
                SDL_snprintf(fileLocation, locationMaxSize, "%s/mv.mp4", chartLocation);
                loadVideo(fileLocation);
            } else if(songs[currentSongIndex].hasMP4fileBG){
                SDL_snprintf(fileLocation, locationMaxSize, "%s/bg.mp4", chartLocation);
                loadVideo(fileLocation);
            } else if(songs[currentSongIndex].hasWebMfilePV){
                SDL_snprintf(fileLocation, locationMaxSize, "%s/pv.webm", chartLocation);
                loadVideo(fileLocation);
            } else if(songs[currentSongIndex].hasWebMfileMV){
                SDL_snprintf(fileLocation, locationMaxSize, "%s/mv.webm", chartLocation);
                loadVideo(fileLocation);
            } else if(songs[currentSongIndex].hasWebMfileBG){
                SDL_snprintf(fileLocation, locationMaxSize, "%s/bg.webm", chartLocation);
                loadVideo(fileLocation);
            }
        }
        #endif // Circular8_ENABLE_FFmpeg
        SDL_free(fileLocation);

        //debug only
        /*for(int i = 0; i < chartLength; i++){
            if(chartNotes[i].noteType != noteTypeSlide){
                continue;
            }

            for(int n = 0; n < chartNotes[i].info.slide.lenOfStartIndex; n += 2){
                if(n > 0){
                    printf(",");
                }

                int slideButtons[2] = {chartNotes[i].info.slide.startIndexes[n].touchLocation1,
                                                        chartNotes[i].info.slide.startIndexes[n].touchLocation2};

                if(slideButtons[0] / 8 == 2){
                    //swap to ensure b button is before c button for display
                    const int temp1 = slideButtons[0];
                    slideButtons[0] = slideButtons[1];
                    slideButtons[1] = temp1;
                }

                for(int j = 0; j < 2 && slideButtons[j] >= 0 && (j == 0 || (slideButtons[0] != slideButtons[j])); j++){
                    if(j == 1){
                        printf("/");
                    }

                    if(slideButtons[j] / 8 == 0){
                        printf("A%d", (slideButtons[j] % 8) + 1);
                    } else if(slideButtons[j] / 8 == 1){
                        printf("B%d", (slideButtons[j] % 8) + 1);
                    } else {
                        printf("C1/C2");
                        break;
                    }
                }
            }
            printf("\n");
        }*/

        firstOffset = songs[currentSongIndex].individualOffset[selectedDifficulty];

        #ifdef Circular8_ENABLE_CONTROLLER
        //ensure the controller is enabled
        if(isUsingSerialController){
            makeTouchscreenControllerSendData();
        }
        #endif // Circular8_ENABLE_CONTROLLER

        runLoadFunction(loadInGame, inGameSlot);
        return 0;
    }

    if(isButtonOrTouchPressed(backButtonBoxI) && timeInDifficultySelect >= (Uint64)gameTicksPerSecond / 4){
        highlightRotationTime = 0;
        timeInDifficultySelect = 0;
        runLoadFunction(loadSongSelect, songSelectSlot);
        return 0;
    }


    if(isButtonOrTouchPressed(decreaseDiffButton)){
        diffChange--;
    }

    if(isButtonOrTouchPressed(increaseDiffButton)){
        diffChange++;
    }

    for(previousDifficulty = selectedDifficulty - 1; previousDifficulty >= 0 && songs[currentSongIndex].difficultyStartIndexes[previousDifficulty] < 2; previousDifficulty--);
    for(nextDifficulty = selectedDifficulty + 1; nextDifficulty <= 6 && songs[currentSongIndex].difficultyStartIndexes[nextDifficulty] < 2; nextDifficulty++);

    if(diffChange < 0 && previousDifficulty >= 0){
        selectedDifficulty = previousDifficulty;
    } else if(diffChange > 0 && nextDifficulty <= 6){
        selectedDifficulty = nextDifficulty;
    }

    return 0;
}
