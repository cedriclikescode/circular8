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

#include "game/songselect.h"
#include "game/chart.h"
#include "game/ingame.h"
#include "game/profileselect.h"
#include "game/options.h"

#include "thirdparty/inih/ini.h"


int numberOfProfiles = 0;
profileInfo* profiles = NULL;
SDL_Texture* userCard = NULL;
static bool searchedProfileHasPicture = false;
static bool searchedProfileHasUserINI = false;

static void handleProfileSectionUser(const char* name, const char* value){
    if(SDL_strcmp(name, "name") == 0){
        profiles[numberOfProfiles].name = SDL_strdup(value);
    } else if(SDL_strcmp(name, "title") == 0){
        profiles[numberOfProfiles].title = SDL_strdup(value);
    } else if(SDL_strcmp(name, "rating") == 0){
        profiles[numberOfProfiles].rating = SDL_max(SDL_atof(value), 0.0);
    } else if(SDL_strcmp(name, "plays") == 0){
        profiles[numberOfProfiles].playCount = SDL_max(SDL_atoi(value), 0);
    }
}

static void handleProfileSectionGameplayOptions(const char* name, const char* value){
    if(SDL_strcmp(name, "notespeed") == 0){
        tapSpeed = clampDouble(SDL_atof(value), 1.0, 10.0);
    } else if(SDL_strcmp(name, "touchspeed") == 0){
        touchSpeed = clampDouble(SDL_atof(value), 1.0, 10.0);
    } else if(SDL_strcmp(name, "sliderotation") == 0){
        rotateStarNote = SDL_strcmp(value, "1") == 0;
    } else if(SDL_strcmp(name, "mirror") == 0){
        mirrorType = clampInt(SDL_atoi(value), 0, 3);
    } else if(SDL_strcmp(name, "trackskip") == 0){
        trackSkip = clampInt(SDL_atoi(value), 0, stackArraySizeInt(trackSkipTexts));
    }
}

static void handleProfileSectionDisplayOptions(const char* name, const char* value){
    if(SDL_strcmp(name, "centerdisplay") == 0){
        displayType = clampInt(SDL_atoi(value), 0, stackArraySizeInt(achievementTexts));
    } else if(SDL_strcmp(name, "backgroundbrightness") == 0){
        backgroundBrightness = clampDouble(SDL_atof(value), 0.0, 1.0);
    } else if(SDL_strcmp(name, "showsensors") == 0){
        showSensors = clampInt(SDL_atoi(value), 0, 3);
    } else if(SDL_strcmp(name, "usealtstarnote") == 0){
        useAlternateStarNote = SDL_strcmp(value, "1") == 0;
    }
}

static void handleProfileSectionJudgeOptions(const char* name, const char* value){
    if(SDL_strcmp(name, "showfastlate") == 0){
        showFastLate = clampInt(SDL_atoi(value), 0, 3);
    } else if(SDL_strcmp(name, "showcritperfect") == 0){
        showCriticalPerfect = clampInt(SDL_atoi(value), 0, 2);
    } else if(SDL_strcmp(name, "chartoffset") == 0){
        chartOffset60fps = clampDouble(SDL_atof(value), -10.0, 10.0);
        realChartOffset = SDL_round(chartOffset60fps / 60.0 * gameTicksPerSecond);
    } else if(SDL_strcmp(name, "inputoffset") == 0){
        inputOffset60fps = clampDouble(SDL_atof(value), -10.0, 10.0);
        realInputOffset = SDL_round(inputOffset60fps / 60.0 * gameTicksPerSecond);
    }
}

static void handleProfileSectionAudioOptions(const char* name, const char* value){
    if(SDL_strcmp(name, "sfxvolume") == 0){
        gameplaySfxVolume = clampDouble(SDL_atof(value), 0.0, 1.0);
    } else if(SDL_strcmp(name, "bgmvolume") == 0){
        userMusicVolume = clampDouble(SDL_atof(value), 0.0, 1.0);
    } else if(SDL_strcmp(name, "enablefireworkssfx") == 0){
        enableFireworksSFX = SDL_strcmp(value, "1") == 0;
    } else if(SDL_strcmp(name, "enableslidesfx") == 0){
        enableSlideSFX = SDL_strcmp(value, "1") == 0;
    } else if(SDL_strcmp(name, "guideoffset") == 0){
        guideOffset60fps = clampDouble(SDL_atof(value), -10.0, 10.0);
        realGuideOffset = SDL_round(guideOffset60fps / 60.0 * gameTicksPerSecond);
    }
}

