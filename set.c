#include "lib.h"
#include "set.h"
#include <string.h>

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
	if (orignal->map == orignal->defmap)
		new->map = new->defmap;
	return new;
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
