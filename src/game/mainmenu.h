#pragma once
#ifndef Circular8_MAINMENU_H
#define Circular8_MAINMENU_H

#include <SDL3/SDL.h>
Uint64 mainMenu(Uint64 delta);

extern SDL_Texture* mainMenuTitle;
extern SDL_Texture* mainMenuStartButton;
extern SDL_Texture* SDLVersions[4];
extern SDL_Texture* buildDate;
void runProfileLoadingHelper(void);
#endif // Circular8_MAINMENU_H
