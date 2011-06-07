%{

/* colour macro */
#define red(str) "\e[01;31m"#str"\e\[0m"
#define green(str) "\e[01;32m"#str"\e\[0m"
#define yellow(str) "\e[01;33m"#str"\e\[0m"
#define navyblue(str) "\e[01;34m"#str"\e\[0m"
#define purple(str) "\e[01;35m"#str"\e\[0m"
#define cambrigeblue(str) "\e[01;36m"#str"\e\[0m"
#define grey(str) "\e[01;30m"#str"\e\[0m"
#include <fcntl.h>


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
