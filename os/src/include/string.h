#ifndef __STRING_H__
#define __STRING_H__

#include "stddef.h"

int isspace(int c);
int isdigit(int c);
int atoi(const char *s);
void *memset(void *dest, uint8_t c, size_t n);
void *memmove(void *dst, const void *src, uint n);
void *memcpy(void *dst, const void *src, uint n);
int memcmp(const void *v1, const void *v2, uint n);

int strcmp(const char *l, const char *r);
size_t strlen(const char *);
size_t strnlen(const char *s, size_t n);
char *strncpy(char *restrict d, const char *restrict s, size_t n);
int strncmp(const char *_l, const char *_r, size_t n);

#endif // __STRING_H__