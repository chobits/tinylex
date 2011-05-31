#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <lib.h>
#include <reg.h>

/*
 * tiny text file stream
 */
#define TEXT_BUFSIZE 4096
#define TEXT_BUF_END (&text_buf[TEXT_BUFSIZE])
#define TEXT_BUF_START text_buf

#define TEXT_LINE 0
#define TEXT_CHAR 1

/* text file */
static int text_fd = -1;
static int text_eof;

/* text buffer */
#define TEXT_REAL_BUFSIZE (TEXT_BUFSIZE + 1) /* add '\0' at tail */
static char text_buf[TEXT_REAL_BUFSIZE];
static char *text_pos;
static char *text_end;
static char *text_prev_pos;		/* prev pos for error output */
static int text_prev_op;		/* prev operation: getchar or getline */
/* line */
static int text_lineno;
/* backup line */
static char *text_prev_linetail;
static int text_prev_lineno = 0;
/* backup terminal */
#define TEXT_TERMINAL '\0'
static char text_term_char;
static char *text_term_pos;

void text_err(char *str)
{
	char *p;
	fprintf(stderr, "Syntax error:%s.", str);
	fprintf(stderr, " At line %d:", text_lineno);
	if (text_prev_op == TEXT_CHAR) {
		fputc(text_prev_pos ? *text_prev_pos : ' ', stderr);
	} else if (text_prev_op == TEXT_LINE && text_prev_pos) {
		for (p = text_prev_pos;
			p < TEXT_BUF_END && *p && *p != '\n';
			p++)
			fputc(*p, stderr);
	}
	fputc('\n', stderr);
}

void text_errx(char *str)
{
	text_err(str);
	exit(EXIT_FAILURE);
}

/* get current text position */
char *text_getpos(void)
{
	return text_pos;
}

/* mark terminal at @pos */
void text_term(char *pos)
{
	text_term_pos = pos;
	text_term_char = *pos;
	*pos = TEXT_TERMINAL;
}

/* restore pos */
void text_unterm(void)
{
	if (text_term_pos) {
		*text_term_pos = text_term_char;
		text_term_char = TEXT_TERMINAL;
		text_term_pos = NULL;
	}
}

/* default save for error output */
void text_save_pos(int op)
{
	text_prev_pos = text_pos;
	text_prev_op = op;
}

static void text_save(void)
{
	char *p;
	*TEXT_BUF_START = text_pos[-1];		/* save prev char */

	p = TEXT_BUF_START + 1;
	while (text_pos < text_end)
		*p++ = *text_pos++;
	text_pos = TEXT_BUF_START + 1;
	text_end = p;				/* text_end[0] is invalid */
}

static void text_fill_buf(void)
{
	int n;
	if (text_eof)
		return;
	text_save();
	n = read(text_fd, text_end, TEXT_BUF_END - text_end);
	if (n < 0) {
		errexit("text_fill_buf read error");
	} else if (n == 0) {
		text_eof = 1;
		/* safe for text_getline */
		*text_end = EOF;
	} else {
		text_end += n;
	}

}

char text_getchar(void)
{
	text_unterm();

	if (text_pos >= text_end)
		text_fill_buf();
	if (text_eof && text_pos >= text_end)
		return EOF;
	
	text_save_pos(TEXT_CHAR);
	return *text_pos++;
}

char *text_lookahead(int len)
{
	if (len < 1)
		return NULL;
	text_unterm();

	if (text_pos + len - 1 >= text_end)
		text_fill_buf();
	if (text_eof && (text_pos + len - 1 >= text_end))
		return NULL;

	return text_pos;
}

/* It can only run once after text_getchar()! */
void text_backchar(void)
{
	/* 
	 * No call text_unterm() here ,
	 * it will be called in text_get*().
	 */
	text_pos--;
}

int text_lookback(void)
{
	return text_pos[-1];
}

int text_prevline(char **linp)
{
	int len = 0;
	if (text_prev_linetail) {
		/* Can backed line be usable still? */
		if (text_prev_lineno == text_lineno + 1) {
			/* default save for error output */
			text_save_pos(TEXT_LINE);
			/* get prev line */
			text_pos = text_prev_linetail;
			text_lineno++;
			text_term(text_pos);
			len = text_prev_linetail - text_pos;
		}
		text_prev_linetail = NULL;
	}
	return len;
}

int text_getline(char **line)
{
	int len = 0;

	text_unterm();
	if (len = text_prevline(line))
		return len;

	/* when EOF, it is safe, because text_end[0] == EOF */
	while (text_pos[len] != '\n') {
		if (&text_pos[len] >= text_end)
			text_fill_buf();
		if (text_eof && &text_pos[len] >= text_end) {
			if (text_pos >= text_end)
				return 0;
			/* for line which has no tailing '\n' */
			len--;
			break;
		}
		len++;
	}

	if (text_lookback() == '\n')
		text_lineno++;

	if (line)
		*line = text_pos;

	text_save_pos(TEXT_LINE);
	text_pos += len + 1;	/* 1 for skipping '\n' */
	text_term(text_pos);

	return len + 1;		/* contain '\n' char */
}

/*
 * line buffer:
 * |<....>|\n|?|
 *  |         `---> lex_line_end
 *  `-------------> line
 */
/* not check line */
void text_backline(char *line)
{
	text_unterm();

	text_prev_linetail = text_pos;
	text_pos = line;
	text_prev_lineno = text_lineno;
	text_lineno--;
}

void text_open(char *file)
{
	/* open file */
	if (!file)
		errexit("open NULL file");
	text_fd = open(file, O_RDONLY);
	if (text_fd == -1)
		errexit("cannot open file");
	/* buf */
	text_pos = TEXT_BUF_START + 1;	/* skip prev line tail */
	text_end = TEXT_BUF_START + 1;
	*text_pos = TEXT_TERMINAL;	/* for text_getline */
	text_prev_pos = NULL;
	/* terminal */
	text_term_pos = NULL;
	text_term_char = TEXT_TERMINAL;
	/* line */
	*TEXT_BUF_START = '\n';		/* dummy prev line tail */
	*TEXT_BUF_END = TEXT_TERMINAL;
	text_lineno = 0;
}

int skip_whitespace(void)
{
        int c;
        /* skip space char and space line */
        do {
                c = text_getchar();
        } while (isspace(c));
        if (c != EOF)
        	text_backchar();
	return c;
}

#ifdef LEX_TEXT_TEST

int main(int argc, char **argv)
{
	char *line;
	int len;
	int c, k;
	if (argc != 2) {
		dbg("Usage: %s file", argv[0]);
		exit(EXIT_FAILURE);
	}

	text_open(argv[1]);
/*
	while ((c = text_getchar()) != EOF) {
		putchar(c);
		len = text_getline(&line);
		if (!len)
			break;
		printf("%s", line);
	}
 */
/*
	while ((c = text_getchar()) != EOF) {
		putchar(c);
	}
*/
/*
	while (len = text_getline(&line)) {
		printf("%s", line);
	}
 */
/* double line 
	k = 0;
	while (len = text_getline(&line)) {
		printf(green(%-4d)red(:)"%s", text_lineno, line);
		if (++k & 1)
			text_backline(line);
	}
 */
	while (len = text_getline(&line))
		printf(green(%-4d)red(:)"%s", text_lineno, line);
	return 0;
}

#endif	/* LEX_TEXT_TEST */
