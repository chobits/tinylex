#ifndef __TOKEN_H
#define __TOKEN_H

typedef enum tokens {
	NIL,		/* null */
	DOT,		/* `.`  */
	OR,		/* `|`  */
	ASTERISK,	/* `*`  */
	ADD,		/* `+`  */
	QUESTION,	/* `?`  */
	LP,		/* `(`  */
	RP,		/* `)`  */
	LSB,		/* `[`  */
	RSB,		/* `]`  */
	DASH,		/* `-`  */
	UPARROW,	/* `^`  */
	DOLLAR,		/* `$`  */
	CC,		/* not real token: concatenation */
	TERMINAL,	/* terminal symbol */
	_EOF		/* end of file */
} token_t;

extern char *yytext;
extern int yyleng;

#endif	/* token.h */
