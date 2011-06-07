LEX	= ../tinylex
CC	= gcc

all:colour
colour:colour.c
	$(CC) $< -o $@
colour.c:colour.l
	../tinylex -p ../lex.yy.part.c colour.l -o colour.c
clean:
	rm -rf colour.c colour
