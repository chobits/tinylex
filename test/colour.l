%{

#include <fcntl.h>
#include "../include/reg.h"	/* for colour macro */

%}

digit [0-9]

%%

"case"|"default"|"switch"|"typeof"|"sizeof"|"return"|"if"|"else"|"while"|"break"|"continue"|"do"|"until" printf(yellow(%s), yytext);
"FILE"|"char"|"static"|"void"|"int"|"short"|"unsigned"|"float"|"double"|"struct" printf(green(%s), yytext);
{digit}+		printf(red(%s), yytext);	/* digit */
"/*"[^*]*[^/]*"*/"	printf(navyblue(%s), yytext);	/* comment */
"#".*			printf(purple(%s), yytext);	/* macro */

%%

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	yyfd = open(argv[1], O_RDONLY);
	yylex();
	return 0;
}
