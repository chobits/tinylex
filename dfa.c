#include <stdio.h>
#include <stdlib.h>
#include "set.h"
#include "nfa.h"
#include "dfa.h"
#include "lib.h"

/* dfa sets auxiliary function */
static int currentdfa, ndfas;
static struct set **dfastates = NULL;

static void free_dfas(void)
{
	int i;
	for (i = 0; i < ndfas; i++)
		delset(dfastates[i]);
}

static void init_dfas(struct nfa *sstate, struct set *acceptset)
{
	struct set *first;
	int i, accept;
	dfastates = xmalloc(MAX_DFAS * sizeof(struct set *));
	/*
	 * init first dfa state
	 * NOTE: First NFA cannot be accepted,
	 *       so epsilon_closure second parameter is set NULL
	 */
	first = newset();
	addset(first, nfastate(sstate));
	epsilon_closure(first, &accept, 0);
	dfastates[0] = first;
	if (accept >= 0) {
		addset(acceptset, 0);
	}

	/* others */
	for (i = 1; i < MAX_DFAS; i++)
		dfastates[i] = NULL;

	/* some internal parmaters */
	ndfas = 1;
	currentdfa = 0;
}

static int add_dfas(struct set *dfa)
{
	if (ndfas >= MAX_DFAS)
		errexit("dfas overflows");
		
	if (dfa && dfastates)
		dfastates[ndfas] = dfa;
	return ndfas++;
}

static int in_dfas(struct set *set)
{
	int i;
	/* no safe check */
	for (i = 0; i < ndfas; i++) {
		if (equset(set, dfastates[i]))
			return i;
	}
	return -1;
}

static struct set *next_dfas(void)
{
	if (currentdfa >= ndfas)
		return NULL;
	return dfastates[currentdfa++];
}

void subsetconstruct(int (*dfatable)[128], struct set *acceptset)
{
	struct set *dfa, *next;
	int nextstate = 0;
	int c, accept, state;

	while (dfa = next_dfas()) {
		for (c = 0; c < MAX_CHARS; c++) {
			/* compute next dfa, to which dfa move on c */
			next = epsilon_closure(move(dfa, c), &accept, 0);
			/* no transition */
			if (!next)
				state = F;
			/* transition from current to next */
			else if ((state = in_dfas(next)) >= 0)
				delset(next);
			else
				state = add_dfas(next);
			dfatable[currentdfa - 1][c] = state;
			if (next && accept >= 0)
				addset(acceptset, ndfas - 1);
		}
	}

}

/*
 * subset construction:
 * convert NFA directed graph to DFA table
 */
int dfaconstruct(struct nfa *sstate, int (**table)[], struct set **acceptset)
{
	/* dfatable[STATES][CHARS] */
	int (*dfatable)[MAX_CHARS];
	struct set *accept;
	int i;

	/* init dfa table */
	dfatable = xmalloc(MAX_TABLESIZE * sizeof(int));

	/* alloc accept set */
	accept = newset();

	/* 
	 * init internal dfa auxiliary method,
	 *  which is used in subsetconstruct()
	 */
	init_dfas(sstate, accept);

	/* subset construction */
	subsetconstruct(dfatable, accept);

	/* free dfa state sets */
	free_dfas();

	/* adjust dfatable real size */
	dfatable = realloc(dfatable, ndfas * MAX_CHARS * sizeof(int));

	/* return value */
	if (table)
		*table = dfatable;
	if (acceptset)
		*acceptset = accept;
		
	return ndfas;
}

void traverse_dfatable(int (*dfatable)[128], int size, struct set *accept)
{
	int i, c;
	for (i = 0; i < size; i++) {
		for (c = 0; c < MAX_CHARS; c++)
			if (dfatable[i][c] >= 0)
				printf("%d --> %d on %c\n",
					i, dfatable[i][c], c);
	}
	for (nextmember(NULL); (i = nextmember(accept)) != -1; i++)
		printf("accept state:%d\n", i);
}

#ifdef DFA_TABLE_TEST

int main(int argc, char **argv)
{
	char line[256];
	struct nfa *nfa;
	struct set *accept;
	/* 
	 * NOTE: int *table1[x] --> size = x*4
	 *       int (*table2)[x] --> size = 4
	 *       typeof(&table2) --> int (**table2p)[x]
	 */
	int (*table)[MAX_CHARS];	/* dfa table */
	int size;	/* dfa table size */

	if (argc != 2)
		errexit("ARGC != 2");

	/* init token stream: interpreting regular expression */
	fileopen(argv[1]);
	/* construct NFA from regular expression */
	init_nfa_buffer();
	nfa = rule();
	traverse_nfa(nfa);
	/* construct dfa table */
	size = dfaconstruct(nfa, &table, &accept);
	traverse_dfatable(table, size, accept);

	return 0;
}

#endif	/* NFA_TABLE_TEST */
