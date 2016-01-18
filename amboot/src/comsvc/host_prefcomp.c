/**
 * system/src/comsvc/host_prefcomp.c
 *
 * History:
 *    2006/04/11 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <comsvc/prefcomp.h>

#define AMB_PREF_H	"amb_pref.h"
#define AMB_PREF_C	"amb_pref.c"

extern int lineno;
extern FILE *yyin;
extern struct pref_data_s pref_data;
extern const char __embc[];
extern const int __embc_len;

/**
 * Program usage.
 */
static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s -i <infile> -p <prefix>\n", argv[0]);
}

/**
 * Parser error handling.
 */
int yyerror(const char *msg)
{
	fprintf(stderr, "syntax error: line %d\n", lineno);
	return -1;
}

/**
 * Key function table.
 */
struct key_func_tbl_s
{
	const char *type;	/**< Type name */
	const int array;	/**< Array? */
	const char *fn;		/**< Function name */
};

/**
 * Get 'set' call-function based on variable type.
 */
static const char *get_var_set_func(const struct pref_item_s *item)
{
	static const struct key_func_tbl_s var_set_func_tbl[] = {
		{ "u8",		0,	"set_u8_val", },
		{ "u16",	0,	"set_u16_val", },
		{ "u32",	0,	"set_u32_val", },
		{ "s8",		0,	"set_s8_val", },
		{ "s16",	0,	"set_s16_val", },
		{ "s32",	0,	"set_s32_val", },
		{ "char",	0,	"set_s8_val", },
		{ "short",	0,	"set_s16_val", },
		{ "int",	0,	"set_s32_val", },
		{ "long",	0,	"set_s32_val", },
		{ "char",	1,	"set_string_val", },
		{ "ethaddr_t",	0,	"set_ethaddr_val", },
		{ "ipaddr_t",	0,	"set_ipaddr_val", },
		{ NULL,		0,	NULL, },
	};
	int i;

	for (i = 0; var_set_func_tbl[i].type != NULL; i++) {
		if (strcmp(item->type, var_set_func_tbl[i].type) == 0 &&
		    (item->array > 0 ? 1 : 0) == var_set_func_tbl[i].array)
			return var_set_func_tbl[i].fn;
	}

	return NULL;
}

/**
 * Get 'get' call-function based on variable type.
 */
static const char *get_var_get_func(const struct pref_item_s *item)
{
	static const struct key_func_tbl_s var_get_func_tbl[] = {
		{ "u8",		0,	"get_u8_val", },
		{ "u16",	0,	"get_u16_val", },
		{ "u32",	0,	"get_u32_val", },
		{ "s8",		0,	"get_s8_val", },
		{ "s16",	0,	"get_s16_val", },
		{ "s32",	0,	"get_s32_val", },
		{ "char",	0,	"get_s8_val", },
		{ "short",	0,	"get_s16_val", },
		{ "int",	0,	"get_s32_val", },
		{ "long",	0,	"get_s32_val", },
		{ "char",	1,	"get_string_val", },
		{ "u8",		1,	"get_u8_array", },
		{ "u16",	1,	"get_u16_array", },
		{ "u32",	1,	"get_u32_array", },
		{ "s8",		1,	"get_s8_array", },
		{ "s16",	1,	"get_s16_array", },
		{ "s32",	1,	"get_s32_array", },
		{ "u32",	1,	"get_u32_array", },
		{ "short",	1,	"get_s16_array", },
		{ "int",	1,	"get_s32_array", },
		{ "long",	1,	"get_s32_array", },
		{ "ethaddr_t",	0,	"get_ethaddr_val", },
		{ "ipaddr_t",	0,	"get_ipaddr_val", },
		{ NULL,		0,	NULL, },
	};
	int i;

	for (i = 0; var_get_func_tbl[i].type != NULL; i++) {
		if (strcmp(item->type, var_get_func_tbl[i].type) == 0 &&
		    (item->array > 0 ? 1 : 0) == var_get_func_tbl[i].array)
			return var_get_func_tbl[i].fn;
	}

	return NULL;
}

