OBJS	= tinylex nfa egrep_like dfatable set mindfa text nfa token
CC	= gcc
CLFAGS	= -Iinclude

all:tinylex set text
mindfa:mindfa.c dfa.c interpret.c nfa.c token.c lib.c set.c text.c macro.c
	$(CC) $(CLFAGS) -DDEBUG -DDEBUG_MIN_TABLE -DMIN_DFA_TEST $^ -o $@
dfatable:dfa.c interpret.c nfa.c token.c lib.c set.c text.c macro.c
	$(CC) $(CLFAGS) -DDEBUG -DDFA_TABLE_TEST $^ -o $@
nfa:nfa.c token.c lib.c set.c text.c macro.c
	$(CC) $(CLFAGS) -DDEBUG -DNFATEST $^ -o $@
set:set.c lib.c
	$(CC) $(CLFAGS) -DSET_TEST $^ -o $@
text:text.c lib.c
	$(CC) $(CLFAGS) -DLEX_TEXT_TEST $^ -o $@

GREPOBJS	=	\
	dfa.c		\
	interpret.c	\
	macro.c		\
	mindfa.c	\
	nfa.c		\
	lib.c	\
	token.c	\
	script.c	\
	set.c		\
	text.c

TLOBJS	= $(GREPOBJS) main.c compressdfa.c

tinylex:$(TLOBJS)
	$(CC) $(CLFAGS) -DDEBUG -DDEBUG_MIN_TABLE $^ -o $@

token:$(GREPOBJS)
	$(CC) $(CLFAGS) -DDEBUG -DTOKEN_TEST $^ -o $@

egrep_like:$(GREPOBJS)
	$(CC) $(CLFAGS) -DINTERPRET $^ -o $@

clean:
	rm -rf $(OBJS)
lines:
	@echo "[======= Code Lines ========]"
	@wc -l *.c include/*.h
