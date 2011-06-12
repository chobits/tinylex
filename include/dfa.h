#ifndef __DFA_H
#define __DFA_H

#include "nfa.h"

#define MAX_CHARS	128
#define MAX_DFAS	128
#define MAX_GROUPS	128
#define MAX_TABLESIZE	(MAX_DFAS * MAX_CHARS)

#define F	-1	/* fail transition on char */

extern struct set *epsilon_closure(struct set *, struct accept **, int);
extern struct set *move(struct set *, int state);

typedef int ROW[MAX_CHARS];

struct dfa {
	struct set *states;	/* nfa states */
	struct accept *accept;	/* accept structure */
	int group;		/* used for dfa minization */
};

extern int ndfas;
extern struct dfa *dfastates;

extern int sgroup;
extern int ngroups;

extern void traverse_dfatable(int (*)[128], int, struct set *);
extern int construct_dfa(struct nfa *, int (**)[], struct set **);

#endif	/* dfa.h */
