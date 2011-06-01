#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <token.h>
#include <reg.h>
#include <set.h>
#include <nfa.h>
#include <text.h>

#ifdef DEBUG

static int calldepth = 0;
static const char _space[] = "                                        ";
#define ENTER()\
do {\
	fprintf(stderr, "%.*s+ %s [%.*s]\n",\
		calldepth * 3, _space, __FUNCTION__, yyleng, yytext);\
	calldepth++;\
} while (0)

#define LEAVE()\
do {\
	calldepth--;\
	fprintf(stderr, "%.*s- %s\n", calldepth * 3, _space, __FUNCTION__);\
} while (0)

#else
#define ENTER()
#define LEAVE()

#endif /* DEBUG */


int nfapos = 0;
struct nfa *nfabuf = NULL;

int nfatop;

#define STACKNFAS 32
struct nfa *nfastack[STACKNFAS];

#define nfastackfull() (nfatop >= (STACKNFAS - 1))
#define nfastackempty() (nfatop < 0)

static char *emptyaction = "[<empty accept action>]";

void init_nfa_buffer(void)
{
	nfabuf = (struct nfa *)xmalloc(sizeof(*nfabuf) * MAXNFAS);
	nfapos = 0;
	nfatop = -1;
}

void free_nfas(void)
{
	struct nfa *n;
	int i;
	for (n = nfabuf, i = 0; i < nfapos; i++, n++)
		if (n->accept && n->accept != emptyaction)
			free(n->accept);
	free(nfabuf);
}

void freenfa(struct nfa *n)
{
	if (!nfastackfull()) {
		nfastack[++nfatop] = n;
		n->edge = EG_DEL;
		if (n->accept && n->accept != emptyaction)
			free(n->accept);
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
	n->accept = NULL;
	n->edge = EG_EPSILON;
	n->anchor = 0;
	return n;
}

static token_t current = NIL;

/* the yytext is valid when match return 1 */
int match(token_t type)
{
	current = get_token();
	if (current != _EOF)
		back_token();
	return (current == type);
}

void advance(void)
{
	get_token();
}

void matched(token_t type)
{
	current = get_token();
	if (current != type)
		errexit("not matched");
}

/* Terminal Symbol */
void terminal(struct nfa **start, struct nfa **end)
{
	ENTER();

	/* at least one terminal */
	if (match(L)) {
		advance();
		/* real allocing for NFA */
		*start = allocnfa();
		*end = allocnfa();
		(*start)->next[0] = *end;
		/* get terminal symbol */
		(*start)->edge = *yytext;
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
	while (match(AST) || match(ADD) || match(QST)) {
		istart = allocnfa();
		iend = allocnfa();
		istart->next[0] = *start;
		(*end)->next[0] = iend;
		if (match(AST) || match(ADD))
			(*end)->next[1] = *start;
		if (match(AST) || match(QST))
			istart->next[1] = iend;
		/* update */
		*start = istart;
		*end = iend;
		advance();
	}
	LEAVE();
}

extern void regstr(struct nfa **, struct nfa **);
/*
 * parentheses -> ( regstr ) | <terminal>
 * NOTE: `( epsilon )` is error!
 */
void parentheses(struct nfa **start, struct nfa **end)
{
	ENTER();
	if (match(LP)) {		/* ( */
		advance();
		regstr(start, end);
		matched(RP);		/* ) */
	} else {
		terminal(start, end);
	}
	LEAVE();
}

/*
 * do_dash -> ^ | epsilon | chars | char-char
 */
void do_dash(struct nfa *nfa)
{
	int prev = 0, compl = 0, i;

	ENTER();
	/* uparrow ^ */
	if (match(UPA)) {	/* [^ ... ] */
		advance();	/* not set anchor field here */
		compl = 1;
	}

	/* TODO:epsilon */
	if (match(RSB))
		text_errx("cannot handling [^] or []");

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
			advance();
			prev = *yytext;
			addset(nfa->set, prev);
		}
	}
	if (compl)
		complset(nfa->set);
	LEAVE();
}

