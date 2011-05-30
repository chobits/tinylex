#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "macro.h"
#include "lib.h"
#include "nfa.h"
#include "dfa.h"
#include "set.h"
#include "text.h"

static int part = 0;		/* current handing part */

int ispartend(char *line)
{
	if (line[0] == '%' && line[1] == '%')
		return 1;
	return 0;
}

void parse_errx(char *str)
{
	fprintf(stderr, "\nPART %d:\n", part);
	text_errx(str);
}

/* get a valid line, skip blank line and space-starting line */
int get_valid_line(char **linp)
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
 */
void parse_cheader(void)
{
	char *line;
	int len;

	/* set current part */
	part = 1;

	len = get_valid_line(&line);
	/* ignore `%{` or `}%` line tail */
	/* start code: `%{` */
	if (line[0] == '%' && line[1] == '{') {
		while (len = text_getline(&line)) {
			/* end code: `}%` */
			if (line[0] == '}' && line[1] == '%')
				break;
			/* C code header body */
			/* FIXME: need a good interface */
			printf("%s", line);
		}
		if (!len)
			parse_errx("no header end code: }% ");
	} else if (ispartend(line)) {
		/* `%%` means part end */
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
	/* `%%\n` */
	p = text_lookahead(3);
	if (!p)
		text_errx("part2 is too small");
	if (ispartend(p))
		text_errx("part2 is empty");
}

void parse_regexp(void)
{
	struct nfa *nfa;
	struct set *accept;

	int (*table)[MAX_CHARS];	/* dfa table */
	int (*mintable)[MAX_CHARS];	/* minimized dfa table */
	int (*mintable2)[MAX_CHARS];	/* minimized dfa table2 */
	int size;	/* dfa table size */

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

	/* minimization */
	minimize_dfa(table, accept);

	/* debug */
	debug_group();

	/* minimize dfa table */
	minimize_dfatable2(table, &mintable2);
	traverse_dfatable(mintable2, ngroups, accept);
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
		printf("%s", line);
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
		// option handling
	parse_cheader();
	parse_macro();
	/* part2: */
	parse_regexp();
	/* part3: */
	parse_ccode();
}

void open_script(const char *name)
{
	text_open(name);
}
