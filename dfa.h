#ifndef __DFA_H
#define __DFA_H

#define MAX_CHARS	128
#define MAX_DFAS	128
#define MAX_TABLESIZE	(MAX_DFAS * MAX_CHARS)

#define F	-1	/* fail transition on char */

extern struct set *epsilon_closure(struct set *, int *, int);
extern struct set *move(struct set *, int state);

typedef int ROW[MAX_CHARS];

struct dfa {
	int group;
	struct set *states;	/* nfa states */
	int accept;
};

#endif	/* dfa.h */
