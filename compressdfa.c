#include <dfa.h>
#include <lib.h>
#include <set.h>

int equal_col(int *table, int nrows, int ncols, int col1, int col2)
{
	int i;
	for (i = 0; i < nrows; i++)
		if (table[col1 + ncols * i] != table[col2 + ncols * i])
			return 0;
	return 1;
}

int equal_row(int *table, int nrows, int ncols, int row1, int row2)
{
	int i;
	for (i = 0; i < ncols; i++)
		if (table[row1 * ncols + i] != table[row2 * ncols + i])
			return 0;
	return 1;
}

void redundant_compress_debug(int *origtbl, int origrows, int origcols,
				int *table, int rows, int cols,
				int *row_map, int *col_map)
{
	int i, k;

	/* compressed table */
	printf("redundant compressed table:\n");
	for (i = 0; i < rows; i++) {
		for (k = 0; k < cols; k++)
			printf("%-5d", table[i * cols + k]);
		printf("\n");
	}
	/* col map*/
	printf("col_map:\n");
	for (i = 0; i < origcols; i++)
		printf("%d ", col_map[i]);
	printf("\n");
	/* row map*/
	printf("row_map:\n");
	for (i = 0; i < origrows; i++)
		printf("%d ", row_map[i]);
	printf("\n");

	/* check validity */
	for (i = 0; i < origrows; i++)
		for (k = 0; k < origcols; k++)
			if (origtbl[i * origcols + k] !=
				table[row_map[i] * cols + col_map[k]])
				errexit("redundant compressed table corrupts");

}

/*
 * clear implementation, but its performance is not good
 * alloc a new compressed table according to orignal table
 */
void redundant_compress(int (*table)[MAX_CHARS], int nrows, int ncols)
{
	struct set *col_set, *row_set;
	int *col_map;
	int *row_map;
	int *com_table;
	int com_cols, com_rows, row, col;
	int i, k;

	/* compress columns */
	col_map = xmalloc(sizeof(int) * ncols);
	col_set = newset();
	for (i = com_cols = 0; i < ncols; i++) {
		for (k = 0; k < i; k++) {
			if (equal_col((int *)table, nrows, ncols, k, i)) {
				col_map[i] = col_map[k];
				break;
			}
		}
		/* not redundant column */
		if (k >= i) {
			col_map[i] = com_cols++;
			addset(col_set, i);
		}
	}

	/* 
	 * compress rows
	 * the algorithm is the same as columns compression
	 */
	row_map = xmalloc(sizeof(int) * nrows);
	row_set = newset();
	for (i = com_rows = 0; i < nrows; i++) {
		for (k = 0; k < i; k++) {
			/* some redundant column entry is still compared */
			if (equal_row((int *)table, nrows, ncols, k, i)) {
				row_map[i] = row_map[k];	
				break;
			}
		}
		if (k >= i) {
			row_map[i] = com_rows++;
			addset(row_set, i);
		}
	}

	/* build compressed table */
	com_table = xmalloc(com_rows * com_cols * sizeof(int));
	printf(" %d * %d\n", com_rows, com_cols);
	col = 0;
	for_each_member(i, col_set) {
		row = 0;
		for_each_member(k, row_set) {
			/* row = row_map[k]  col = col_map[i] */
			com_table[row * com_cols + col] = table[k][i];
			row++;
		}
		col++;
	}
	freeset(col_set);
	freeset(row_set);

	/* check rows and cols */
	if ((row != com_rows) && (col != com_cols))
		errexit("error rows or cols of redundant compressed table");
	
	printf("\n");
	redundant_compress_debug((int *)table, nrows, ncols,
				com_table, com_rows, com_cols, row_map, col_map);
}

void compress_dfatable(int (*table)[MAX_CHARS], int rows, int cols)
{
	redundant_compress(table, rows, cols);
}
