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


#include "main.h"
#include "events.h"
#include "gameinit.h"
#include "load.h"
#include "render.h"
#include "miscutils.h"

#include "game/songselect.h"
#include "game/rendersong.h"
#include "game/ingame.h"
#include "game/profileselect.h"
#include "game/difficultyselect.h"
#include "game/mainmenu.h"

bool enableSongSelectBGs = true;
int currentSongIndex = 0;
int lengthOfSongs = 0;
static int songChangeDirection = 0;
static Uint64 songChangeTimer = 0;
songFile* songs = NULL;
SDL_Texture* buttonBox = NULL;
SDL_Texture* menuBackground = NULL;
SDL_Texture* songBorder = NULL;
SDL_Texture* buttonBoxTexts[8] = {0};
const SDL_FPoint buttonBoxWH = {183.3541 * 1.2, 54.8541 * 1.2};

const SDL_Color buttonBoxColors[8] = {
    {  0, 153, 204, 255}, //Options
    {255, 255, 255, 255}, //unused
    {255, 255, 255, 255}, //unused
    {  0, 204, 153, 255}, //Next
    {204, 102,   0, 255}, //v Scroll v
    {196,   0,  72, 255}, //Back
    {255, 255, 255, 255}, //unused
    {204, 102,   0, 255}, //^ Scroll ^
};

const int backButtonBoxI = 6;
const int nextButtonBoxI = 4;
const int optionsButtonBoxI = 1;
const int scrollDownButtonBoxI = 5;
const int scrollUpButtonBoxI = 8;

const int buttonBoxesToDisplay[5 - 1] = {optionsButtonBoxI, nextButtonBoxI, scrollDownButtonBoxI, /*backButtonBoxI,*/ scrollUpButtonBoxI};

void renderButtonTextBox(int i, SDL_Texture* text, Uint8 r, Uint8 g, Uint8 b){
    SDL_FRect buttonBoxFRect = moveNoteObject(buttonBox, getCenterX(), getCenterY(), 1.f, i, (360.f - buttonBoxWH.y / 2.f) * guiScale);

    setTextureColor(buttonBox, r, g, b);
    renderTextureRotated(buttonBox, NULL, &buttonBoxFRect, -22.5 + 45.0 * i + 180.0, NULL, SDL_FLIP_NONE);

    if(text == NULL){
        return;
    }

    float texW, texH;
    SDL_GetTextureSize(text, &texW, &texH);

    const SDL_FRect buttonBoxTextFRect = moveNoteObject(text, getCenterX(), getCenterY(), 1.f, i, (360.f - (buttonBoxWH.y + texH / (1.5f * guiScale)) / 2.f) * guiScale);
    const double rotateMoreBy = i >= 3 && i <= 6 ? 180.0 : 0.0;
    renderTextureRotated(text, NULL, &buttonBoxTextFRect, -22.5 + 45.0 * i + rotateMoreBy, NULL, SDL_FLIP_NONE);
}

void renderButtonTextBox2(int i, SDL_Texture* text, SDL_Color color){
    renderButtonTextBox(i, text, color.r, color.g, color.b);
}

void renderBackButtonBox(void){
    const SDL_Color backColor = buttonBoxColors[backButtonBoxI - 1];
    renderButtonTextBox2(backButtonBoxI, buttonBoxTexts[backButtonBoxI - 1], backColor);
}

void renderNextButtonBox(void){
    const SDL_Color nextColor = buttonBoxColors[nextButtonBoxI - 1];
    renderButtonTextBox2(nextButtonBoxI, buttonBoxTexts[nextButtonBoxI - 1], nextColor);
}

void renderScrollButtonBoxes(void){
    const SDL_Color scrollDownColor = buttonBoxColors[scrollDownButtonBoxI - 1];
    renderButtonTextBox2(scrollDownButtonBoxI, buttonBoxTexts[scrollDownButtonBoxI - 1], scrollDownColor);
    const SDL_Color scrollUpColor = buttonBoxColors[scrollUpButtonBoxI - 1];
    renderButtonTextBox2(scrollUpButtonBoxI, buttonBoxTexts[scrollUpButtonBoxI - 1], scrollUpColor);
}


float songDistance = 0.f;

void renderSongBorder(float yPos, int song, bool isHighLighted){
    if(isHighLighted){
        setTextureColor(songBorder, 255, 255, 255);
    } else {
        setTextureColor(songBorder, 64, 64, 64);
    }

    const SDL_FRect songBorderFRect = FRectWithTexSimple(songBorder, getCenterX(), getCenterY() + (150.f * yPos + songDistance) * guiScale, 1.f, true);
    renderTexture(songBorder, NULL, &songBorderFRect);

    const bool showSongBG = enableSongSelectBGs && songs[song].background != NULL;

    const SDL_FRect songBGFRect = {songBorderFRect.x + 8.f * guiScale, songBorderFRect.y + 8.f * guiScale, 112.f * guiScale * showSongBG, 112.f * guiScale};
    if(showSongBG){
        renderTexture(songs[song].background, NULL, &songBGFRect);
    }

    float texW = 0;
    float texH = 0;
    SDL_GetTextureSize(songs[song].titleText, &texW, &texH);
    const float multiplier = 80.f * guiScale / texH;

    SDL_FRect songTitleFRect = {songBGFRect.x + songBGFRect.w + 16.f * guiScale, songBGFRect.y + 16.f * guiScale, texW * multiplier, 80.f * guiScale};
    const float maxWidth = 432.f * guiScale - songBGFRect.w;

    if(songTitleFRect.w > maxWidth){
        songTitleFRect.w = maxWidth;
    } else {
        songTitleFRect.x += (maxWidth - songTitleFRect.w) / 2.f;
    }

    if(isHighLighted){
        setRenderDrawColor(0, 0, 0, SDL_ALPHA_OPAQUE);
        changeFRectSizeBy(SDL_ceilf(8.f * guiScale), &songTitleFRect);
        renderFillRectF(&songTitleFRect);
        changeFRectSizeBy(-SDL_ceilf(8.f * guiScale), &songTitleFRect);
    }
    renderTexture(songs[song].titleText, NULL, &songTitleFRect);
}


