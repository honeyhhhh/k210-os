#ifndef __ASSERT_H__
#define __ASSERT_H__


#include "stddef.h"


void __warn(const char *file, int line, const char *fmt, ...);
void __panic(const char *file, int line, const char *fmt, ...);


#define warn(...)                                       \
    __warn(__FILE__, __LINE__, __VA_ARGS__)


#define panic(...)                                      \
    __panic(__FILE__, __LINE__, __VA_ARGS__)




#ifndef assert
#define assert(f) \
    if (!(f))     \
	    panic("\n --- Assert Fatal ! ---    : %s\n", #f);
#endif

#endif 