/* bit map implemented set type */

#ifndef __SET_H
#define __SET_H

/*
 * set operation
 */
#define CELLS(bits)		((bits) >> 3)
#define CELLS_UP(bits)		(((bits) + 7) >> 3)
#define CELL_BIT(bits)		((bits) & 7)
#define BIT_CELL(cell, bit)	(((cell) << 3) + (bit))

/*
 * type definition
 */
/* 256 for basically supporting ASCII set */
#define DEF_MAPBITS		128
#define DEF_MAPCELLS		CELLS_UP(DEF_MAPBITS)

#define set_cell_t		unsigned char

struct set {
	unsigned int compl;
	unsigned int used;			/* used bits */
	unsigned int nbits;
	unsigned int ncells;
	set_cell_t *map;			/* real map pointer */
	set_cell_t defmap[DEF_MAPCELLS];	/* default map cells */
};

/* extern function */

extern struct set *newset(void);
extern struct set *allocset(int nbits);
extern void expandset(struct set *set, int entry);
extern void addset(struct set *set, int entry);
extern struct set *dupset(struct set *orignal);
extern int memberofset(int member, struct set *set);
extern void freeset(struct set *set);
extern void delset(struct set *set, int entry);
extern void complset(struct set *set);
extern int emptyset(struct set *set);

#endif	/* set.h */
