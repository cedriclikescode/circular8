#pragma once
#ifndef Circular8_PROFILESELECT_H
#define Circular8_PROFILESELECT_H
#include <stdbool.h>
#include <SDL3/SDL.h>

Uint64 profileSelect(Uint64 delta);
extern int numberOfProfiles;
extern int currentProfileIndex;
extern SDL_Texture* userCard;

typedef struct profileInfo {
    double rating;
    SDL_Texture* profileIcon;
    SDL_Texture* nameTexture;
    SDL_Texture* ratingTexture;
    char* profileLocation;
    char* title;
    char* name;
    char* fullPath;
    int playCount;
    bool hasProfilePicture;
} profileInfo;

extern profileInfo* profiles;
void updateProfile(bool updatePlayCount);
SDL_EnumerationResult loadProfilesCallback(void* userdata, const char* directoryName, const char* fileName);
int renderUserInfoOnSubscreen(void);
int profileHandler(void* user, const char* section, const char* name, const char* value);
#endif // Circular8_PROFILESELECT_H
