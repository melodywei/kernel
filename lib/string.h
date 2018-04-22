#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_

#include "stdint.h"


void memset(void *dest, uint8_t value, uint32_t size);

void memcpy(void *dest, const void *src, uint32_t size);

int memcmp(const void *a, const void *b, uint32_t size);

char *strcpy(char *dest, const char *src);

uint32_t strlen(const char *str);

int strcmp(const char *a, const char *b);

char *strchr(const char *str, const char ch);

char *strrchr(const char *str, const char ch);

char *strcat(char *dest, const char *src);

uint32_t strchrs(const char *str, char ch);

#endif //!_LIB_STRING_H_