#include "string.h"
#include "../kernel/debug.h"
#include "../kernel/global.h"

void memset(void *dest, uint8_t value, uint32_t size)
{
    ASSERT(dest != NULL);
    uint8_t *dest_tmp = (uint8_t *)dest;

    while(size-- > 0)
    {
        *dest_tmp++ = value;
    }
}

void memcpy(void *dest, const void *src, uint32_t size)
{
    ASSERT(dest != NULL && src != NULL);

    uint8_t *dest_tmp = (uint8_t*)dest;
    const uint8_t *src_tmp = (const uint8_t *)src;

    while(size-- > 0)
    {
        *dest_tmp++ = *src_tmp++;
    }
}

int memcmp(const void *a, const void *b, uint32_t size)
{
    const char * a_tmp = (const char *)a;
    const char *b_tmp = (const char *)b;

    ASSERT(a != NULL || b != NULL);

    while (size-- > 0)
    {
        if(*a_tmp != *b_tmp)
        {
            return *a_tmp > *b_tmp ? 1 : -1;
        }
        ++a_tmp;
        ++b_tmp;
    }

    return 0;
}

char *strcpy(char *dest, const char *src)
{
    ASSERT(dest != NULL && src != NULL);
    char *ret = dest;

    while((*dest++ = *src++));
    return ret;
}

uint32_t strlen(const char *str)
{
    ASSERT(str != NULL);
    uint32_t len = 0;

    while(*str++)
    {
        ++len;
    }
    return len;
}

int strcmp(const char *a, const char *b)
{
    ASSERT(a != NULL && b != NULL);
    while(*a && *a == *b)
    {
        ++a;
        ++b;
    }

    return *a < *b ? -1 : *a > *b;
}

char *strchr(const char *str, const char ch)
{
    ASSERT(str != NULL);
    while(*str)
    {
        if(*str == ch)
            return (char *)str;
        
        ++str;
    }
    return NULL;
}

char *strrchr(const char *str, const char ch)
{
    ASSERT(str != NULL);

    char *last_char = NULL;

    while(*str)
    {
        if(*str == ch)
            last_char = str;
        
        ++str;
    }

    return last_char;
}

char *strcat(char *dest, const char *src)
{
    ASSERT(dest != NULL && src != NULL);

    char *str = dest;
    while (*++str);

    --str;

    while((*str++ = *src++));

    return dest;
}

uint32_t strchrs(const char *str, char ch)
{
    ASSERT(str != NULL);

    uint32_t ch_cnt = 0;
    
    while (*str)
    {
        if(*str == ch)
            ++ch_cnt;
        
        ++str;
    }
    return ch_cnt;
}