#include <stdio.h>
#include <string.h>
#include "token.h"
#include "reg.h"
#include "set.h"
#include "nfa.h"

#ifdef DEBUG

static int calldepth = 0;
static const char space[] = "                                        ";
#define ENTER()\
do {\
	fprintf(stderr, "%.*s+ %s [%.*s]\n",\
		calldepth * 3, space, __FUNCTION__, yyleng, yytext);\
	calldepth++;\
} while (0)

#define LEAVE()\
do {\
	calldepth--;\
	fprintf(stderr, "%.*s- %s\n", calldepth * 3, space, __FUNCTION__);\
} while (0)

#else
#define ENTER()
#define LEAVE()

#endif /* DEBUG */

extern void reg(struct nfa **start, struct nfa **end);

int nfapos = 0;
struct nfa *nfabuf = NULL;

int nfatop;

#define STACKNFAS 32
struct nfa *nfastack[STACKNFAS];

#define nfastackfull() (nfatop >= (STACKNFAS - 1))
#define nfastackempty() (nfatop < 0)

void init_nfa_buffer(void)
{
	nfabuf = (struct nfa *)xmalloc(sizeof(*nfabuf) * MAXNFAS);
	nfapos = 0;
	nfatop = -1;
}

void freenfa(struct nfa *n)
{
	if (!nfastackfull()) {
		nfastack[++nfatop] = n;
		n->edge = EG_DEL;
	}
	/* else-part just discard this nfa */
}

struct nfa *popnfa(void)
{
	return nfastack[nfatop--];
}

struct nfa *allocnfastack(void)
{
	if (nfastackempty())
		return NULL;
	return popnfa();
}

struct nfa *allocnfa(void)
{
	struct nfa *n;
	if (nfapos >= MAXNFAS)
		errexit("nfa buffer overflows");
	/* alloc nfa from stack */
	n = allocnfastack();
	/* alloc nfa from buffer */
	if (!n)
		n = &nfabuf[nfapos++];
	n->next[0] = NULL;
	n->next[1] = NULL;
	n->edge = EG_EPSILON;
	n->anchor = 0;
}

static token_t current = NIL;

int match(token_t type)
{
	if (current == NIL)
		current = get_token();
	return (current == type);
}

void advance(void)
{
	current = NIL;
}

void matched(token_t type)
{
	if (match(type))
		advance();
	else
		errexit("not matched");
}

/* Terminal Symbol */
void terminal(struct nfa **start, struct nfa **end)
{
	ENTER();
	/* at least one terminal */
	if (match(TERMINAL)) {
		/* real allocing for NFA */
		*start = allocnfa();
		*end = allocnfa();
		(*start)->next[0] = *end;
		/* get terminal symbol */
		(*start)->edge = *yytext;
		advance();
	} else {
		errexit("not matched TERMINAL");
	}
	LEAVE();
}

/* closure -> * closure | + closure | ? closure | epsilon */
/* NOTE: *start & *end is alloced */
void closure(struct nfa **start, struct nfa **end)
{
	ENTER();
	struct nfa *istart, *iend;
	while (match(ASTERISK) || match(ADD) || match(QUESTION)) {
		istart = allocnfa();
		iend = allocnfa();
		istart->next[0] = *start;
		(*end)->next[0] = iend;
		if (match(ASTERISK) || match(ADD))
			(*end)->next[1] = *start;
		if (match(ASTERISK) || match(QUESTION))
			istart->next[1] = iend;
		/* update */
		*start = istart;
		*end = iend;
		advance();
	}
	LEAVE();
}

/*
 * parentheses -> ( regular ) | <terminal>
 * NOTE: `( epsilon )` is error!
 */
void parentheses(struct nfa **start, struct nfa **end)
{
	ENTER();
	if (match(LP)) {		/* ( */
		advance();
		reg(start, end);
		matched(RP);		/* ) */
	} else {
		terminal(start, end);
	}
	LEAVE();
}

/*
 * handle: string | char1 - char2 | epsilon
 */
void do_dash(struct nfa *nfa)
{
	int prev = 0, i;
	ENTER();
	/* epsilon */
	if (match(RSB))
		return;

	if (!nfa->set)
		nfa->set = newset();
	/* all token is interpreted as lexeme char */
	while (!match(RSB)) {
		/* situation: e.g: a-z */
		if (match(DASH)) {
			advance();
			if (prev && !match(RSB)) {
				for (i = prev; i <= *yytext; i++)
					addset(nfa->set, i);
				advance();
			}
			prev = 0;
		} else {
			prev = *yytext;
			advance();
			addset(nfa->set, prev);
		}
	}
	LEAVE();
}

/*
 * squarebrackets -> [^ string ]
 *                |  [  string ]
 *                |  [^]
 *                |  []
 *                |  .
 *                |  parentheses
 */
void squarebrackets(struct nfa **start, struct nfa **end)
{
	ENTER();
	if (match(LSB)) {		/* [ */
		advance();
		*start = allocnfa();
		*end = allocnfa();
		(*start)->next[0] = *end;
		(*start)->edge = EG_CCL;
		/* uparrow ^ */
		if (match(UPARROW))	/* [^ ... ] */
			advance();	/* not set anchor field here */

		/*  epsilon or string */
		do_dash(*start);
		matched(RSB);		/* ] */
	} else if (match(DOT)) {	/* dot(.) */
		advance();
		*start = allocnfa();
		*end = allocnfa();
		(*start)->next[0] = *end;
		(*start)->edge = EG_CCL;
		/* newset containing all chars except newline */
		(*start)->set = newset();
		addset((*start)->set, '\n');
		complset((*start)->set);
	} else {
		parentheses(start, end);
	}
	LEAVE();
}