int profileHandler(void* user, const char* section, const char* name, const char* value){
    UNUSED(user);
    if(SDL_strcmp(section, "user") == 0){
        handleProfileSectionUser(name, value);
        return 0;
    } else if(SDL_strcmp(section, "gameplayoptions") == 0){
        handleProfileSectionGameplayOptions(name, value);
        return 0;
    } else if(SDL_strcmp(section, "displayoptions") == 0){
        handleProfileSectionDisplayOptions(name, value);
        return 0;
    } else if(SDL_strcmp(section, "judgeoptions") == 0){
        handleProfileSectionJudgeOptions(name, value);
        return 0;
    } else if(SDL_strcmp(section, "audiooptions") == 0){
        handleProfileSectionAudioOptions(name, value);
        return 0;
    }
    return 1;
}

void updateProfile(bool updatePlayCount){
    char info[1024] = {0};
    if(updatePlayCount){
        profiles[currentProfileIndex].playCount++;
    }
    simple_snprintf(info, "[user]\nname = %s\ntitle = %s\nrating = %.2f\nplays = %d\n\n[gameplayoptions]\nnotespeed = %.2f\ntouchspeed = %.2f\nsliderotation = %d\nmirror = %d\ntrackskip = %d\n\n[displayoptions]\ncenterdisplay = %d\nbackgroundbrightness = %.2f\nshowsensors = %d\nusealtstarnote = %d\n\n[judgeoptions]\nshowfastlate = %d\nshowcritperfect = %d\nchartoffset = %.2f\ninputoffset = %.2f\n\n[audiooptions]\nsfxvolume = %.2f\nbgmvolume = %.2f\nenablefireworkssfx = %d\nenableslidesfx = %d\nguideoffset = %.2f\n",
                    profiles[currentProfileIndex].name, profiles[currentProfileIndex].title,
                    profiles[currentProfileIndex].rating,
                    profiles[currentProfileIndex].playCount,
                    tapSpeed,
                    touchSpeed,
                    rotateStarNote,
                    mirrorType,
                    trackSkip,
                    displayType,
                    backgroundBrightness,
                    showSensors,
                    useAlternateStarNote,
                    showFastLate,
                    showCriticalPerfect,
                    chartOffset60fps,
                    inputOffset60fps,
                    gameplaySfxVolume,
                    userMusicVolume,
                    enableFireworksSFX,
                    enableSlideSFX,
                    guideOffset60fps);

    char* path = joinPrefixMiddleEndCharArrays("profiles/", profiles[currentProfileIndex].profileLocation, "/user.ini");
    //simple_snprintf(path, "profiles/%s/user.ini", profiles[currentProfileIndex].profileLocation);

    SDL_SaveFile(path, info, SDL_strnlen(info, stackArraySize(info)));
    SDL_free(path);
    updateRatingTexture(currentProfileIndex);
}

static SDL_EnumerationResult loadOneProfileCallback(void* userdata, const char* directoryName, const char* fileName){
    UNUSED(userdata);
    UNUSED(directoryName);
    if(SDL_strcmp(fileName, "profile.png") == 0){
        searchedProfileHasPicture = true;
    } else if(SDL_strcmp(fileName, "user.ini") == 0){
        searchedProfileHasUserINI = true;
    }
    return SDL_ENUM_CONTINUE;
}

SDL_EnumerationResult loadProfilesCallback(void* userdata, const char* directoryName, const char* fileName){
    UNUSED(userdata);
    SDL_PathInfo fileInfo;
    char* fullPath = joinPrefixMiddleEndCharArrays(directoryName, "/", fileName);
    SDL_GetPathInfo(fullPath, &fileInfo);
    if(fileInfo.type == SDL_PATHTYPE_DIRECTORY){
        searchedProfileHasPicture = false;
        searchedProfileHasUserINI = false;
        SDL_EnumerateDirectory(fullPath, loadOneProfileCallback, NULL);

        if(!searchedProfileHasUserINI){
            SDL_free(fullPath);
            return SDL_ENUM_CONTINUE;
        }

        if(numberOfProfiles % 8 == 0){
            profileInfo* newPointer = (profileInfo*)reallocfTypeSDL((profiles), sizeof(profileInfo) * (numberOfProfiles + 8));
            profiles = newPointer;
        }
        profiles[numberOfProfiles].profileLocation = SDL_strdup(fileName);
        profiles[numberOfProfiles].hasProfilePicture = searchedProfileHasPicture;
        profiles[numberOfProfiles].profileIcon = NULL; //valgrind moment
        profiles[numberOfProfiles].nameTexture = NULL;
        profiles[numberOfProfiles].ratingTexture = NULL;
        profiles[numberOfProfiles].rating = -1;
        profiles[numberOfProfiles].playCount = 0;

        profiles[numberOfProfiles].name = NULL;//"(no name)";
        profiles[numberOfProfiles].fullPath = fullPath;

        //ini moment (garbage)
        char* realFileName = joinTwoCharArraysToOne(fullPath, "/user.ini");
        if(ini_parse(realFileName, profileHandler, NULL) < 0){
            printf("BAD INI\n");
        }
        SDL_free(realFileName);
        numberOfProfiles++;
    }
    return SDL_ENUM_CONTINUE;
}


