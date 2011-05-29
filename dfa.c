#include <stdio.h>
#include <stdlib.h>
#include "set.h"
#include "nfa.h"
#include "dfa.h"
#include "lib.h"

/* dfa sets auxiliary function */
int currentdfa, ndfas;
struct dfa *dfastates = NULL;

static void free_dfas(void)
{
	int i;
	for (i = 0; i < ndfas; i++)
		freeset(dfastates[i].states);
	free(dfastates);
}

static void init_dfas(struct nfa *sstate, struct set *acceptset)
{
	struct set *first;
	int i, accept;
	dfastates = xmalloc(MAX_DFAS * sizeof(struct dfa));
	/* others */
	for (i = 0; i < MAX_DFAS; i++) {
		dfastates[i].group = -1;
		dfastates[i].states = NULL;
		dfastates[i].accept = -1;
	}
	/*
	 * init first dfa state
	 * NOTE: First NFA cannot be accepted, 
	 *       so epsilon_closure second parameter is set NULL 
	 */
	first = newset();
	addset(first, nfastate(sstate));
	epsilon_closure(first, &accept, 0);
	dfastates[0].states = first;
	if (accept >= 0) {
		addset(acceptset, 0);
		dfastates[0].accept = accept;
	}


	/* some internal parmaters */
	ndfas = 1;
	currentdfa = 0;
}

static int add_dfa(struct set *nfastates, int accept)
{
	if (ndfas >= MAX_DFAS)
		errexit("dfas overflows");

	if (nfastates && dfastates) {
		dfastates[ndfas].states = nfastates;
		dfastates[ndfas].accept = accept;
	}
	return ndfas++;
}

static int in_dfa(struct set *states)
{
	int i;
	/* no safe check */
	for (i = 0; i < ndfas; i++) {
		if (equset(states, dfastates[i].states))
			return i;
	}
	return -1;
}

int state_dfa(struct dfa *dfa)
{
	return dfa - dfastates;
}

static struct dfa *next_dfa(void)
{
	if (currentdfa >= ndfas)
		return NULL;
	return &dfastates[currentdfa++];
}

void subsetconstruct(int (*dfatable)[128], struct set *acceptset)
{
	struct dfa *dfa;
	struct set *next;
	int nextstate = 0;
	int c, accept, state;

	while (dfa = next_dfa()) {
		for (c = 0; c < MAX_CHARS; c++) {
			/* compute next dfa, to which dfa move on c */
			accept = -1;
			next = move(dfa->states, c);
			next = epsilon_closure(next, &accept, 0);
			/* no transition */
			if (!next)
				state = F;
			/* transition from current to next */
			else if ((state = in_dfa(next)) >= 0)
				freeset(next);
			else
				state = add_dfa(next, accept);
			dfatable[state_dfa(dfa)][c] = state;
			if (accept >= 0)
				addset(acceptset, ndfas - 1);
		}
	}
}

/*
 * subset construction:
 * convert NFA directed graph to DFA table
 */
int construct_dfa(struct nfa *sstate, int (**table)[], struct set **acceptset)
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
	printf("\n-====== debug dfa table ========-\n");
	for (i = 0; i < size; i++) {
		for (c = 0; c < MAX_CHARS; c++)
			if (dfatable[i][c] >= 0)
				printf("  %d --> %d on %c\n",
					i, dfatable[i][c], c);
	}
	for (nextmember(NULL); (i = nextmember(accept)) != -1; i++)
		printf(" accept state:%d\n", i);
	printf("-====== end debug dfa table ====-\n");
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
	nfa = machine();
	traverse_nfa(nfa);
	/* construct dfa table */
	size = construct_dfa(nfa, &table, &accept);
	traverse_dfatable(table, size, accept);

	/* free dfa state sets */
	free_dfas();

	return 0;
}

#endif	/* NFA_TABLE_TEST */
