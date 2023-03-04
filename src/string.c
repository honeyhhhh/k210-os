#include "include/stddef.h"
#include "include/string.h"

#define SS (sizeof(size_t))
#define ALIGN (sizeof(size_t) - 1)
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)



int isspace(int c)
{
    return c == ' ' || (unsigned)c - '\t' < 5;
}

int isdigit(int c)
{
    return (unsigned)c - '0' < 10;
}

int atoi(const char *s)
{
    int n = 0, neg = 0;
    while (isspace(*s))
        s++;
    switch (*s)
    {
        case '-':
            neg = 1;
        case '+':
            s++;
    }
    /* Compute n as a negative number to avoid overflow on INT_MIN */
    while (isdigit(*s))
        n = 10 * n - (*s++ - '0');
    return neg ? n : -n;
}

void *memset(void *dest, uint8_t c, size_t n)
{
    uint8_t *p = (uint8_t *)dest;
    for (int i = 0; i < n; ++i, *(p++) = c)
        ;
    return dest;
}


int memcmp(const void *v1, const void *v2, uint n)
{
  const uint8_t *s1, *s2;

  s1 = v1;
  s2 = v2;
  while(n-- > 0){
    if(*s1 != *s2)
      return *s1 - *s2;
    s1++, s2++;
  }

  return 0;
}

void *memmove(void *dst, const void *src, uint n)
{
  const char *s;
  char *d;

  s = src;
  d = dst;
  if(s < d && s + n > d){
    s += n;
    d += n;
    while(n-- > 0)
      *--d = *--s;
  } else
    while(n-- > 0)
      *d++ = *s++;

  return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void *memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}





int strcmp(const char *l, const char *r)
{
    for (; *l == *r && *l; l++, r++)
        ;
    return *(unsigned char *)l - *(unsigned char *)r;
}

// int strncmp(const char *_l, const char *_r, size_t n)
// {
//     const unsigned char *l = (void *)_l, *r = (void *)_r;
//     if (!n--)
//         return 0;
//     for (; *l && *r && n && *l == *r; l++, r++, n--)
//         ;
//     return *l - *r;
// }
int
strncmp(const char *p, const char *q, uint32_t n)
{
    while(n > 0 && *p && *p == *q)
    {
        n--;
        p++;
        q++;
    }
    if(n == 0)
        return 0;
        
    return (uint8_t)*p - (uint8_t)*q;
}

char* strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

size_t strlen(const char *s)
{
    const char *a = s;
    typedef size_t __attribute__((__may_alias__)) word;
    const word *w;
    for (; (uintptr_t)s % SS; s++)
        if (!*s)
            return s - a;
    for (w = (const void *)s; !HASZERO(*w); w++)
        ;
    s = (const void *)w;
    for (; *s; s++)
        ;
    return s - a;
}

void *memchr(const void *src, int c, size_t n)
{
    const unsigned char *s = src;
    c = (unsigned char)c;
    for (; ((uintptr_t)s & ALIGN) && n && *s != c; s++, n--)
        ;
    if (n && *s != c)
    {
        typedef size_t __attribute__((__may_alias__)) word;
        const word *w;
        size_t k = ONES * c;
        for (w = (const void *)s; n >= SS && !HASZERO(*w ^ k); w++, n -= SS)
            ;
        s = (const void *)w;
    }
    for (; n && *s != c; s++, n--)
        ;
    return n ? (void *)s : 0;
}

size_t strnlen(const char *s, size_t n)
{
    const char *p = memchr(s, 0, n);
    return p ? p - s : n;
}

char *strcpy(char *restrict d, const char *s)
{
    typedef size_t __attribute__((__may_alias__)) word;
    word *wd;
    const word *ws;
    if ((uintptr_t)s % SS == (uintptr_t)d % SS)
    {
        for (; (uintptr_t)s % SS; s++, d++)
            if (!(*d = *s))
                return d;
        wd = (void *)d;
        ws = (const void *)s;
        for (; !HASZERO(*ws); *wd++ = *ws++)
            ;
        d = (void *)wd;
        s = (const void *)ws;
    }
    for (; (*d = *s); s++, d++)
        ;
    return d;
}

// char*
// strncpy(char *s, const char *t, int n)
// {
//   char *os;

//   os = s;
//   while(n-- > 0 && (*s++ = *t++) != 0)
//     ;
//   while(n-- > 0)
//     *s++ = 0;
//   return os;
// }
char *strncpy(char *restrict d, const char *s, size_t n)
{
    typedef size_t __attribute__((__may_alias__)) word;
    word *wd;
    const word *ws;
    if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN))
    {
        for (; ((uintptr_t)s & ALIGN) && n && (*d = *s); n--, s++, d++)
            ;
        if (!n || !*s)
            goto tail;
        wd = (void *)d;
        ws = (const void *)s;
        for (; n >= sizeof(size_t) && !HASZERO(*ws); n -= sizeof(size_t), ws++, wd++)
            *wd = *ws;
        d = (void *)wd;
        s = (const void *)ws;
    }
    for (; n && (*d = *s); n--, s++, d++)
        ;
tail:
    memset(d, 0, n);
    return d;
}


// convert uchar string into wide char string 
void wnstr(uint16_t *dst, char const *src, int len) {
  while (len -- && *src) {
    *(uint8_t*)dst = *src++;
    dst ++;
  }

  *dst = 0;
}

// convert wide char string into uchar string 
void snstr(char *dst, uint16_t const *src, int len) {
  while (len -- && *src) {
    *dst++ = (uint8_t)(*src & 0xff);
    src ++;
  }
  while(len-- > 0)
    *dst++ = 0;
}

char *strcat(char *dest, const char *src)
{
    char *tmp = dest;

    while (*dest)
        dest++;
    while ((*dest++ = *src++) != '\0');
    return tmp;
}














