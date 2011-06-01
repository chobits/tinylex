#ifndef __TOKEN_H
#define __TOKEN_H

typedef enum tokens {
	NIL,		/* null */
	DOT,		/* `.`  */
	OR,		/* `|`  */
	AST,		/* `*`  asterisk */
	ADD,		/* `+`  */
	QST,		/* `?`  question mark */
	LP,		/* `(`  open parenthesis */
	RP,		/* `)`  close parenthesis */
	LSB,		/* `[`  open square brakcet */
	RSB,		/* `]`  close square bracket */
	LCP,		/* `{`  open curly bracket */
	RCP,		/* `}`  close curly bracket */
	DASH,		/* `-`  */
	UPA,		/* `^`  up arrow anchor */
	DOLLAR,		/* `$`  */
	L,		/* lexeme */
	EOL,		/* end of line: \n */
	SPACE,		/* space tab */
	_EOF		/* end of file */
} token_t;

extern char *yytext;
extern int yyleng;

#endif	/* token.h */