/**
 * Header comment generation.
 */
static void output_source_header(FILE *fout)
{
	fprintf(fout, "/*\n");
	fprintf(fout, " * Created by the 'Preferences Compiler'\n");
	fprintf(fout, " * Written by: Charles Chiou <cchiou@ambarella.com>\n");
	fprintf(fout, " */\n\n");
}

/**
 * Output the proto-type and data structures.
 */
static void output_proto(FILE *fout, struct pref_data_s *pref)
{
	struct pref_inc_s *inc;
	struct pref_item_s *item;
	int runtime = 0;
	int factory = 0;

	fprintf(fout, "#ifndef __PREF_COMP_H__\n");
	fprintf(fout, "#define __PREF_COMP_H__\n\n");

	fprintf(fout, "#ifdef __cplusplus\n");
	fprintf(fout, "extern \"C\" {\n");
	fprintf(fout, "#endif\n\n");

	/* List includes */
	for (inc = pref_data.includes; inc; inc = inc->next)
		fprintf(fout, "#include \"%s\"\n", inc->file);
	fprintf(fout, "\n");

	/* Built-in data types */
	fprintf(fout, "#ifndef __NO_ETHADDR_TYPE__\n");
	fprintf(fout, "typedef struct ethaddr_s {\n");
	fprintf(fout, "\tunsigned char addr[6];\n");
	fprintf(fout, "} ethaddr_t;\n");
	fprintf(fout, "#endif  /* __NO_ETHADDR_TYPE__ */\n");
	fprintf(fout, "\n");
	fprintf(fout, "#ifndef __NO_IPADDR_TYPE__\n");
	fprintf(fout, "typedef unsigned int ipaddr_t;\n");
	fprintf(fout, "#endif  /* __NO_IPADDR_TYPE__ */\n");
	fprintf(fout, "\n");
	

	/* Walk through the list to get the count of runtime/factory */
	for (item = pref_data.items; item; item = item->next) {
		if (item->runtime)
			runtime++;
		if (item->factory)
			factory++;
	}

	/* Generate structure */
	fprintf(fout, "typedef struct app_pref_s\n{\n");
	if (runtime > 0) {
		fprintf(fout, "\t/* run-time values */\n");
		fprintf(fout, "\tstruct {\n");
		for (item = pref_data.items; item; item = item->next) {
			if (!item->runtime)
				continue;
			fprintf(fout, "\t\t%s %s", item->type, item->var);
			if (item->array > 0)
				fprintf(fout, "[%d]", item->array);
			fprintf(fout, ";\n");
		}
		fprintf(fout, "\t} runtime;\n");
	}
	if (factory > 0) {
		fprintf(fout, "\n\t/* factory calibrated values */\n");
		fprintf(fout, "\tstruct {\n");
		for (item = pref_data.items; item; item = item->next) {
			if (!item->factory)
				continue;
			fprintf(fout, "\t\t%s %s", item->type, item->var);
			if (item->array > 0)
				fprintf(fout, "[%d]", item->array);
			fprintf(fout, ";\n");
		}
		fprintf(fout, "\t} factory;\n");
	}
	fprintf(fout, "} app_pref_t;\n\n");

	/* Global default (const) */
	fprintf(fout, "extern const app_pref_t G_app_pref_default;\n\n");

	/* Function prototypes */
	fprintf(fout, "/* get key count */\n");
	fprintf(fout,
		"extern int app_pref_get_key_count(void);\n\n");
	fprintf(fout, "/* get key name */\n");
	fprintf(fout,
		"extern const char *app_pref_get_key_name(int idx);\n\n");
	fprintf(fout, "/* set value */\n");
	fprintf(fout,
		"extern int app_pref_set(app_pref_t *pref,\n"
		"\t\t\tconst char *key, const char *val);\n\n");
	fprintf(fout, "/* get value */\n");
	fprintf(fout,
		"extern int app_pref_get(app_pref_t *pref,\n"
		"\t\t\tconst char *key, char *val, int size_val);\n\n");
	fprintf(fout, "/* set factory value *\n");
	fprintf(fout,
		"extern int app_pref_set_factory(app_pref_t *pref,\n"
		"\t\t\tconst char *key, const char *val);\n\n");
	fprintf(fout, "/* get factory value */\n");
	fprintf(fout,
		"extern int app_pref_get_factory(app_pref_t *pref,\n"
		"\t\t\tconst char *key, char *val, int size_val);\n\n");

	fprintf(fout, "#ifdef __cplusplus\n");
	fprintf(fout, "}\n");
	fprintf(fout, "#endif\n\n");

	fprintf(fout, "#endif\n");
}

