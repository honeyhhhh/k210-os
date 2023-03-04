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
// char* strncpy(char *s, const char *t, int n);
int strncmp(const char *p, const char *q, uint32_t n);
char *strcat(char *dest, const char *src);

char* strchr(const char *s, char c);
void  wnstr(uint16_t *dst, char const *src, int len);
void  snstr(char *dst, uint16_t const *src, int len);

// size_t strlen(const char *);
// size_t strnlen(const char *s, size_t n);
char *strncpy(char *restrict d, const char *restrict s, size_t n);


#endif // __STRING_H__