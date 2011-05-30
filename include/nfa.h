#ifndef __NFA_H
#define __NFA_H

#include "set.h"
#define dbg(fmt, arg...) fprintf(stderr, "%s "fmt"\n", __FUNCTION__, ##arg)

#define MAXNFAS 512

/* edge special state */
#define EG_EPSILON	(-1)
#define EG_CCL		(-2)
#define EG_EMPTY	(-3)
#define EG_DEL		(-4)	/* lazy deleted, in nfa stack */

/* anchor flag */
#define AC_NONE		(0)	/* ..... */
#define AC_START	(1)	/* ^.... */
#define AC_END		(2)	/* ....$ */
#define AC_BOTH		(3)	/* ^...$ */

struct nfa {
	int edge;
	int anchor;
	struct set *set;	/* concatenation chars */
	struct nfa *next[2];
	char *accept;
};

/* extern global parameter */
extern int nfapos;
extern struct nfa *nfabuf;

/* extern function */
extern void __traverse_nfa(struct nfa *, int, struct nfa *);
extern void traverse_nfa(struct nfa *);
extern struct nfa *machine(void);


/* auxilary method */
static inline int nfastate(struct nfa *nfa)
{
	return (nfabuf && nfa) ? (nfa - nfabuf) : -1;
}

static inline struct nfa *statenfa(int state)
{
	return (state >= 0 && state < MAXNFAS) ?
				&nfabuf[state] :
				NULL;
}

#endif	/* nfa.h */
