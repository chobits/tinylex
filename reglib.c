#include "lib.h"

void errexit(char *str)
{
	if (errno)
		perror(str);
	else
		fprintf(stderr, "ERROR: %s\n", str);
	exit(EXIT_FAILURE);
}

void *xmalloc(size_t size)
{
	void *p = malloc(size);
	if (!p)
		errexit("malloc");
	return p;
}

/* detect whether the line consists of all white space */
int isspaceline(char *line)
{
	while (*line && isspace(*line))
		line++;
	return (*line) ? 0 : 1;
}