static int renderSongSelect(void) {
    renderTexture(menuBackground, NULL, NULL);
    for(int i = -3; i <= 3; i++){
        const int n = i + currentSongIndex;
        bool shouldSongBeShown = n >= 0 && n < lengthOfSongs;

        if(shouldSongBeShown){
            const bool isHighLighted = songChangeDirection == 0 && i == 0;
            renderSongBorder(i, n, isHighLighted);
        }
    }

    for(int n = 0; n < stackArraySizeInt(buttonBoxesToDisplay); n++){
        const int i = buttonBoxesToDisplay[n];
        if(i != nextButtonBoxI || lengthOfSongs > 0){
            renderButtonTextBox(i, buttonBoxTexts[i - 1], buttonBoxColors[i - 1].r, buttonBoxColors[i - 1].g, buttonBoxColors[i - 1].b);
        }
    }

    if(numberOfProfiles > 0){
        renderBackButtonBox();
    }
    return 0;
}


Uint64 songSelect(Uint64 delta){
    static int upDownDir = 0;
    static Uint64 timeInSongSelect = 0;
    timeInSongSelect += delta;

    static int oldScrollHeld = 0;
    static int currentScrollHeld = 0;
    static Uint64 scrollHeldTime = 0;
    const Uint64 songSelectMenuDAS = gameTicksPerSecond / 2;
    renderFunction = renderSongSelect;
    subscreenRenderFunction = renderUserInfoOnSubscreen;

    const Uint64 songChangeLength = gameTicksPerSecond / 12; //todo: offer option to change speed

    if(isButtonOrTouchPressed(scrollDownButtonBoxI) && currentScrollHeld >= 0){
        upDownDir -= 1;
        currentScrollHeld--;
    }

    if(isButtonOrTouchPressed(scrollUpButtonBoxI) && currentScrollHeld <= 0){
        upDownDir += 1;
        currentScrollHeld++;
    }

    if(currentScrollHeld == -1 && isButtonAndTouchReleased(scrollDownButtonBoxI)){
        currentScrollHeld = 0;
    } else if(currentScrollHeld == 1 && isButtonAndTouchReleased(scrollUpButtonBoxI)){
        currentScrollHeld = 0;
    }

    //const int newScroll = heldButtons[scrollUpButtonBoxI - 1] - heldButtons[scrollDownButtonBoxI - 1];

    if(oldScrollHeld == currentScrollHeld && currentScrollHeld != 0){
        scrollHeldTime += delta;
        if(scrollHeldTime > songSelectMenuDAS){
            songChangeDirection = -32000 * currentScrollHeld;
        }
    } else {
        oldScrollHeld = currentScrollHeld;
        scrollHeldTime = 0;
        songChangeDirection = clampInt(songChangeDirection, -1, 1);
    }

    if(songChangeDirection == 0){
        if(isButtonOrTouchPressed(nextButtonBoxI) && timeInSongSelect >= gameTicksPerSecond / 2 && lengthOfSongs > 0){
            //select easiest difficulty to make sure no issues occur
            for(selectedDifficulty = 0; selectedDifficulty <= 6 && songs[currentSongIndex].difficultyStartIndexes[selectedDifficulty] < 2; selectedDifficulty++);
            //more hacks in charts with 0 difficulties
            selectedDifficulty = clampInt(selectedDifficulty, 0, 6);
            songDistance = 0.f;
            timeInSongSelect = 0;
            oldScrollHeld = 0;
            currentScrollHeld = 0;
            scrollHeldTime = 0;
            runLoadFunction(loadDifficultySelect, difficultySelectSlot);
            return 0;
        } else if(isButtonOrTouchPressed(optionsButtonBoxI) && timeInSongSelect >= gameTicksPerSecond / 2){
            timeInSongSelect = 0;
            resetButtons();
            oldScrollHeld = 0;
            currentScrollHeld = 0;
            scrollHeldTime = 0;
            runLoadFunction(loadGameOptions, gameOptionsSlot);
            return 0;
        } else if(numberOfProfiles > 0 && isButtonOrTouchPressed(backButtonBoxI) && timeInSongSelect >= gameTicksPerSecond / 2){
            timeInSongSelect = 0;
            oldScrollHeld = 0;
            currentScrollHeld = 0;
            scrollHeldTime = 0;
            runProfileLoadingHelper();
            return 0;
        }

        songChangeDirection -= upDownDir;
        upDownDir = 0;
    }

    songDistance = progressivelyMoveBy(delta, &songChangeTimer, songChangeLength, &currentSongIndex, &songChangeDirection, lengthOfSongs, 150.f);

    return 0;
}
