#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "token.h"

static char buf[128];
static int fd = 0, eof = 0;
static int pos = 0, size = 0, lineno = 1;

char *yytext = NULL;
int yyleng = 0;

void mark_start(void)
{
	yytext = &buf[pos];
	yyleng = 0;
}

void mark_end(void)
{
	yyleng = &buf[pos] - yytext;
}

void fillbuf(void)
{
	size = read(fd, buf, 128);
	if (size < 128)
		eof = 1;
	pos = 0;
}

char get_char(void)
{
	while (pos >= size) {
		if (eof)
			return EOF;
		fillbuf();
	}
	if (buf[pos] == '\n')
		lineno++;
	return buf[pos++];
}

token_t get_token(void)
{
	int c, escape = 0;
	token_t type;

	mark_start();
	while (1) {
		c = get_char();
		if (c != '\n')
			break;	
	}

	/* escape char */
	if (c == '\\') {
		escape = 1;
		c = get_char();
	}

	switch (c) {
	case '.':
		type = DOT;
		break;
	case '^':
		type = UPARROW;
		break;
	case '$':
		type = DOLLAR;
		break;
	case '[':
		type = LSB;
		break;
	case ']':
		type = RSB;
		break;
	case '(':
		type = LP;
		break;
	case ')':
		type = RP;
		break;
	case '-':
		type = DASH;
		break;
	case '*':
		type = ASTERISK;
		break;
	case '?':
		type = QUESTION;
		break;
	case '+':
		type = ADD;
		break;
	case '|':
		type = OR;
		break;
	case EOF:
		type = _EOF;
		break;
	default:
		type = TERMINAL;
		break;
	}
	mark_end();

	if (escape) {
		if (type == _EOF)
			errexit("string: \\EOF");
		else
			type = TERMINAL;
	}

	return type;
}

void fileopen(char *path)
{
	fd = open(path, O_RDONLY);
	if (fd == -1)
		errexit("open");

}