/*
 * concatenation -> squarebrackets closure
 */
void concatenation(struct nfa **start, struct nfa **end)
{
	ENTER();
	squarebrackets(start, end);
	closure(start, end);
	LEAVE();
}

int cc_first_set(void)
{
	if (current == NIL)
		current = get_token();
	switch (current) {
	case RP:	/* ) */
	case OR:	/* | */
	case _EOF:	/* EOF */
	case DOLLAR:	/* $ */
		return 0;
		break;
	case DASH:	/* - */
	case ADD:	/* + */
	case ASTERISK:	/* * */
	case QUESTION:	/* ? */
	case RSB:	/* ] */
	case UPARROW:	/* ^ */
		errexit("not matched concatenation");
		return 0;
		break;
	default:
		break;
	}
	return 1;
}

/*
 * regor -> concatenation regor | concatenation
 */
void regor(struct nfa **start, struct nfa **end)
{
	struct nfa *istart, *iend;
	ENTER();
	if (cc_first_set())
		concatenation(start, end);
	else
		errexit("regor not match concatenation first set");

	while (cc_first_set()) {
		concatenation(&istart, &iend);
		memcpy(*end, istart, sizeof(*istart));
		freenfa(istart);
		/* update new end */
		*end = iend;
	}
	LEAVE();
}


/*
 * reg -> regor OR reg
 *      | regor
 */
void reg(struct nfa **start, struct nfa **end)
{
	struct nfa *istart, *iend;
	ENTER();
	regor(start, end);

	while (match(OR)) {			/* regor | ... */
		advance();
		/* new start */
		istart = allocnfa();
		istart->next[0] = *start;
		/* or-part */
		regor(istart->next + 1, &iend);
		/* new end */
		(*end)->next[0] = iend->next[0] = allocnfa();
		/* update to new start or new end */
		*start = istart;
		*end = (*end)->next[0];
	}
	LEAVE();
}

/*
 * rule -> ^ reg | reg $ | reg
 */
struct nfa *rule(void)
{
	struct nfa *start, *end;
	int anchor = AC_NONE;
	if (match(UPARROW)) {			/* ^... */
		advance();
		start = allocnfa();
		start->edge = '\n';
		reg(start->next, &end);
		anchor |= AC_START;
	} else {
		reg(&start, &end);
	}

	if (match(DOLLAR)) {			/* ...$ */
		advance();
		end->next[0] = allocnfa();
		end->edge = '\n';
		end = end->next[0];
		anchor |= AC_END;
	}
	/* why put anchor to last NFA? */
	end->anchor = anchor;
	end->edge = EG_EMPTY;

	matched(_EOF);
	return start;
}

/* output set chars */
void printset(struct set *set)
{
	int i, bitnr;
	printf("[%smaps]", set->compl ? "complemented " : "");
	if (set->compl)
		return;
	printf("\n      ");
	for (i = 0; i < set->ncells; i++) {
		if (!set->map[i])
			continue;
		for (bitnr = 0; bitnr < 8; bitnr++) {
			if ((1 << bitnr) & set->map[i])
				putchar(BIT_CELL(i, bitnr));
		}
	}
}

/* output nfa::anchor */
void printanchor(int anchor)
{
	switch (anchor) {
	case AC_NONE:
		printf("    ");
		break;
	case AC_START:
		printf(" ^  ");
		break;
	case AC_END:
		printf(" $  ");
		break;
	case AC_BOTH:
		printf(" ^ $");
		break;
	default:
		break;
	}
}

/* output nfa edge information */
void printedge(struct nfa *nfa, struct nfa *pstart)
{
	if (nfa->next[0]) {
		if (nfa->next[0]) {
			printf("state: %4d --> %4d on ",
					nfa - pstart,
					nfa->next[0] - pstart);
			if (nfa->edge > 0) {
				printf("[%c]", nfa->edge);
			} else if (nfa->edge == EG_EPSILON) {
				printf("[epsilon]");
			} else if (nfa->edge == EG_CCL) {
				printset(nfa->set);
			}
		}
		/* another transimit */
		if (nfa->next[1]) {
			printf("\n           ");
			printf("state: %4d --> %4d on ",
					nfa - pstart,
					nfa->next[1] - pstart);
			if (nfa->edge > 0) {
				printf("[%c]", nfa->edge);
			} else if (nfa->edge == EG_EPSILON) {
				printf("[epsilon]");
			} else if (nfa->edge == EG_CCL) {
				printset(nfa->set);
			}
		}
	} else {
		if (nfa->edge == EG_EMPTY)
			printf("(accept)");
		else
			errexit("nfa accept is not empty");
	}
}

/*
 * @pstart staring nfa in physical memory
 * @n      nfa number
 * @lstart logical staring nfa
 */
void __traverse_nfa(struct nfa *pstart, int n, struct nfa *lstart)
{
	struct nfa *nfa;
	int i;
	printf("start state: %d\n", nfastate(lstart));
	for (nfa = pstart, i = 0; i < n; i++, nfa++) {
		/* skip lazy deleted one */
		if (nfa->edge == EG_DEL)
			continue;
		printf("[%4d] ", i);
		printanchor(nfa->anchor);
		printedge(nfa, pstart);
		printf("\n");
	}
}

void traverse_nfa(struct nfa *lstart)
{
	printf("\n[===   NFAS   ===]\n");
	__traverse_nfa(nfabuf, nfapos, lstart);
	printf("[=== NFAS end ===]\n");
}

#ifdef NFATEST

int main(int argc, char **argv)
{
	struct nfa *nfa;
	if (argc == 2)
		fileopen(argv[1]);
	init_nfa_buffer();
	nfa = rule();
	traverse_nfa(nfa);
}

#endif