/*
 * squarebrackets -> [ do_dash ]
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
	current = get_token();
	if (current != _EOF)
		back_token();
	switch (current) {
	/* concatenation follow set */
	case RP:	/* ) */
	case OR:	/* | */
	case _EOF:	/* EOF */
	case EOL:
	case DOLLAR:	/* $ */
	case SPACE:	/* space or tab */
		return 0;
		break;
	/* concatenation first set */
	case LSB:	/* [ */
	case DOT:	/* . */
	case LP:	/* ( */
	case L:		/* lexeme */
		return 1;
		break;
	case DASH:	/* - */
	case ADD:	/* + */
	case AST:	/* * */
	case QST:	/* ? */
	case RSB:	/* ] */
	case UPA:	/* ^ */
	case LCP:	/* { */
	case RCP:	/* } */
		errexit("not matched concatenation");
		break;
	default:
		errexit("Unrecognized token");
		break;
	}
	/* dummy return value */
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
		/* free overlaping nfa */
		istart->accept = NULL;
		freenfa(istart);
		/* update new end */
		*end = iend;
	}
	LEAVE();
}

/*
 * regstr -> regor OR regstr
 *         | regor
 */
void regstr(struct nfa **start, struct nfa **end)
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
 * regexp -> ^ regstr | regstr $ | regstr
 */
void regexp(struct nfa **startp, struct nfa **endp)
{
	struct nfa *start, *end;
	ENTER();

	int anchor = AC_NONE;
	if (match(UPA)) {			/* ^... */
		advance();
		start = allocnfa();
		start->edge = '\n';
		regstr(start->next, &end);
		anchor |= AC_START;
	} else {
		regstr(&start, &end);
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

	/* set return value */
	*startp = start;
	*endp = end;
	LEAVE();
}

/*
 * action -> one-line string(EOL) | { string } space EOL | epsilon
 *        |  one-line string(_EOF) | { string } space _EOF
 */
void action(struct nfa *end)
{
	char *line;
	int len;
	ENTER();
	/* epsilon */
	if (match(EOL) || match(_EOF)) {
		end->accept = emptyaction;
		advance();
		return;
	}

	/* `{ string }` */
	if (match(LCP)) {
		/*
		 * TODO: support { string }
		 * Should I need a C parser?
		 */
		errexit("not support { string }");
	} else {
		len = text_getline(&line);
		/* Should it happen? */
		if (len <= 1)
			errexit("error new line");
		end->accept = strdup(line);
		/* elimite tail newline */
		if (end->accept[len - 1] == '\n')
			end->accept[len - 1] = '\0';
	}
	LEAVE();
}

/*
 * specail handle: skip blank char(space or tab),
 *                 not using token stream
 */
void space(void)
{
	int c;
	do {
		c = text_getchar();
	} while (isblank(c));
	if (c != EOF)
		text_backchar();
}

/*
 * rule -> space regexp space action
 */
struct nfa *rule(void)
{
	struct nfa *start, *end;
	ENTER();
	space();
	regexp(&start, &end);
	space();
	action(end);
	LEAVE();
	return start;
}

/*
 * specail handle: match partend `%%`,
 *                 not using token stream
 */
int matchendpart(void)
{
	char *p;
	p = text_lookahead(2);
	if (!p || !ispartend(p))
		return 0;
	/* skip first `%` for andvance() in machine() */
	advance();
	if (!match(L) || *yytext != '%')	/* second % */
		text_errx("match end part `\%\%` error");
	return 1;
}

/*
 * machine -> rule <spaceline> machine
 *          | rule <spaceline> EOP
 */
struct nfa *machine(void)
{
	struct nfa *start, *s;
	ENTER();

	/* prealloc */
	start = s = allocnfa();
	s->next[0] = rule();

	while (1) {
		/* <spaceline> */
		skip_whitespace();
		if (matchendpart() || match(_EOF))
			break;
		s->next[1] = allocnfa();
		s = s->next[1];
		s->next[0] = rule();
	}

	/* part end `%%` */
	advance();

	LEAVE();
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
		text_open(argv[1]);
	init_nfa_buffer();
	nfa = machine();
	traverse_nfa(nfa);
}

#endif
