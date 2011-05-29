#ifndef __LIB_H
#define __LIB_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

extern void errexit(char *str);
extern void *xmalloc(size_t size);
extern int isspaceline(char *line);

#define dbg(fmt, arg...) fprintf(stderr, "%s "fmt"\n", __FUNCTION__, ##arg)
#endif	/* lib.h */
