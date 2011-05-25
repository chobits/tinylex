all:nfa egrep_like dfatable set
dfatable:dfa.c interpret.c nfa.c regtoken.c reglib.c set.c
	gcc -DDEBUG -DDFA_TABLE_TEST $^ -o $@
egrep_like:interpret.c nfa.c regtoken.c reglib.c set.c
	gcc -DINTERPRET $^ -o $@
nfa:nfa.c regtoken.c reglib.c set.c
	gcc -DDEBUG -DNFATEST $^ -o $@
set:set.c reglib.c
	gcc -DSET_TEST $^ -o $@
clean:
	rm -rf nfa egrep_like dfatable set
