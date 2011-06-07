#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <macro.h>
#include <lib.h>
#include <nfa.h>
#include <dfa.h>
#include <set.h>
#include <text.h>

extern FILE *fout;

static int part = 0;		/* current handing part */

void parse_errx(char *str)
{
	fprintf(stderr, "\nPART %d:\n", part);
	text_errx(str);
}

/* get a valid line, skip blank line and space-starting line */
int parse_getline(char **linp)
{
	char *line;
	int len;
	/* skip total white space line */
	while ((len = text_getline(&line)) && isspace(line[0])) {
		/* Is there invalid char at line head? */
		if (!isspaceline(line))
			parse_errx("whitespace is redundant");
	}
	if (!len)
		parse_errx("invalid script: empty script");
	if (linp)
		*linp = line;
	return len;
}

/*
 * handling c code header:
 *  %{
 *  <body>
 *  }%
 *
 * + output c code header into lex.yy.c
 */
void parse_cheader(void)
{
	char *line;
	int len;

	/* set current part */
	part = 1;

	len = parse_getline(&line);
	/* ignore other chars of `%{` or `}%` line */
	/* start code: `%{` */
	if (line[0] == '%' && line[1] == '{') {
		while (len = text_getline(&line)) {
			/* end code: `}%` */
			if (line[0] == '}' && line[1] == '%')
				break;
			/* C code header body */
			/* FIXME: need a good interface */
			fprintf(fout, "%s", line);
		}
		if (!len)
			parse_errx("no header end code: }% ");
	} else if (ispartend(line)) {
		/* `%%` means part end, which skip macro definition phase */
		part = 2;
	} else {
		/* save line for macro handing */
		text_backline(line);
	}
}

/*
 * macroname macrotext
 */
void parse_macro(void)
{
	char *line;
	int len;
	/* skip to part 2 */
	if (part == 2)
		return;
	init_macro();
	while (len = text_getline(&line)) {
		if (ispartend(line))
			break;
		add_macro(line, len);
	}
	if (!len)
		parse_errx("no part end: \%\% ");
}

void parse_prepare_regexp(void)
{
	char *p;
	if (skip_whitespace() == EOF)
		text_errx("part2 && part3 is empty");
	/* here, we can lookahead at least one char */
	/* `%%` */
	p = text_lookahead(2);
	/* `x` */
	if (!p)
		text_err("small part2");
	if (p && ispartend(p))
		text_errx("part2 is empty");
}

void parse_regexp(void)
{
	struct nfa *nfa;
	struct set *accept;
	struct set *minaccept;

	int (*table)[MAX_CHARS];	/* dfa table */
	int (*mintable)[MAX_CHARS];	/* minimized dfa table */
	int size;			/* dfa table size */
	int minsize;			/* minimized dfa table size*/

	part = 2;

        /* prepare token stream */
	parse_prepare_regexp();

        /* real parse */
	init_nfa_buffer();
        nfa = machine();
	traverse_nfa(nfa);

	/* construct dfa table */
	size = construct_dfa(nfa, &table, &accept);
	traverse_dfatable(table, size, accept);

	/* minimization: accept will be freed */
	minimize_dfa(table, accept, &minaccept);

	/* minimize dfa table */
	minsize = minimize_dfatable2(table, &mintable);
	traverse_dfatable(mintable, minsize, minaccept);
	/* Now we can free table, should we? */
	free(table);
	freeset(minaccept);
	/* compress dfa table */
	compress_dfatable(mintable, minsize, MAX_CHARS);
	/* free mintable */
	free(mintable);
}

void parse_ccode(void)
{
	int len, c;
	char *line;
	/* ignore `%%` line */
	c = text_getchar();
	if (c != '\n')
		text_getline(&line);
	/* output part 3 */
	while (len = text_getline(&line))
		fprintf(fout, "%s", line);
}

/*
 * Parsing script text:
 * The script format is as follow:
 *
 *  <part1: C file header, pattern macro definition >
 *  %%
 *  <part2: regular expression, associated rule >
 *  %%
 *  <part3: C code >
 *
 */
void parse_script(void)
{
	/* prehandle */
	/* part1: */
	/* TODO:option handling */
	parse_cheader();
	parse_macro();
	/* part2: */
	parse_regexp();
	/* generate part code into lex.yy.c */
	gen_part_code();
	/* part3: */
	parse_ccode();
}

void open_script(const char *name)
{
	text_open(name);
}
