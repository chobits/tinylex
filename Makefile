OBJS	= nfa egrep_like dfatable set mindfa

all:$(OBJS)
mindfa:mindfa.c dfa.c interpret.c nfa.c regtoken.c reglib.c set.c
	gcc -DDEBUG -DDEBUG_MIN_TABLE -DMIN_DFA_TEST $^ -o $@
dfatable:dfa.c interpret.c nfa.c regtoken.c reglib.c set.c
	gcc -DDEBUG -DDFA_TABLE_TEST $^ -o $@
egrep_like:interpret.c nfa.c regtoken.c reglib.c set.c
	gcc -DINTERPRET $^ -o $@
nfa:nfa.c regtoken.c reglib.c set.c
	gcc -DDEBUG -DNFATEST $^ -o $@
set:set.c reglib.c
	gcc -DSET_TEST $^ -o $@
clean:
	rm -rf $(OBJS)
