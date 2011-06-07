#include <dfa.h>
#include <lib.h>
#include <set.h>

/* comp_table[com_rows][com_cols] */
static int *com_table;
static int com_rows;
static int com_cols;
/* row_map[nrows] */
static int *row_map;
static int nrows;
/* col_map[ncols] */
static int *col_map;
static int ncols;

void print_array(FILE *fp, int *array, int size, int col)
{
	int i;
	for (i = 0; i < size; i++) {
		fprintf(fp, "%4d,", array[i]);
		if (i % col == col - 1)
			fprintf(fp, "\n");
	}
	if (i % col)
		fprintf(fp, "\n");
}

void print_row_map(FILE *fp)
{
	fprintf(fp, "static int row_map[%d] = {\n", nrows);
	print_array(fp, row_map, nrows, 8);
	fprintf(fp, "};\n");
}

void print_col_map(FILE *fp)
{
	fprintf(fp, "static int col_map[%d] = {\n", ncols);
	print_array(fp, col_map, ncols, 8);
	fprintf(fp, "};\n");
}

void print_table(FILE *fp, int rowchars)
{
	int i;
	fprintf(fp, "static int com_table[%d][%d] = {\n", com_rows, com_cols);
	for (i = 0; i < com_rows * com_cols; i++) {
		fprintf(fp, "%5d,", com_table[i]);
		if (i % rowchars == rowchars - 1)
			fprintf(fp, "\n");
	}
	if (i % rowchars)
		fprintf(fp, "\n");
	fprintf(fp, "};\n");
}

void print_redundant_table(FILE *fp)
{
	fprintf(fp, "\n");
	print_row_map(fp);
	fprintf(fp, "\n");
	print_col_map(fp);
	fprintf(fp, "\n");
	print_table(fp, 8);
	fprintf(fp, "\n");
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

/*
 * clear implementation, but its performance is not good
 * alloc a new compressed table according to orignal table
 *
 * RETURN:
 * 	@ctable     compressed table
 * 	@cmap       column map
 * 	@rmap       row map
 * 	@crow       compressed table row
 * 	@ccol       compressed table column
 */
void redundant_compress(int (*table)[MAX_CHARS])
{
	struct set *col_set, *row_set;
	int row, col;
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
	printf("compress table: %d * %d\n", com_rows, com_cols);
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

void compress_dfatable(int (*table)[MAX_CHARS], int row, int col)
{
	nrows = row;
	ncols = col;
	redundant_compress(table);
}
