/**
 * system/src/comsvc/host_embc.c
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

#include <stdio.h>

static void usage(int argc, char **argv)
{
	fprintf(stderr, "Usage: %s <files>\n", argv[0]);
}

int main(int argc, char **argv)
{
	FILE *fout = stdout;
	FILE *fin;
	unsigned char buf[1024];
	int i, j;
	unsigned int l = 0;
	size_t size;

	if (argc <= 1) {
		usage(argc, argv);
		return -1;
	}

	fprintf(fout, "const char __embc[] = {\n");

	for (i = 1; i < argc; i++) {
		fin = fopen(argv[i], "r");
		if (fin == NULL) {
			fprintf(stderr, "error in opening '%s'!\n", argv[i]);
			return -2;
		}

		fprintf(fout, "\t/* \"%s\" */", argv[1]);
		while ((size = fread(buf, 1, sizeof(buf), fin)) > 0) {
			l += size;
			for (j = 0; j < size; j++) {
				if ((j % 8) == 0)
					fprintf(fout,"\n\t");
				fprintf(fout, "0x%.2x,", buf[j]);
			}
			fprintf(fout, "\n");
		}

		fclose(fin);
	}

	fprintf(fout, "};\n\n");
	fprintf(fout, "const int __embc_len = %d;\n\n", l);


	return 0;
}
