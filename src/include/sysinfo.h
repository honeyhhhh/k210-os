#ifndef __SYSINFO_H
#define __SYSINFO_H

#include "string.h"

struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char domainname[65];
};




#endif
