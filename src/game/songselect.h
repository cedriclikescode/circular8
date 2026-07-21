#pragma once
#ifndef Circular8_SONGSELECT_H
#define Circular8_SONGSELECT_H

#include <stdbool.h>
#include <SDL3/SDL.h>

Uint64 songSelect(Uint64 delta);

typedef struct songFile { //todo: optimize it
    Sint64 individualOffset[7]; //&first_#
    char* difficultyLevels[7];
    char* difficultyNames[7];
    char* checksumOfFile; //todo: hash only the chart
    char* songTitle;
    char* chartLocation;
    SDL_Texture* background;
    SDL_Texture* titleText;
    int difficultyStartIndexes[7];
    SDL_Color difficultyColors[7]; //&color_#
    bool hasPNG;
    bool hasJPG;
    bool hasMP3;
    bool hasOGG;
    bool hasMP4filePV;
    bool hasMP4fileMV;
    bool hasMP4fileBG;
    bool hasWebMfilePV;
    bool hasWebMfileMV;
    bool hasWebMfileBG;
} songFile;

extern songFile* songs;
extern int lengthOfSongs;
extern int currentSongIndex;
extern bool enableSongSelectBGs;
extern SDL_Texture* buttonBox;
extern SDL_Texture* buttonBoxTexts[8];
extern const SDL_FPoint buttonBoxWH;

void renderButtonTextBox(int i, SDL_Texture* text, Uint8 r, Uint8 g, Uint8 b);
void renderButtonTextBox2(int i, SDL_Texture* text, SDL_Color color);
extern const int backButtonBoxI;
extern const int nextButtonBoxI;
extern const int scrollDownButtonBoxI;
extern const int scrollUpButtonBoxI;
extern const SDL_Color buttonBoxColors[8];
extern SDL_Texture* menuBackground;
extern SDL_Texture* songBorder;

void renderSongBorder(float yPos, int song, bool isHighLighted);
void renderBackButtonBox(void);
void renderNextButtonBox(void);
void renderScrollButtonBoxes(void);
#endif // Circular8_SONGSELECT_H
