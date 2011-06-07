#include <stdio.h>
#include <unistd.h>
#include <lib.h>

static char *script = NULL;
static char *lexpart = NULL;
static char *codefile = NULL;

void version(void)
{
	fprintf(stderr, "tinylex (TINYLEX) 0.0.0 20110607\n\n");
	exit(0);
}

void usage(void)
{
	fprintf(stderr,
		"USAGE: tinylex script [OPTIONS]\n"
		"OPTIONS:\n"
		"      -o file         output code file\n"
		"      -p file         use file instead default lex.yy.part.c\n"
		"      -h              help information\n"
		"      -v              vertion information\n"
		"e.g:\n"
		"  tinylex test/colour.l -o colour.c\n"
		"\n"
		);
	exit(EXIT_FAILURE);
}

void parse_args(int argc, char **argv)
{
	int i;
	char opt;
	if (argc < 2)
		usage();

	opterr = 0;
	while ((opt = getopt(argc, argv, "o:p:hv")) != -1) {
		switch (opt) {
		case 'o':
			if (codefile)
				usage();
			codefile = optarg;
			break;
		case 'h':
			usage();
			break;
		case 'v':
			version();
			break;
		case 'p':
			lexpart = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	if (optind >= argc)
		errexit("no script file");

	script = argv[optind];
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);
	open_script(script);
	code_open(codefile, lexpart);
	parse_script();
	return 0;
}
