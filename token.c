#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <token.h>
#include <text.h>
#include <macro.h>

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
	if (c == EOF)
		type = _EOF;
	else
		type = tokenmap[c];
	mark_end();
	if (inquota)
		type = L;

	return type;
}

void back_token(void)
{
	if (inmacro)
		macropos--;
	else
		text_backchar();
}

#ifdef TOKEN_TEST

int main(int argc, char **argv)
{
	token_t type;

	/* init */
	if (argc == 2)
		text_open(argv[1]);

	parse_cheader();
	parse_macro();

	while ((type = get_token()) != _EOF) {
		switch (type) {
		case DOT:
			printf("[DOT]");
			break;
		case UPA:
			printf("[UPA]");
			break;
		case DOLLAR:
			printf("[DOLLAR]");
			break;
		case LSB:
			printf("[LSB]");
			break;
		case RSB:
			printf("[RSB]");
			break;
		case LP:
			printf("[LP]");
			break;
		case RP:
			printf("[RP]");
			break;
		case DASH:
			printf("[DASH]");
			break;
		case AST:
			printf("[AST]");
			break;
		case QST:
			printf("[?]");
			break;
		case ADD:
			printf("[+]");
			break;
		case OR:
			printf("[|]");
			break;
		case EOL:
			printf("[EOL]");
			break;
		case _EOF:
			printf("[EOF]");
			break;
		case L:
			printf("[terminal:%c]", *yytext);
			break;
		default:
			errexit("unknown token");
			break;
		}
		printf("\n");
	}
	return 0;
}

#endif	/* TOKEN_TEST */
