#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define dbg(fmt, arg...) fprintf(stderr, "%s "fmt"\n", __FUNCTION__, ##arg)

FILE *yyout = NULL;	/* default output file */

/* error handle */
void yyerrx(char *str)
{
	if (errno)
		perror(str);
	else
		fprintf(stderr, "ERROR: %s\n", str);
	exit(EXIT_FAILURE);
}

/* yylex() output text stream */
char *yytext;
int yyleng;
int yylineno;
static char *yysmark;		/* start mark for yytext */
static char *yyemark;		/* end mark for yytext */

char *yy_text(void)
{
	return yysmark;
}

int yy_leng(void)
{
	return yyemark - yysmark;
}

/* low-level I/O stream */
#define YY_AHEADSIZE 16
#define YY_BUFSIZE 4096

#define YY_LOOKAHEAD (&yyend[-YY_AHEADSIZE])
#define YY_BUF_START (&yybuf[0])
#define YY_BUF_END (&yybuf[YY_BUFSIZE])

static char yybuf[YY_BUFSIZE];
static char *yyend;		/* current buffer end */
static char *yypos;		/* current position of I/O stream */
static int yyfd = 0;		/* default input file is standard input */
static int yyeof;
static int yyreallineno;
static int yyprevlineno;

void yy_init(void)
{
	yypos = YY_BUF_START;
	*yypos = '\n';		/* first line start anchor `^` */
	yyend = YY_BUF_START + 1;
	yysmark = yyemark = YY_BUF_START;
	yyeof = 0;
	yytext = "";
	yyleng = 0;
	yylineno = 0;
	yyreallineno = 0;
	/* default */
	if (!yyout)
		yyout = stdout;
}

int yy_lookahead(void)
{
	if (yypos < yyend)
		return *yypos;
	else
		return EOF;
}

void yy_save_buf(void)
{
	char *p;
	int marklen;
	int poslen;
	marklen = yyemark - yysmark;
	poslen = yypos - yysmark;

	p = YY_BUF_START;

	while (yysmark < yyend)
		*p++ = *yysmark++;

	yysmark = YY_BUF_START;
	yyemark = yysmark + marklen;
	yypos = yysmark + poslen;
	yyend = p;
}

void yy_fill_buf(void)
{
	int n;
	if (yysmark < yyend)
		yy_save_buf();
	n = read(yyfd, yyend, YY_BUF_END - yyend);
	if (n < 0) {
		yyerrx("read");
	} else if (n > 0) {
		yyend += n;
	} else {
		yyeof = 1;
	}
}

void yy_advance(void)
{
	if (!yyeof && yypos >= YY_LOOKAHEAD)
		yy_fill_buf();
	if (yypos < yyend) {
		if (*yypos == '\n')
			yyreallineno++;
		yypos++;
	}
}

void yy_move_start(void)
{
	yysmark++;
}

void yy_mark_start(void)
{
	yyemark = yysmark = yypos;
	yylineno = yyreallineno;
}

void yy_mark_end(void)
{
	yyprevlineno = yyreallineno;
	yyemark = yypos;
}

void yy_back_end(void)
{
	yyemark--;
}

void yy_back(void)
{
	if (*yypos == '\n')
		yyreallineno--;
	yypos--;
}

void yy_mark_pos(void)
{
	yyreallineno = yyprevlineno;
	yypos = yyemark;
}

#define YY_TERMINAL		'\0'
static char *yytermpos;		/* terminal postion */
static char yytermchar;		/* backup char for original terminal postion */

void yy_term(void)
{
	yytermpos = yypos;
	yytermchar = *yypos;
	*yypos = YY_TERMINAL;
}

void yy_unterm(void)
{
	if (yytermpos) {
		*yytermpos = yytermchar;
		yytermchar = YY_TERMINAL;
		yytermpos = NULL;
	}
}

/* state transition table */
#define yy_next(state, c) com_table[row_map[state]][col_map[c]]

!table


/* accept anchor */
#define YY_NOAC		0
#define YY_AC_START	1
#define YY_AC_END	2
#define YY_AC_BOCH	3
#define YY_AC_NONE	4

!accept

#define YYF		(-1)
#define ECHO()	fprintf(yyout, "%s", yytext)

int yylex(void)
{
	static int yyfirst = 1;	/* first call yylex() */
	int yystate;		/* current state */
	int yyprev;		/* prev state */
	int yynstate;		/* next state */
	int yylastaccept;	/* last accept state */
	int yyanchor;		/* accept anchor */
	int yylook;		/* gotten char */

	if (yyfirst) {
		yy_init();
		yyfirst = 0;
	}

	/* init */
	yy_unterm();
	yystate = 0;
	yyanchor = 0;
	yylastaccept = 0;
	yy_mark_start();

	while (1) {
		/* get next state according to lookaheah char */
		yylook = yy_lookahead();

		if (yylook == EOF) {
			if (yylastaccept)
				yynstate = YYF;
			else {
				/*
				 * FIXME: some tailing chars will be ignored!
				 */
				yytext = "";
				yyleng = 0;
				return 0;
			}
		} else {
			yynstate = yy_next(yystate, yylook);
		}
		/* dbg("%d -> %d on [%c]", yystate, yynstate, yylook); */

		if (yynstate == YYF) {
			if (yylastaccept)
				yy_mark_pos();
			else
				yy_advance();

			/* handle ^ , $ operator */
			if (yyanchor & YY_AC_START)
				/* FIXME: '\n' is ignored */
				yy_move_start();

			if (yyanchor & YY_AC_END) {
				yy_back_end();
				yy_back();
			}

			yy_mark_end();
			yy_term();
			yytext = yy_text();
			yyleng = yy_leng();

			/* handle accept action */
			switch (yylastaccept) {
!action
			default:
				ECHO();
				break;
			}

			yy_unterm();
			yystate = 0;
			yylastaccept = 0;
			yyanchor = 0;
			yy_mark_start();
		} else {
			yy_advance();
			yyanchor = yyaccept[yynstate];
			/* Acceptable? */
			if (yyanchor) {
				yy_mark_end();
				yylastaccept = yynstate;
			}
			yystate = yynstate;
		}
	}
}

