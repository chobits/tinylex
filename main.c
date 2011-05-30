#include <stdio.h>
#include <lib.h>

int main(int argc, char **argv)
{
	if (argc != 2)
		errexit("ARGC != 2");

	open_script(argv[1]);
	parse_script();
	return 0;
}
