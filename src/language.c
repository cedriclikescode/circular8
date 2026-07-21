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
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include "thirdparty/inih/ini.h"

#include "main.h"
#include "miscutils.h"
#include "language.h"

static languageKeyValue langTexts[128] = {0};
static int lenOfLangTexts = 0;
char* currentLocaleName = NULL;
char* currentJudgeLocaleName = NULL;

char* langGetStr(const char* key){
    for(int i = 0; i < lenOfLangTexts; i++){
        if(SDL_strcmp(key, langTexts[i].key) == 0){
            return langTexts[i].value;
        }
    }
    return "BAD KEY USED";
}

static int langHandler(void* user, const char* section, const char* name, const char* value){
    if(SDL_strcmp(section, "lang") != 0 || lenOfLangTexts >= stackArraySizeInt(langTexts)){
        return 1;
    }

    languageKeyValue* pLangTexts = (languageKeyValue*)user;
    if(lenOfLangTexts > 0 && SDL_strcmp(name, pLangTexts[lenOfLangTexts - 1].key) == 0){
        //concat time
        char* newValue = joinPrefixMiddleEndCharArrays(pLangTexts[lenOfLangTexts - 1].value, "\n", value);
        SDL_free(pLangTexts[lenOfLangTexts - 1].value);
        pLangTexts[lenOfLangTexts - 1].value = newValue;
    } else {
        pLangTexts[lenOfLangTexts].key = SDL_strdup(name);
        pLangTexts[lenOfLangTexts].value = SDL_strdup(SDL_strcmp(name, "thousandsseparator") == 0 && SDL_strcmp(value, "space") == 0 ? " " : //hack for thousands separator being a space in French
                                                      value);
        lenOfLangTexts++;
    }
    return 1;
}

void loadLangTexts(const char* localeName){
    char* realFileName = joinTwoCharArraysToOne(localeName, ".ini");
    char* pathToLocaleFile = joinPrefixMiddleEndCharArrays(basePathLocation, "lang/", realFileName);
    SDL_free(realFileName);

    if(ini_parse(pathToLocaleFile, langHandler, &langTexts) < 0){
        printf("Bad locale ini\n");
    }
    printf("length of langTexts: %d\n", lenOfLangTexts);
}

void separateNumberByThousands(char* str, size_t maxlen, int number){
    if(number >= 1000000){
        SDL_snprintf(str, maxlen, "%d%s%03d%s%03d", number / 1000000, langGetStr("thousandsseparator"), number % 1000, langGetStr("thousandsseparator"), number % 1000);
    } else if(number >= 1000){
        SDL_snprintf(str, maxlen, "%d%s%03d", number / 1000, langGetStr("thousandsseparator"), number % 1000);
    } else {
        SDL_snprintf(str, maxlen, "%d", number);
    }
}


void separateNumberWithCorrectDecimal(char* str, size_t maxlen, double number, int precision){
    char formatStr[16] = {0};
    simple_snprintf(formatStr, "%%.%d%c", precision, 'f');

    SDL_snprintf(str, maxlen, formatStr, number); //magic
    const int len = SDL_strnlen(str, maxlen);
    for(int i = 0; i < len; i++){
        if(str[i] == '.'){
            const char* decimal = langGetStr("decimalseparator");
            str[i] = decimal[0];
            break;
        }
    }
}

void separatePercentageWithCorrectDecimal(char* str, size_t maxlen, double number, int precision){
    separateNumberWithCorrectDecimal(str, maxlen, number, precision);
    SDL_strlcat(str, "%", maxlen);
}