/**
 * Output the implementations.
 */
static void output_impl(FILE *fout, struct pref_data_s *pref)
{
	struct pref_item_s *item;
	int key_count = 0;
	int runtime = 0;
	int factory = 0;
	int i;

	fprintf(fout, "#include <stdio.h>\n");
	fprintf(fout, "#include <string.h>\n");
	fprintf(fout, "#include \"%s\"\n\n", AMB_PREF_H);

	/* Walk through the list to get the count of runtime/factory */
	for (item = pref_data.items; item; item = item->next) {
		key_count++;
		if (item->runtime)
			runtime++;
		if (item->factory)
			factory++;
	}

	/* Embedded C utils */
	i = fwrite(__embc, 1, __embc_len, fout);
	if (i != __embc_len)
		printf("fwrite failed\n");

	fprintf(fout, "\n\n");

	/* Global default (const) */
	fprintf(fout, "/* default object */\n");
	fprintf(fout, "const app_pref_t G_app_pref_default = {\n");
	if (runtime > 0) {
		fprintf(fout, "\t{  /* Run-time part */\n");
		for (item = pref_data.items; item; item = item->next) {
			if (!item->runtime)
				continue;
			if (item->array > 0) {
				if (strcmp(item->type, "char") == 0) {
					fprintf(fout, "\t\t\"%s\",",
						item->defval);
				} else {
					fprintf(fout, "\t\t{ %s },",
						item->defval);
				}
			} else {
				fprintf(fout, "\t\t%s,", item->defval);
			}
			fprintf(fout, " /* %s */\n", item->var);
		}
		fprintf(fout, "\t},\n");
	}
	if (factory > 0) {
		fprintf(fout, "\t{ /* Factory part */\n");
		for (item = pref_data.items; item; item = item->next) {
			if (!item->factory)
				continue;
			if (item->array > 0) {
				if (strcmp(item->type, "char") == 0) {
					fprintf(fout, "\t\t\"%s\",",
						item->defval);
				} else {
					fprintf(fout, "\t\t{ %s },",
						item->defval);
				}
			} else {
				fprintf(fout, "\t\t%s,", item->defval);
			}
			fprintf(fout, " /* factory.%s */\n", item->var);
		}
		fprintf(fout, "\t},\n");
	}
	fprintf(fout, "};\n\n");

	/* Lookup Table */
	fprintf(fout, "/* look-up table struct def. */\n");
	fprintf(fout, "struct pref_lut_s {\n");
	fprintf(fout, "\tint id;\n");
	fprintf(fout, "\tconst char *name;\n");
	fprintf(fout, "};\n\n");
	fprintf(fout, "/* look-up table */\n");
	fprintf(fout, "static const struct pref_lut_s lut[] = {\n");
	for (item = pref_data.items, i = 0; item; item = item->next, i++)
		fprintf(fout, "\t{\t%d,\t\"%s\", },\n", i, item->var);
	fprintf(fout, "\t{\t-1,\tNULL, },\n};\n\n");

	/* Lookup function */
	fprintf(fout, "/* lookup function */\n");
	fprintf(fout, "static int lookup_id(const char *s)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\tint i;\n\n");
	fprintf(fout, "\tfor (i = 0; lut[i].name; i++) {\n");
	fprintf(fout, "\t\tif (strcmp(s, lut[i].name) == 0)\n");
	fprintf(fout, "\t\t\treturn lut[i].id;\n");
	fprintf(fout, "\t}\n\n");
	fprintf(fout, "\treturn -1;\n}\n\n");

	/* app_pref_get_size() */
	fprintf(fout, "/*\n * app_pref_get_size()\n */\n");
	fprintf(fout, "int app_pref_get_size(void)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\treturn sizeof(app_pref_t);\n");
	fprintf(fout, "}\n\n");

	/* app_pref_init() */
	fprintf(fout, "/*\n * app_pref_init()\n */\n");
	fprintf(fout, "int app_pref_init(app_pref_t *pref)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\tmemcpy(pref, &G_app_pref_default, sizeof(*pref));\n");
	fprintf(fout, "\treturn sizeof(app_pref_t);\n");
	fprintf(fout, "}\n\n");

	/* app_pref_get_key_count() */
	fprintf(fout, "/*\n * app_pref_get_key_count()\n */\n");
	fprintf(fout, "int app_pref_get_key_count(void)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\treturn %d;\n", key_count);
	fprintf(fout, "}\n\n");

	/* app_pref_get_key_name() */
	fprintf(fout, "/*\n * app_pref_get_key_name()\n */\n");
	fprintf(fout,
		"const char *app_pref_get_key_name(int idx)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\treturn lut[idx].name;\n");
	fprintf(fout, "}\n\n");

	/* app_pref_set() */
	fprintf(fout, "/*\n * app_pref_set()\n */\n");
	fprintf(fout, "int app_pref_set(app_pref_t *pref,\n"
		"\t\tconst char *key, const char *val)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\tswitch (lookup_id(key)) {\n");
	for (item = pref_data.items, i = 0; item; item = item->next, i++) {
		const char *sfn;

		if (!item->runtime)
			continue;
		fprintf(fout, "\tcase %d: ", i);
		sfn = get_var_set_func(item);
		if (sfn == NULL) {
			fprintf(fout,
				"break; /* %s: type not supported! */\n",
				item->var);
		} else {
			if (item->array > 0) {
				fprintf(fout,
					"return %s(pref->runtime.%s, %d, val);\n",
					sfn, item->var, item->array);
			} else {
				fprintf(fout,
					"return %s(&pref->runtime.%s, val);\n",
					sfn, item->var);
			}
		}
	}
	fprintf(fout, "\t}\n\n");
	fprintf(fout, "\treturn -1;\n");
	fprintf(fout, "}\n\n");

	/* app_pref_get() */
	fprintf(fout, "/*\n * app_pref_get()\n */\n");
	fprintf(fout, "int app_pref_get(app_pref_t *pref,\n"
		"\t\tconst char *key, char *val, int size_val)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\tswitch (lookup_id(key)) {\n");
	for (item = pref_data.items, i = 0; item; item = item->next, i++) {
		const char *sfn;

		if (!item->runtime)
			continue;
		fprintf(fout, "\tcase %d: ", i);
		sfn = get_var_get_func(item);
		if (sfn == NULL) {
			fprintf(fout,
				"break; /* %s: type not supported! */\n",
				item->var);
		} else {
			if (item->array > 0) {
				fprintf(fout,
					"return %s(pref->runtime.%s, %d, val, size_val);\n",
					sfn, item->var, item->array);
			} else {
				fprintf(fout,
					"return %s(&pref->runtime.%s, val, size_val);\n",
					sfn, item->var);
			}
		}
	}
	fprintf(fout, "\t}\n\n");
	fprintf(fout, "\treturn -1;\n");
	fprintf(fout, "}\n\n");

	/* app_pref_set_factory() */
	fprintf(fout, "/*\n * app_pref_set_factory()\n */\n");
	fprintf(fout, "int app_pref_set_factory(app_pref_t *pref,\n"
		"\t\t\tconst char *key, const char *val)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\tswitch (lookup_id(key)) {\n");
	for (item = pref_data.items, i = 0; item; item = item->next, i++) {
		const char *sfn;

		if (!item->factory)
			continue;
		fprintf(fout, "\tcase %d: ", i);
		sfn = get_var_set_func(item);
		if (sfn == NULL) {
			fprintf(fout,
				"break; /* %s: type not supported! */\n",
				item->var);
		} else {
			if (item->array > 0) {
				fprintf(fout,
					"return %s(pref->factory.%s, %d, val);\n",
					sfn, item->var, item->array);
			} else {
				fprintf(fout,
					"return %s(&pref->factory.%s, val);\n",
					sfn, item->var);
			}
		}
	}
	fprintf(fout, "\t}\n\n");
	fprintf(fout, "\treturn -1;\n");
	fprintf(fout, "}\n\n");

	/* app_pref_get_factory() */
	fprintf(fout, "/*\n * app_pref_get_factory()\n */\n");
	fprintf(fout, "int app_pref_get_factory(app_pref_t *pref,\n"
		"\t\t\tconst char *key, char *val, int size_val)\n");
	fprintf(fout, "{\n");
	fprintf(fout, "\tswitch (lookup_id(key)) {\n");
	for (item = pref_data.items, i = 0; item; item = item->next, i++) {
		const char *sfn;

		if (!item->factory)
			continue;
		fprintf(fout, "\tcase %d: ", i);
		sfn = get_var_get_func(item);
		if (sfn == NULL) {
			fprintf(fout,
				"break; /* %s: type not supported! */\n",
				item->var);
		} else {
			if (item->array > 0) {
				fprintf(fout,
					"return %s(pref->factory.%s, %d, val, size_val);\n",
					sfn, item->var, item->array);
			} else {
				fprintf(fout,
					"return %s(&pref->factory.%s, val, size_val);\n",
					sfn, item->var);
			}
		}
	}
	fprintf(fout, "\t}\n\n");
	fprintf(fout, "\treturn -1;\n");
	fprintf(fout, "}\n\n");

	fprintf(fout, "/* implementation end */\n");
}

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
	int rval;
	struct stat buf;
	FILE *fout_c;
	FILE *fout_h;
	char path[512];

	if (argc != 3 && argc != 5) {
		usage(argc, argv);
		return -1;
	}

	if (strcmp(argv[1], "-i") != 0) {
		usage(argc, argv);
		return -2;
	}

	if (argc == 5) {
		if (strcmp(argv[3], "-p") != 0) {
			usage(argc, argv);
			return -3;
		}

		if (strcmp(argv[4], ".") == 0)
			argc = 3;
		else {
			rval = stat(argv[4], &buf);
			if (rval < 0) {
				perror(argv[4]);
				return -4;
			}

			if (!S_ISDIR(buf.st_mode)) {
				usage(argc, argv);
				return -5;
			}

			chdir(argv[4]);
		}
	}

	yyin = fopen(argv[2], "r");
	if (yyin == NULL) {
		perror(argv[2]);
		return -6;
	}

	if (yyparse() != 0) {
		return -7;
	}

	fout_h = fopen(AMB_PREF_H, "w");
	if (fout_h == NULL) {
		fprintf(stderr, "error: cannot open '%s'!\n", AMB_PREF_H);
		return -8;
	}
	output_source_header(fout_h);

	fout_c = fopen(AMB_PREF_C, "w");
	if (fout_c == NULL) {
		fprintf(stderr, "error: cannot open '%s'!\n", AMB_PREF_C);
		fclose(fout_h);
		return -9;
	}
	output_source_header(fout_c);

	output_proto(fout_h, &pref_data);
	output_impl(fout_c, &pref_data);

	fclose(fout_h);
	fclose(fout_c);

	printf("'%s' and '%s' have been generated\n", AMB_PREF_H, AMB_PREF_C);

	return 0;
}
