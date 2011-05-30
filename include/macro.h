#ifndef __MACRO_H
#define __MACRO_H

#include "list.h"

struct macro {
	char *name;
	char *text;
	unsigned int hash;
	struct list_head list;
};

#define MACRO_HASH_BITS 8
#define MACRO_HASH_SIZE (1 << MACRO_HASH_BITS)
#define MACRO_HASH_MASK (MACRO_HASH_SIZE - 1)
#define tbl(hash) (&macrotable[hash & MACRO_HASH_MASK])

static inline int ismacrochar(int c)
{
	return (c == '_' || isalnum(c));
}

extern void init_macro(void);
extern char *expand_macro(char *name);
extern void add_macro(char *line, int len);

#endif	/* macro.h */
