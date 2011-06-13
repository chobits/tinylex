#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <token.h>
#include <text.h>
#include <macro.h>
#include <lib.h>

char *yytext = NULL;
int yyleng = 0;

static unsigned int inmacro = 0;
static char *macrotext;
static char *macropos;

void mark_start(void)
{
	if (inmacro)
		yytext = macropos;
	else
		yytext = text_getpos();
	yyleng = 0;
}

void mark_end(void)
{
	if (inmacro)
		yyleng = macropos - yytext;
	else
		yyleng = text_getpos() - yytext;
}

int token_getchar(void)
{
	/* output expanded macro char */
	if (inmacro) {
		if (macropos < macrotext) {
			macropos = macrotext;
			return *macropos;
		}
		if (*macropos)
			return *macropos++;
		inmacro = 0;
		macropos = NULL;
		mark_start();	/* remark */
	}
	return text_getchar();
}

static int back_token_inescape = 0;
/* handle `\realchar` */
int token_getrealchar(void)
{
	int realchar, c;
	/* remark start position */
	mark_start();
	/* get real char */
	realchar = token_getchar();
	/* handle special char */
	switch (realchar) {
	case 'n':
		c = '\n';
		break;
	case 't':
		c = '\t';
		break;
	case '\\':
		c = '\\';
		break;
	defalut:
		/* we ignore escape characher for other char */
		c = realchar;
		break;
	}
	/* for back_token() */
	back_token_inescape = 1;
	/*
	 * Reset real char!
	 * Should we restore its original char?
	 * We will restore it in back_token().
	 */
	*yytext = c;
	return c;
}

/* expand {macro} */
void token_expand_macro(void)
{
	static char macro[128];
	int c, len;

	if (inmacro)
		text_errx("recursive macro expand");
	len = 0;
	while (len < 128) {
		c = text_getchar();
		/* macro end */
		if (c == '}')
			break;
		if (!ismacrochar(c))
			text_errx("invalid macro char");
		macro[len] = c;
		len++;
	}
	if (len >= 128)
		text_errx("macro name is too long");
	macro[len] = '\0';
	macrotext = expand_macro(macro);
	if (!macrotext)
		text_errx("macro is not existed");
	inmacro = 1;
	macropos = macrotext;
}

/* map char to token */
static const int tokenmap[128] = {
/* 0    '\0'    SOH     STX     ETX     ENQ     ACK     '\a'    '\b' */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 8    '\b'    '\t'    '\n'    '\v'    '\f'    '\r'    SO      SI   */
	L,	SPACE,	EOL,	SPACE,	L,	L,	L,	L,
/* 16   DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB  */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 24   CAN     EM      SUB     ESC     FS      GS      RS      US   */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 32   ' '     !       "       #       $       %       &       '    */
	SPACE,	L,	L,	L,	DOLLAR,	L,	L,	L,
/* 40   (       )       *       +               -       .       ´    */
	LP,	RP,	AST,	ADD,	L,	DASH,	DOT,	L,
/* 48   0       1       2       3       4       5       6       7    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 56   8       9       :       ;       <       =       >       ?    */
	L,	L,	L,	L,	L,	L,	L,	QST,
/* 64   @       A       B       C       D       E       F       G    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 72   H       I       J       K       L       M       N       O    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 80   P       Q       R       S       T       U       V       W    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 88   X       Y       Z       [       \       ]       ^       _    */
	L,	L,	L,	LSB,	L,	RSB,	UPA,	L,
/* 96   `       a       b       c       d       e       f       g    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 104  h       i       j       k       l       m       n       o    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 112  p       q       r       s       t       u       v       w    */
	L,	L,	L,	L,	L,	L,	L,	L,
/* 120  x       y       z       {       |       }       ~       DEL  */
	L,	L,	L,	LCP,	OR,	RCP,	L,	L,
};

token_t get_token(void)
{
	static unsigned int inquota = 0;	/* "in quotation" */
	int c;
	token_t type;
restart:
	mark_start();

	c = token_getchar();
	/* handle "quato" */
	if (c == '"') {
		inquota = ~inquota;
		goto restart;
	}

	/* handle {macro} */
	if (c == '{' && !inquota) {
		token_expand_macro();
		goto restart;
	}

	type = L;
	/*
	 * if we back token(escape char) last,
	 * we should take care here!
	 */
	if (back_token_inescape == 1) {
		/* token_getrealchar -> (now)get_token */
		back_token_inescape = 0;
	} else if (back_token_inescape == 2) {
		/* token_getrealchar -> back_token -> (now)get_token */
		back_token_inescape = 1;
		goto end;
	}

	if (!inquota) {
		/* hansle escape character \ */
		if (c == '\\')
			c = token_getrealchar();
		else
			type = (c == EOF) ? _EOF : tokenmap[c];
	}
end:
	mark_end();
	return type;
}

void back_token(void)
{
	if (inmacro)
		macropos--;
	else
		text_backchar();
	/* backed token is escape char, e.g `\n` */
	if (back_token_inescape == 1)
		back_token_inescape = 2;
}
