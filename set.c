#include <string.h>
#include "lib.h"
#include "set.h"

void complset(struct set *set)
{
	set->compl = ~(set->compl);
}

/* alloc new set using default map */
struct set *newset(void)
{
	struct set *set;
	set = xmalloc(sizeof(*set));
	set->nbits = DEF_MAPBITS;
	set->ncells = DEF_MAPCELLS;
	set->used = 0;
	set->compl = 0;
	/* init default map */
	set->map = set->defmap;
	memset(set->map, 0x0, set->ncells);
	return set;
}

/*
 * alloc new set
 * alloc new expanded map when nbits > DEF_MAPBITS
 */
struct set *allocset(int nbits)
{
	struct set *set;
	set = newset();
	/* reset map */
	if (CELLS_UP(nbits) > CELLS_UP(set->nbits)) {
		set->nbits = nbits;
		set->ncells = CELLS_UP(nbits);
		set->map = xmalloc(set->ncells);
		memset(set->map, 0x0, set->ncells);
	}
	return set;
}

void delset(struct set *set)
{
	if (!set)
		return;
	/* free expanded map */
	if (set->map && set->map != set->defmap)
		free(set->map);
	free(set);
}

/* duplicate @orignal set */
struct set *dupset(struct set *orignal)
{
	struct set *new;
	new = xmalloc(sizeof(*new));
	memcpy(new, orignal, sizeof(*new));
	/* fix default map pointer */
	if (orignal->map == orignal->defmap) {
		new->map = new->defmap;
	} else {
		new->map = xmalloc(orignal->ncells);
		memcpy(new->map, orignal->map, orignal->ncells);
	}
	return new;
}

void copyset(struct set *old, struct set *new)
{
	if (!old || !new)
		return;
	memcpy(new, old, sizeof(*old));
	/* fix default map pointer */
	if (old->map == old->defmap) {
		new->map = new->defmap;
	} else {
		new->map = xmalloc(old->ncells);
		memcpy(new->map, old->map, old->ncells);
	}

}

/*
 * expand set map to CELLS_UP(entry) cells
 */
void expandset(struct set *set, int entry)
{
	int n = CELLS_UP(entry);

	set_cell_t *bak = set->map;

	set->map = xmalloc(n);
	/* backup original map */
	memcpy(set->map, bak, set->ncells);
	/* clear expanded map */
	memset(set->map + set->ncells, 0x0, n - set->ncells);
	set->ncells = n;
	set->nbits = entry;
	/* free original expanded map */
	if (bak != set->defmap)
		free(bak);
}

int equset(struct set *s1, struct set *s2)
{
	int i;
	unsigned int k;
	if (s1->nbits != s2->nbits)
		return 0;
	for (i = 0; i < s1->ncells; i++) {
		/* complemented */
		if (s1->compl ^ s2->compl) {
			if (s1->map[i] & s2->map[i])
				return 0;
		} else {
			if (s1->map[i] != s2->map[i])
				return 0;
		}
	}
	return 1;
}

int memberofset(int member, struct set *set)
{
	if (member < 0)
		return 0;
	if (set->map[CELLS(member)] & (1 << CELL_BIT(member)))
		return !(set->compl);
	return !!(set->compl);
}

/*
 * add new entry into set
 * expand set map when entry overflows original map
 */
void addset(struct set *set, int entry)
{
	if (CELLS_UP(entry) > CELLS_UP(set->nbits))
		expandset(set, entry);
	if (!(set->map[CELLS(entry)] & (1 << CELL_BIT(entry)))) {
		if(!set->compl) {
			set->used++;
			set->map[CELLS(entry)] |= (1 << CELL_BIT(entry));
		}
	} else if (set->compl) {
		set->used--;
		set->map[CELLS(entry)] &= ~(1 << CELL_BIT(entry));
	}
}

/*
 * test whether @entry is a member of @set before, and add entry to @set
 * this function = memberofset() + addset()
 * return 1 for @entry is a member of @set before, otherwise return 0
 */
int test_add_set(struct set *set, int entry)
{
	int oldused;
	if (!set)
		return -1;

	if (CELLS_UP(entry) > CELLS_UP(set->nbits))
		expandset(set, entry);

	oldused = set->used;

	if (!(set->map[CELLS(entry)] & (1 << CELL_BIT(entry)))) {
		if(!set->compl) {
			set->used++;
			set->map[CELLS(entry)] |= (1 << CELL_BIT(entry));
		}
	} else if (set->compl) {
		set->used--;
		set->map[CELLS(entry)] &= ~(1 << CELL_BIT(entry));
	}

	return !(set->used ^ oldused);
}

/*
 * temp set stream:
 *    each calling will get next member in @current set stream
 * when @set != @current, reset @current set
 *      @set is NULL, clear @current set stream
 */
int nextmember(struct set *set)
{
	static struct set *current = NULL;
	static int member = -1;
	/* clear original set stream */
	if (!set) {
		current = NULL;
		member = -1;
		return 0;
	}

	/* reset set stream, and get the first member in @set */
	if (set != current) {
		current = set;
		/* fast find first member cell */
		for (member = 0; member < current->ncells; member += 8)
			if (current->map[CELLS(member)])
				break;
	}

	while (member < current->nbits) {
		if (memberofset(member, current))
			return member++;
		member++;
	}

	return -1;
}

void outputmap(struct set *set)
{
	int i;
	for (i = 0; i < set->ncells; i++)
		printf("%02x ", set->map[i]);
	printf("\n");
}

#ifdef SET_TEST

void equset_test(void)
{
	struct set *s1, *s2;
	dbg("equset test:");

	s1 = newset();
	s2 = newset();

	addset(s1, 0);
	addset(s2, 0);
	if (!equset(s1, s2))
		errexit("set test 1 error");

	complset(s2);
	if (s2->compl != 0xffffffff)
		errexit("set test 1.5 error");

	if (equset(s1, s2))
		errexit("set test 2 error");

	complset(s1);
	if (!equset(s1, s2))
		errexit("set test 3 error");

	complset(s1);
	complset(s2);
	addset(s1, 111);
	if (equset(s1, s2))
		errexit("set test 4 error");

	complset(s1);
	if (equset(s1, s2))
		errexit("set test 5 error");

	delset(s1);
	delset(s2);

	dbg("ok!");
}

int main(void)
{
	equset_test();
	return 0;
}

#endif
