#pragma once
#ifndef Circular8_MAIN_H
#define Circular8_MAIN_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

extern SDL_Window *window;
extern MIX_Mixer* mainMixer;

extern TTF_Font *bigFont, *mediumFont, *smallFont;
extern int mouseX, mouseY;
extern Uint8 globalBrightness;
extern int rendererVSync;

extern bool didCursorSet, didMouseMove;
extern int (*renderFunction)(void);
extern int (*subscreenRenderFunction)(void);

extern Uint64 countPerSecond;

extern SDL_Texture* numbersText;
extern SDL_FRect numbersTextFRects[15];
extern char numberChars[16];
void loadAllText(void);
extern int currentSlot;
extern Uint64 (*functionPointers[16])(Uint64);
extern void (*reloadFunction)(int);
void addGameFunctionToCurrentSlot(Uint64 (*gameFunction)(Uint64 delta));

extern char* basePathLocation;

extern SDL_Texture* gameVersion;
extern bool reRunGameFunction;
#endif // Circular8_MAIN_H
