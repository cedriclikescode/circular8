#pragma once
#ifndef Circular8_DIFFICULTYSELECT_H
#define Circular8_DIFFICULTYSELECT_H

#include <SDL3/SDL.h>

Uint64 difficultySelect(Uint64 delta);
extern SDL_Texture* difficultyCircle;
extern SDL_Texture* textLV;
extern SDL_Texture* decreaseDifficultyText;
extern SDL_Texture* increaseDifficultyText;
extern SDL_Texture* difficultyHighlight;
extern SDL_Texture* difficultyTextures[7];
extern int selectedDifficulty;
#endif // Circular8_DIFFICULTYSELECT_H
