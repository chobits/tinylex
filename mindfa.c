#include <stdio.h>
#include <stdlib.h>
#include "lib.h"
#include "dfa.h"
#include "nfa.h"
#include "set.h"

extern struct dfa *dfastates;
extern int ndfas;

struct set *groups[MAX_GROUPS];
int ngroups;

/* group auxiliary functio n*/
void add_group(int group, int dfa)
{
	dfastates[dfa].group = group;
	addset(groups[group], dfa);
}

void del_group(int dfa)
{
	if (dfastates[dfa].group < 0)
		errexit("del dfa not in some group");
	delset(groups[dfastates[dfa].group], dfa);
	dfastates[dfa].group = F;
}

int new_group(void)
{
	if (ngroups >= MAX_GROUPS)
		errexit("groups overflow!");
	groups[ngroups] = newset();
	return ngroups++;
}

void init_groups(void)
{
	int i, group;

	for (i = 0; i < MAX_GROUPS; i++)
		groups[i] = NULL;

	/* init accept group and non-accept group */
	ngroups = 0;
	new_group();	/* non-accept group: 0 */
	new_group();	/* accept group: 1 */
	for (i = 0; i < ndfas; i++) {
		group = (dfastates[i].accept < 0);
		add_group(group, i);
	}
	/* fix: Is non-accept group? Remove it! */
	if (emptyset(groups[1])) {
		freeset(groups[1]);
		groups[1] = NULL;
		ngroups--;
	}
}

int transgroup(int (*dfatable)[128], int dfa, int c)
{
	int nextdfa;
	nextdfa = dfatable[dfa][c];
	if (nextdfa == F)
		return F;
	return dfastates[nextdfa].group;
}

void part_groups(int (*dfatable)[128], int g, int c)
{
	struct set *group;
	int firstgrp, newgrp;
	int dfa, nextdfa;

	/* get group set */
	group = groups[g];
	newgrp = -1;
	/* get first dfa group */
	nextmember(NULL);
	dfa = nextmember(group);
	if (dfa == -1)
		return;
	firstgrp = nextdfa = transgroup(dfatable, dfa, c);

	/* loop every dfa in group */
	while ((dfa = nextmember(group)) != -1) {
		/* get dfa -- on c --> nextdfa::group */
		if (transgroup(dfatable, dfa, c) != firstgrp) {
			if (newgrp < 0)
				newgrp = new_group();
			del_group(dfa);
			add_group(newgrp, dfa);
		}
	}
}

void minimize_dfa(int (*table)[128], struct set *accept)
{
	int c, group, n;

	init_groups();
	/* minimize dfatable */
	do {
		/* backup ngroups */
		n = ngroups;
		for (c = 0; c < MAX_CHARS; c++) {
			for (group = 0; group < n; group++) {
				part_groups(table, group, c);
			}
		}
	} while (n < ngroups);

	/* reset accept state */
	for (nextmember(NULL); (n = nextmember(accept)) != -1; ) {
		delset(accept, n);
		addset(accept, dfastates[n].group);
	}
}

void minimize_dfatable(int (*table)[128], int (**ret)[128])
{
	struct set *group;
	int (*t)[128];		/* new table */
	int g, dfa, c;
#ifdef DEBUG_MIN_TABLE
	int times = 0;
#endif
	/* alloc new minimized table */
	t = xmalloc(MAX_CHARS * ngroups * sizeof(int));

	/*
	 * loop hierarchy:
	 *   DFAS --> Groups --> Chars
	 */
	for (dfa = 0; dfa < ndfas; dfa++) {
		/*
		 * FIXME: Different dfa can make the same g!
		 *        How to reduce the same g times?
		 */
		g = dfastates[dfa].group;
#ifdef DEBUG_MIN_TABLE
		times++;
#endif
		for (c = 0; c < MAX_CHARS; c++)
			if (table[dfa][c] == F)
				t[g][c] = F;
			else
				t[g][c] = dfastates[table[dfa][c]].group;
	}
	/* set return table */
	if (ret)
		*ret = t;
#ifdef DEBUG_MIN_TABLE
	dbg(" %d * 128(chars) times", times);
#endif
}

void minimize_dfatable2(int (*table)[128], int (**ret)[128])
{
	struct set *group;
	int (*t)[128];		/* new table */
	int g, dfa, c;

#ifdef DEBUG_MIN_TABLE
	int times = 0;
#endif
	/* alloc new minimized table */
	t = xmalloc(MAX_CHARS * ngroups * sizeof(int));
	/*
	 * loop hierarchy:
	 *   Groups --> DFAs --> Chars
	 */
	for (g = 0; g < ngroups; g++) {
		nextmember(NULL);
		if ((dfa = nextmember(groups[g])) != -1) {
#ifdef DEBUG_MIN_TABLE
		times++;
#endif
			for (c = 0; c < MAX_CHARS; c++)
				if (table[dfa][c] == F)
					t[g][c] = F;
				else
					t[g][c] = dfastates[table[dfa][c]].group;
			/* only one table */
		} else {
			errexit("Group is empty!");
		}
	}
	if (ret)
		*ret = t;
#ifdef DEBUG_MIN_TABLE
	dbg(" %d * 128(chars) times", times);
#endif
}

#ifdef MIN_DFA_TEST

void debug_group(void)
{
	int i, g, n;
	fprintf(stderr, "\n-------debug group-----------\n");
	for (i = 0; i < ndfas; i++) {
		g = dfastates[i].group;
		fprintf(stderr, "dfa %d group %d accept %d\n",
			i, g, dfastates[i].accept);
	}
	fprintf(stderr, "\n[groups]\n");
	for (g = 0; g < ngroups; g++) {
		fprintf(stderr, "group:%d dfas:", g);
		for (nextmember(NULL); (n = nextmember(groups[g])) != -1;)
			fprintf(stderr, "%d ", n);
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "-------end debug group-------\n\n");
}

int main(int argc, char **argv)
{
	struct nfa *nfa;
	struct set *accept;

	int (*table)[MAX_CHARS];	/* dfa table */
	int (*mintable)[MAX_CHARS];	/* minimized dfa table */
	int (*mintable2)[MAX_CHARS];	/* minimized dfa table2 */
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
	size = construct_dfa(nfa, &table, &accept);
	traverse_dfatable(table, size, accept);

	/* minimization */
	minimize_dfa(table, accept);

	/* debug */
	debug_group();

	dbg("------minimization 1 test--------");
	minimize_dfatable(table, &mintable);
	traverse_dfatable(mintable, ngroups, accept);

	dbg("------minimization 2 test--------");
	minimize_dfatable2(table, &mintable2);
	traverse_dfatable(mintable2, ngroups, accept);

	return 0;
}

#endif