int currentProfileIndex = 0;
static int profileChangeDirection = 0;
static float profileDistance = 0.f;
static Uint64 profileChangeTimer = 0;

static void renderUserCard(float xOffset, float yPos, int i, bool isHighLighted, bool useCenterY){
    if(isHighLighted){
        setTextureColor(userCard, 255, 255, 255);
    } else {
        setTextureColor(userCard, 64, 64, 64);
    }

    const SDL_FRect cardFRect = FRectWithTexSimple(userCard, getCenterX() + xOffset * guiScale, (useCenterY * getCenterY()) + (150.f * yPos + profileDistance) * guiScale, 1.f, true);
    renderTexture(userCard, NULL, &cardFRect);
    const float BGSizeNormalized = profiles[i].profileIcon != NULL;
    const float cardBGSize = 110.f * guiScale * BGSizeNormalized;
    const SDL_FRect cardBGFRect = {cardFRect.x + 6.f * guiScale, cardFRect.y + 15.f * guiScale, cardBGSize, cardBGSize};
    renderTexture(profiles[i].profileIcon, NULL, &cardBGFRect);

    const SDL_FRect nameFRect = FRectWithTexSimple(profiles[i].nameTexture, cardBGFRect.x + (cardFRect.w + cardBGSize - (6.f * guiScale * BGSizeNormalized)) / 2.f, cardBGFRect.y + 5.f * guiScale, 1.f, true);
    renderTexture(profiles[i].nameTexture, NULL, &nameFRect);

    const SDL_FRect ratingFRect = FRectWithTexSimple(profiles[i].ratingTexture, cardBGFRect.x + (cardFRect.w + cardBGSize - (6.f * guiScale * BGSizeNormalized)) / 2.f, nameFRect.y + 85.f * guiScale, 1.f, true);
    renderTexture(profiles[i].ratingTexture, NULL, &ratingFRect);
}

int renderUserInfoOnSubscreen(void){
    if(numberOfProfiles > 0){
        profileDistance = 0.f;
        renderUserCard(-165, 0.6f, currentProfileIndex, true, false);
    }
    return 0;
}

static int renderProfileSelect(void) {
    for(int i = -3; i <= 3; i++){
        const int n = i + currentProfileIndex;
        bool shouldProfileBeShown = n >= 0 && n < numberOfProfiles;
        //songBorders[i + 3]->shown = shouldSongBeShown;


        if(shouldProfileBeShown){
            const bool isHighLighted = profileChangeDirection == 0 && i == 0;
            renderUserCard(0.f, i, n, isHighLighted, true);
        }
    }
    renderNextButtonBox();
    renderScrollButtonBoxes();
    return 0;
}

Uint64 profileSelect(Uint64 delta){
    static int upDownDir = 0;
    renderFunction = renderProfileSelect;
    if(isButtonOrTouchPressed(scrollDownButtonBoxI)){
        upDownDir -= 1;
    }

    if(isButtonOrTouchPressed(scrollUpButtonBoxI)){
        upDownDir += 1;
    }

    static Uint64 timeInProfileSelect = 0;

    timeInProfileSelect += delta;


    if(profileChangeDirection == 0){
        profileChangeDirection -= upDownDir;
        upDownDir = 0;
        if(isButtonOrTouchPressed(nextButtonBoxI) && timeInProfileSelect > gameTicksPerSecond / 2){
            profileChangeDirection = 0;
            resetButtons();
            timeInProfileSelect = 0;
            runLoadFunction(loadSongSelect, songSelectSlot);
            return 0;
        }
    }

    profileDistance = progressivelyMoveBy(delta, &profileChangeTimer, gameTicksPerSecond / 10, &currentProfileIndex, &profileChangeDirection, numberOfProfiles, 150.f);
    return 0;
}
