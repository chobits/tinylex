OBJS	= tinylex nfa egrep_like dfatable set mindfa text nfa token

all:tinylex
mindfa:mindfa.c dfa.c interpret.c nfa.c regtoken.c reglib.c set.c text.c macro.c
	gcc -DDEBUG -DDEBUG_MIN_TABLE -DMIN_DFA_TEST $^ -o $@
dfatable:dfa.c interpret.c nfa.c regtoken.c reglib.c set.c text.c macro.c
	gcc -DDEBUG -DDFA_TABLE_TEST $^ -o $@
nfa:nfa.c regtoken.c reglib.c set.c text.c macro.c
	gcc -DDEBUG -DNFATEST $^ -o $@
set:set.c reglib.c
	gcc -DSET_TEST $^ -o $@
text:text.c reglib.c
	gcc -DLEX_TEXT_TEST $^ -o $@

GREPOBJS	=	\
	dfa.c		\
	interpret.c	\
	macro.c		\
	mindfa.c	\
	nfa.c		\
	reglib.c	\
	regtoken.c	\
	script.c	\
	set.c		\
	text.c

TLOBJS	= $(GREPOBJS) main.c

tinylex:$(TLOBJS)
	gcc -DDEBUG -DDEBUG_MIN_TABLE $^ -o $@

token:$(GREPOBJS)
	gcc -DDEBUG -DTOKEN_TEST $^ -o $@

egrep_like:$(GREPOBJS)
	gcc -DINTERPRET $^ -o $@

clean:
	rm -rf $(OBJS)
lines:
	@echo "[======= Code Lines ========]"
	@wc -l *.c *.h
