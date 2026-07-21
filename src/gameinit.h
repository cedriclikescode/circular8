#pragma once
#ifndef Circular8_GAMEINIT_H
#define Circular8_GAMEINIT_H

enum {
    mainMenuSlot = 0,
    profileSelectSlot = 1,
    songSelectSlot = 2,
    difficultySelectSlot = 3,
    inGameSlot = 4,
    scoreScreenSlot = 5,
    gameOptionsSlot = 6,
    loadingScreenSlot = 7
};

enum {
    bigPtSize = 55,
    mediumPtSize = 30,
    smallPtSize = 22
};

void loadMainMenuGui(int slot);

void loadInGame(int slot);
void loadSongSelect(int slot);
void loadDifficultySelect(int slot);
void loadGameOptions(int slot);

void loadInGameBootstrap(int slot);
void loadScoreScreen(int slot);

void reloadJacketsAndTitlesForAllSongs(void);
void loadChartGraphics(void);
void loadProfileSelect(int slot);

int SDLCALL profileSortingCompare(const void *a, const void *b);

void updateRatingTexture(int i);
#endif // Circular8_GAMEINIT_H
