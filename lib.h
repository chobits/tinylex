#ifndef __LIB_H
#define __LIB_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void errexit(char *str);
void *xmalloc(size_t size);
#define dbg(fmt, arg...) fprintf(stderr, "%s "fmt"\n", __FUNCTION__, ##arg)
#endif
