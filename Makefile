all:nfa egrep_like
egrep_like:interpret.c nfa.c regtoken.c reglib.c set.c
	gcc -DINTERPRET $^ -o $@
nfa:nfa.c regtoken.c reglib.c set.c
	gcc -DDEBUG -DNFATEST $^ -o $@
clean:
	rm -rf reg match nfa egrep_like
