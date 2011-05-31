#include <stdio.h>
#include <string.h>
#include <lib.h>
#include <reg.h>
#include <set.h>
#include <nfa.h>

#define MAXSTACKNFAS 128

#define RECURSION_EPSILON_CLOSURE
#ifdef RECURSION_EPSILON_CLOSURE

/*  real recursion computation function */
void e_closure(struct nfa *nfa, struct set *stateset, char **accept)
{
	int state;
	if (!nfa)
		return;

	state = nfastate(nfa);
	/* greedy algorithm: accept as much states as possible */
	if (!nfa->next[0]) {
		if (!nfa->accept)
			errexit("no accept action string");
		if (accept)
			*accept = nfa->accept;
	}

	if (nfa->edge == EG_EPSILON) {
		/* next 0 */
		if (nfa->next[0] &&
			!test_add_set(stateset, nfastate(nfa->next[0])))
			e_closure(nfa->next[0], stateset, accept);
		/* next 1 */
		if (nfa->next[1] &&
			!test_add_set(stateset, nfastate(nfa->next[1])))
			e_closure(nfa->next[1], stateset, accept);
	}
}

/*
 * recursion implemented computation for epsilon closure set from input
 * @dup    if 1, output set realloced
 *         otherwise, epsilon closure set is added into input
 *          and return input
 * @accept accept state
 * @input  start state for computing epsilon closure
 *
 * input only contains sstate
 */
struct set *epsilon_closure(struct set *input, char **accept, int dup)
{
	int start_state[MAXSTACKNFAS];
	int state;
	struct set *output;

	/*
	 * We cannot using e_closure(nfa, output),
	 * because sstate is already in output.
	 */
	if (accept)
		*accept = NULL;

	if (!input)
		return NULL;

	/* set return value(epsilon closure set) */
	output = dup ? dupset(input) : input;

	/* buffering start state int input */
	for (startmember(input), state = 0; state < MAXSTACKNFAS; state++) {
		start_state[state] = nextmember(input);
		if (start_state[state] == -1) {
			state--;
			break;
		}
	}

	if (state >= MAXSTACKNFAS)
		errexit("state stack overflows");
	/* computing epsilon closure of every start state in input set */
	for (; state >= 0; state--)
		e_closure(statenfa(start_state[state]), output, accept);

	return output;
}

#else	/* RECURSION_EPSILON_CLOSURE */

/*
 * stack implemented computation for epsilon closure set from input
 */
struct set *epsilon_closure(struct set *input, int *accept, int dup)
{
	struct nfa *nfastack[MAXSTACKNFAS];
	struct nfa *nfa;
	int top = -1, state;
	struct set *output;
	int i;

	if (!input)
		return NULL;
	/* set return value (output set)*/
	output = dup ? dupset(input) : input;
	/* push all states in the input set onto nfastack */
	for_each_member(state, input) {
		nfastack[++top] = statenfa(state);
		if (top >= MAXSTACKNFAS)
			errexit("nfa stack overflows");
	}

	/* init accpet state */
	if (accept)
		*accept = MAX_INT;
	/* real handling */
	while (top >= 0) {
		nfa = nfastack[top--];
		/* NOTE: terminal nfa is epsilon but its next is NULL */
		/* epsilon transition from nfa to nfa->next[0|1] */
		if (nfa->edge == EG_EPSILON) {
			if (nfa->next[0] &&
			!test_add_set(output, nfastate(nfa->next[0]))) {
				nfastack[++top] = nfa->next[0];
				if (top >= MAXSTACKNFAS)
					errexit("nfa stack overflows");
			}
			if (nfa->next[1] &&
			!test_add_set(output, nfastate(nfa->next[1]))) {
				nfastack[++top] = nfa->next[1];
				if (top >= MAXSTACKNFAS)
					errexit("nfa stack overflows");
			}
		}
		if (!nfa->next[0] && accept && nfastate(nfa) < *accept)
			*accept = nfastate(nfa);
	}
	/* if not changed */
	if (accept && *accept == MAX_INT)
		*accept = -1;
	return output;
}

#endif	/* !RECURSION_EPSILON_CLOSURE */

/*
 * return a set that contains all states that can be reached by
 * making transitions on @state from NFA set @input
 *
 * @output is moved from @input on @state
 */
struct set *move(struct set *input, int state)
{
	struct set *output = NULL;
	struct nfa *nfa;
	int i;

	for_each_member(i, input) {
		nfa = statenfa(i);
		if ((nfa->edge == EG_CCL && memberofset(state, nfa->set)) ||
				(nfa->edge == state)) {
			if (!output)
				output = newset();
			/* assert nfa->next[1] == NULL */
			addset(output, nfastate(nfa->next[0]));
		}
	}

	return output;
}

void printstateset(struct set *set)
{
	int state;
	for_each_member(state, set)
		printf("%d ", state);
	printf("\n");
}

#ifdef INTERPRET

void usage(void)
{
	fprintf(stderr, "egrep_like regexpfile file\n");
	exit(EXIT_FAILURE);
}

static int lineno;

int grep(char *line, struct nfa *nfa, char **endstr)
{
	struct set *start, *end;
	int accept, prevaccept;
	int c, buflen;
	char *buf, *bufpos;


	/* init accept state */
	accept = -1;
	prevaccept = -1;

	/* init start state set */
	start = newset();
	addset(start, nfastate(nfa));
	start = epsilon_closure(start, &accept, 0);

	/* get input string */
	buf = line;
	buflen = strlen(buf);
	bufpos = buf;

	/* matching: non-greedy algorithm */
	while (c = *bufpos) {
		end = move(start, c);
		if (!end)
			break;
		end = epsilon_closure(end, &accept, 0);

		if (accept >= 0) {
			prevaccept = accept;
			accept = -1;
		}

		/* for next loop */
		freeset(start);
		start = end;
		end = NULL;
		bufpos++;
	}
	if (start)
		freeset(start);
	if (end)
		freeset(end);
	/* output matched string */
	if (prevaccept >= 0) {
		if (endstr)
			*endstr = bufpos;
		return 1;
	}
	/* not matched */
	return 0;
}


/* egrep-like test program */
int main(int argc, char **argv)
{
	char line[256];
	char *lp, *ep;
	FILE *f;
	struct nfa *nfa;
	/* handle arguments */
	if (argc != 3)
		usage();
	/* init token stream: interpreting regular expression */
	open_script(argv[1]);
	parse_cheader();
	parse_macro();
	/* construct NFA from regular expression */
        skip_whitespace();
	init_nfa_buffer();
	nfa = machine();

	/* init file stream */
	f = fopen(argv[2], "r");
	if (!f)
		errexit("fopen");

	/* grep */
	lineno = 0;
	while (fgets(line + 1, 255, f)) {
		line[0] = '\n';		/* prev line tail */
		lineno++;
		lp = line;
		ep = NULL;
		while (*lp) {
			if (grep(lp, nfa, &ep))
				break;
			lp++;
		}
		/* output matched pattern */
		if (ep) {
			/* elimite prev line tail */
			if (*lp == '\n') {
				lp++;
			}
			/* red matched pattern */
			printf(green(%d)cambrigeblue(:)"%.*s"red(%.*s)"%s",
					lineno,
					lp - line - 1, line + 1,
					ep - lp, lp,
					ep);
		}
	}
	fclose(f);

	return 0;
}

#endif	/* INTERPRET */
