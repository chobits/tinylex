MAKEFLAGS += --no-print-directory

OBJS	= tinylex nfa egrep_like dfatable set mindfa text nfa tinylex_debug
CC	= gcc
CFLAGS	= -Iinclude

all:tinylex egrep_like

GREPOBJS	=	\
	dfa.c		\
	interpret.c	\
	macro.c		\
	mindfa.c	\
	nfa.c		\
	lib.c		\
	token.c		\
	script.c	\
	set.c		\
	text.c		\
	compressdfa.c	\
	gencode.c

TLOBJS	= $(GREPOBJS) main.c

#
#final program: tinylex
#
tinylex:$(TLOBJS)
	$(CC) $(CFLAGS) $^ -o $@

egrep_like:$(GREPOBJS)
	$(CC) $(CFLAGS) -DINTERPRET $^ -o $@


#
# Debug program
# Usage: set|text|nfa|dfatable|mindfa test/test_common.l
#
debug:tinylex_debug set text nfa dfatable mindfa
tinylex_debug:$(TLOBJS)
	$(CC) $(CFLAGS) -DDEBUG -DDEBUG_MIN_TABLE $^ -o $@
	@echo "Run command: ./$@ test/colour.l -o colour.c"
	@echo "            gcc colour.c -o colour"
	@echo "            ./colour lib.c"
	@echo
mindfa:mindfa.c dfa.c interpret.c nfa.c token.c lib.c set.c text.c macro.c
	$(CC) $(CFLAGS) -DDEBUG -DDEBUG_MIN_TABLE -DMIN_DFA_TEST $^ -o $@
	@echo "Run command: ./$@ test/common_test.l"
	@echo
dfatable:dfa.c interpret.c nfa.c token.c lib.c set.c text.c macro.c
	$(CC) $(CFLAGS) -DDEBUG -DDFA_TABLE_TEST $^ -o $@
	@echo "Run command: ./$@ test/common_test.l"
	@echo
nfa:nfa.c token.c lib.c set.c text.c macro.c
	$(CC) $(CFLAGS) -DDEBUG -DNFATEST $^ -o $@
	@echo "Run command: ./$@ test/common_test.l"
	@echo
set:set.c lib.c
	$(CC) $(CFLAGS) -DSET_TEST $^ -o $@
	@echo "Run command: ./$@"
	@echo
text:text.c lib.c
	$(CC) $(CFLAGS) -DLEX_TEXT_TEST $^ -o $@
	@echo "Run command: ./$@ file"
	@echo

#
# Test script: test tinylex
#
test:tltest egtest
tltest:tinylex
	@(cd test; make)
	@echo "Run command: test/colour lib.c"
	@echo
egtest:egrep_like
	@echo "Run command: ./egrep_like test/comment.l lex.yy.part.c"
	@echo


clean:
	rm -rf $(OBJS)
	@(cd test; make clean)

#
# Calculate code lines
#
lines:
	@echo "[======= Code Lines ========]"
	@wc -l *.c include/*.h
