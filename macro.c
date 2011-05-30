#include <string.h>
#include <lib.h>
#include <list.h>
#include <macro.h>

static struct list_head macrotable[MACRO_HASH_SIZE];

/* compute macro name hash value */
unsigned int mhash(char *name)
{
	unsigned int hash, c;

	hash = 0;
	while (c = *name++)
		hash = (hash + (c << 4 + c >> 4)) * 11;

	return hash;
}

/* look up macro entry in macro table according to @name, @hash */
struct macro *lookup_hash_macro(char *name, unsigned int hash)
{
	struct macro *macro;
	list_for_each_entry(macro, tbl(hash), list) {
		/* find */
		if (strcmp(macro->name, name) == 0)
			return macro;
	}
	return NULL;
}

/* look up macro entry in macro table according to @name */
struct macro *lookup_macro(char *name)
{
	unsigned int hash;
	hash = mhash(name);
	return lookup_hash_macro(name, hash);
}

struct macro *new_macro(char *name, char *text)
{
	struct macro *macro;
	macro = xmalloc(sizeof(*macro));
	macro->name = strdup(name);
	macro->text = strdup(text);
	macro->hash = mhash(name);
	list_init(&macro->list);
	return macro;
}

struct macro *make_macro(char *line, int len)
{
	struct macro *macro;
	char macroname[128];
	char macrotext[128];
	int i, k, inquato = 0;
	unsigned int hash;

	if (len > 256)
		text_errx("Format macro is invalid: macro is large");
	/* elimite tail newline */
	if (line[len - 1] == '\n') {
		line[len - 1] = '\0';
		len--;
	}

	/* get macro name */
	for (k = i = 0; i < len; i++, k++) {
		if (isspace(line[i]))
			break;
		macroname[k] = line[i];
	}
	macroname[k] = '\0';

	/* skip whitespace: space or tab */
	while (isspace(line[i]))	/* isspace('\0') -> 0 */
		i++;
	if (i >= len)
		text_errx("no macro text");

	/* get macro text */
	for (k = 0, inquato = 0; i < len; i++, k++) {
		/* "..<blank>.." is valid */
		if (isblank(line[i]) && !inquato)
			break;
		/* text reserve quota `"`. */
		if (line[i] == '"')
			inquato = ~inquato;
		macrotext[k] = line[i];
	}
	macrotext[k] = '\0';
	if (inquato || !isspaceline(&line[i]))
		text_errx("Format macro is invalid");

	/* check whether this macro is already in macor table */
	macro = lookup_macro(macroname);
	if (macro) {
		text_err("reduplicated macro");
		/* using new macro text instead of old one */
		free(macro->text);
		macro->text = strdup(macrotext);
	} else {
		/* real macro allocing */
		macro = new_macro(macroname, macrotext);
	}
	return macro;
}

void add_macro_table(struct macro *macro)
{
	if (list_empty(&macro->list))
		list_add(&macro->list, tbl(macro->hash));
}

/*
 * @line is macro definition
 */
void add_macro(char *line, int len)
{
	struct macro *macro;
	/* check */
	if (isspace(*line)) {
		if (isspaceline(line))
			return;
		else
			text_errx("Format macro is invalid");
	}
	/* make it */
	macro = make_macro(line, len);
	/* add it to hash table */
	add_macro_table(macro);
	dbg("add macro hash:%8x name:%s text:%s",
			macro->hash, macro->name, macro->text);
}

char *expand_macro(char *name)
{
	struct macro *macro;
	macro = lookup_macro(name);
	return macro ? macro->text : NULL;
}

void init_macro(void)
{
	int i;
	for (i = 0; i < MACRO_HASH_SIZE; i++)
		list_init(&macrotable[i]);
}
