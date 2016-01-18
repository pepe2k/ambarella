/**
 * system/src/comsvc/host_embbin.c
 *
 * History:
 *    2005/11/02 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Parse a line in the configuration file.
 */
int parse_line(const char *line, const char *prefix,
	       char *binfile, char *binname)
{
	const char *bofn = line;

	*binfile = '\0';
	*binname = '\0';

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line))
		line++;

	/* Check for comment */
	if (*line == '#' || *line == '\0')
		return 0;

	/* Get 'binfile' */
	if (prefix)
		binfile += sprintf(binfile, "%s/", prefix);
	while (*line != '\0' && !isspace(*line) && *line != '#') {
		if (*line == '/')
			bofn = line;
		*binfile++ = *line++;
	}
	*binfile = '\0';

	if (*bofn == '/')
		bofn++;

	/* Skip white spaces */
	while (*line != '\0' && isspace(*line))
		line++;

	/* Check for comment */
	if (*line == '#' || *line == '\0') {
		while (*bofn != '\0' && !isspace(*bofn) && *bofn != '#')
			*binname++ = *bofn++;
		*binname = '\0';
		return 1;
	}

	/* Get 'binname' */
	while (*line != '\0' && !isspace(*line) && *line != '#')
		*binname++ = *line++;
	*binname = '\0';

	/* Consume rest of line */
	while (*line != '\0' && isspace(*line))
		line++;

	if (*line != '\0' && *line != '#') {
		return -1;	/* Line has more than allowed # of args */
	}
	
	return 2;
}

/**
 * Program entry point.
 */
int main(int argc, char **argv)
{
	int rval = 0;
	const char *prefix;
	const char *filelist;
	const char *outfile;
	FILE *fin = NULL;
	FILE *fout = NULL;
	FILE *fbin = NULL;
	int lineno;
	char buf[512];
	char binfile[512];
	char binname[512];
	int entries;
	int data;
	int len;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s [prefix] [filelist] [embbin.c]\n",
			argv[0]);
		rval = -1;
		goto done;
	}

	prefix = argv[1];
	filelist = argv[2];
	outfile = argv[3];

	fin = fopen(filelist, "r");
	if (fin == NULL) {
		fprintf(stderr, "Unable to open '%s' for input!\n", filelist);
		rval = -2;
		goto done;
	}

	fout = fopen(outfile, "w");
	if (fout == NULL) {
		fprintf(stderr, "Unable to open '%s' for output!\n", outfile);
		rval = -3;
		goto done;
	}

	/* Output file header */
	fprintf(fout, "/* %s: automatic generated file; don't edit! */\n",
		outfile);
	fprintf(fout, "/* %s %s */\n", __DATE__, __TIME__);
	fprintf(fout, "\n");

	/* Define structure in C */
	fprintf(fout, "struct embbin_s {\n");
	fprintf(fout, "\tconst char\t\t*name;\n");
	fprintf(fout, "\tconst unsigned char\t*data;\n");
	fprintf(fout, "\tconst int\t\tlen;\n");
	fprintf(fout, "};\n");
	fprintf(fout, "\n");

	/* First pass */
	entries = 0;
	for (lineno = 1; fgets(buf, sizeof(buf), fin) != NULL; lineno++) {
		rval = parse_line(buf, prefix, binfile, binname);
		if (rval < 0) {
			fprintf(stderr,
				"line %d looks suspicious! ignored...\n",
				lineno);
			continue;
		}
		if (rval == 0) {
			/* Line is pure comment */
			continue;
		}

		fbin = fopen(binfile, "rb");
		if (fbin == NULL) {
			fprintf(stderr, "Error: unable to open '%s'! ",
				binfile);
			exit(1);
		}

		fprintf(fout, "/* '%s' */\n", binname);
		fprintf(fout, "static const unsigned char bin_%d[] __attribute__ ((aligned(32))) = {",
			entries);
		for (len = 0; (data = fgetc(fbin)) != EOF; len++) {
			if ((len & 0x7) == 0x0)
				fprintf(fout, "\n\t\t");
			fprintf(fout, "0x%.2x, ", (unsigned char) data);
		}
		fprintf(fout, "\n};\n\n");

		fclose(fbin);
		entries++;
	}

	/* Output global array */
	fprintf(fout, "const struct embbin_s G_embbin[] = {\n");

	/* Second pass */
	fclose(fin);
	fin = fopen(filelist, "r");
	entries = 0;
	for (lineno = 1; fgets(buf, sizeof(buf), fin) != NULL; lineno++) {
		rval = parse_line(buf, prefix, binfile, binname);
		if (rval < 0) {
			continue;
		}
		if (rval == 0) {
			/* Line is pure comment */
			continue;
		}

		fbin = fopen(binfile, "rb");
		if (fbin == NULL) {
			continue;
		}

		for (len = 0; (data = fgetc(fbin)) != EOF; len++);
		fprintf(fout, "\t{ \"%s\", bin_%d, %d },\n",
			binname, entries, len);

		fclose(fbin);
		entries++;
	}

	/* Output last element */
	fprintf(fout, "\t{ (char *) 0x0, (unsigned char *) 0x0, -1 }\n");

	/* Close global array */
	fprintf(fout, "};\n");
	
	rval = 0;

done:
	if (fin != NULL) {
		fclose(fin);
		fin = NULL;
	}

	if (fout != NULL) {
		fclose(fout);
		fout = NULL;
	}

	return rval;
}
