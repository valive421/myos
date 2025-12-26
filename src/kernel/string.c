#include "string.h"

int strlen(const char* str)
{
    int len = 0;
    while(str[len])
        len++;
    return len;
}

int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n)
{
    while(n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if(n == 0)
        return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strcpy(char* dest, const char* src)
{
    char* d = dest;
    while(*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

char* strcat(char* dest, const char* src)
{
    char* d = dest;
    while(*d)
        d++;
    while(*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}
