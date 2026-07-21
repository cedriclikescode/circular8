#pragma once
#ifndef Circular8_LANGUAGE_H
#define Circular8_LANGUAGE_H
typedef struct languageKeyValue {
    char* key;
    char* value;
} languageKeyValue;

void loadLangTexts(const char* localeName);

char* langGetStr(const char* key);

extern char* currentLocaleName;
extern char* currentJudgeLocaleName;
void separateNumberByThousands(char* str, size_t maxlen, int number);
void separateNumberWithCorrectDecimal(char* str, size_t maxlen, double number, int precision);
void separatePercentageWithCorrectDecimal(char* str, size_t maxlen, double number, int precision);
#endif // Circular8_LANGUAGE_H
