#include <stdio.h>
#include <stdlib.h>
#include <dfa.h>

#define DEF_OUTPUT_FILE	"lex.yy.c"
#define DEF_PART_LIB	"lex.yy.part.c"

FILE *fpart;
FILE *fout;
int order = 0;

/*
 * print switch-case format accept action
 */
void print_accept_action(FILE *fp)
{
	int i;
	for (i = sgroup; i < ngroups; i++)
		if (dfastates[i].accept)
			fprintf(fp,
				"\t\t\tcase %d:\n"
				"\t\t\t\t%s\n"
				"\t\t\t\tbreak;\n",
				i, dfastates[i].accept->action);
}

/* print yyaccept[] anchor table */
void print_accept_array(FILE *fp)
{
	int i;
	char *yy_acp;
	fprintf(fp, "static int yyaccept[%d] = {\n", ngroups - sgroup);
	for (i = 0; i < sgroup; i++)
		fprintf(fp, "\tYY_NOAC,\n");
	for (; i < ngroups; i++) {
		if (dfastates[i].accept) {
			switch (dfastates[i].accept->anchor) {
			case AC_NONE:
				yy_acp = "YY_AC_NONE";
				break;
			case AC_START:
				yy_acp = "YY_AC_START";
				break;
			case AC_END:
				yy_acp = "YY_AC_END";
				break;
			case AC_BOTH:
				yy_acp = "YY_AC_BOTH";
				break;
			default:
				errexit("Unknown accept anchor");
				break;
			}
		} else {
			yy_acp = "YY_NOAC";
		}
		fprintf(fp, "\t%s,\n", yy_acp);
	}

	fprintf(fp, "};\n");
}


void part_accept_action(void)
{
	print_accept_action(fout);
}

void part_transition_table(void)
{
	print_redundant_table(fout);
}

void part_accept_array(void)
{
	print_accept_array(fout);
}

/* 
 * generate other part code in order:
 *      <table> -> <accept> -> <action>
 */
void other_part_code(char *pattern)
{
	if (order == 0 && strncmp(pattern, "table", 5) == 0) {
		part_transition_table();
		order = 1;
	} else if (order == 1 && strncmp(pattern, "accept", 6) == 0) {
		part_accept_array();
		order = 2;	
	} else if (order == 2 && strncmp(pattern, "action", 6) == 0) {
		part_accept_action();
		order = 3;
	} else {
		fprintf(stderr, "part lib:%s is error\n", pattern);
		exit(EXIT_FAILURE);
	}
}

void gen_part_code(void)
{
	char line[256];	
	while (fgets(line, 256, fpart) != NULL) {
		if (line[0] == '!')	/* e.g: `!table` */
			other_part_code(line + 1);
		else
			fprintf(fout, "%s", line);
	}

	if (order != 3) {
		fprintf(stderr, "part lib:%d is error\n", order);
		exit(EXIT_FAILURE);
	}
}

void code_open(char *file, char *part)
{
	/* create generated code file */
	if (!file)
		file = DEF_OUTPUT_FILE;
	fout = fopen(file, "w");
	if (!fout) {
		perror("fopen");
		fprintf(stderr,
				"\tgenerated file %s cannot be opened\n", file);
		exit(EXIT_FAILURE);
	}
	/* open part code library */
	if (!part)
		part = DEF_PART_LIB;
	fpart = fopen(part, "r");
	if (!fpart) {
		perror("fopen");
		fprintf(stderr, "\tpart file %s cannot be opened\n", part);
		exit(EXIT_FAILURE);
	}
}
