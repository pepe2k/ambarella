/**********************************************************************
 *
 * mw_config.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
 *
 * Copyright (C) 2007 - 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <basetypes.h>
#include "mw_defines.h"
#include "mw_struct.h"
#include "mw_api.h"

#define	MAX_ITEMS_TO_PARSE		(3*1000)

/*************************************************
 *
 *		Static Functions, for file internal used
 *
 *************************************************/

/*
static void usage(void)
{
	fprintf(stderr, "   test_config [-h] [-d defenc.cfg] {[-f curenc1.cfg]...[-f curencN.cfg]}"
		" {[-p EncParam1=EncValue1]..[-p EncParamM=EncValueM]}\n\n"
		"## Parameters\n\n"

		"## Options\n"
		"   -v :  prints version\n"
		"   -h :  prints function usage\n"
		"   -d :  use <defenc.cfg> as default file for parameter initializations.\n"
		"         If not used then file defaults to ambaipcam.cfg in local directory.\n"
		"   -f :  read <curencX.cfg> for reseting selected encoder parameters.\n"
		"         Multiple files could be used that set different parameters\n"
		"   -p :  Set parameter <EncParamX> to <EncValueY>.\n"
		"         See default encoder.cfg file for description of all parameters.\n\n"

		"## Examples of usage:\n"
		"   test_config\n"
		"   test_config  -h\n"
		"   test_config  -d default.cfg\n"
		"   test_config  -f curenc1.cfg\n"
		"   test_config  -f curenc1.cfg -p M=3 -p N=15 -p framerate=30\n");
}

static char * get_config_file_content(char *file)
{
	long FileSize;
	FILE *f;
	char *buf;
	char filename[256];

	sprintf(filename, "./%s", file);
	if (NULL == (f = fopen(filename, "r"))) {
		sprintf(filename, "/usr/local/bin/%s", file);
		if (NULL == (f = fopen(filename, "r"))) {
			printf("Cannot open configuration file [%s]\n", filename);
			return NULL;
		}
	}
	if (0 != fseek(f, 0, SEEK_END)) {
		printf("Cannot fseek in configuration file [%s]\n", filename);
		return NULL;
	}

	FileSize = ftell(f);
	if (FileSize < 0 || FileSize > 100000) {
		printf("Unreasonable filesize [%ld] reported by ftell for configuration file [%s]\n",
			FileSize, filename);
		return NULL;
	}
	if (0 != fseek(f, 0, SEEK_SET)) {
		printf("Cannot fseek in configuration file [%s]\n", filename);
		return NULL;
	}

	if (NULL == (buf = (char *)malloc(FileSize + 1))) {
		printf("Cannot allocate memory : get_config_file_content: buf\n");
		return NULL;
	}

	// Note that ftell() gives us the file size as the file system sees it.  The actual file size,
	// as reported by fread() below will be often smaller due to CR/LF to CR conversion and/or
	// control characters after the dos EOF marker in the file.

	FileSize = fread(buf, 1, FileSize, f);
	buf[FileSize] = '\0';

	fclose(f);
	return buf;
}
*/


static int parse_items(char **items, char *buf, int bufsize)
{
	int item = 0;
	int InString = 0, InItem = 0;
	char *p = buf;
	char *bufend = &buf[bufsize];

	while (p < bufend) {
		switch (*p) {
		case 13:
			++p;
			break;

		case '#':					// Found comment
			*p = '\0';
			while ((*p != '\n') && (p < bufend))
				++p;
			InString = 0;
			InItem = 0;
			break;

		case '\n':
			InItem = 0;
			InString = 0;
			*p++ = '\0';
			break;

		case ' ':
		case '\t':					// Skip whitespace, leave state unchanged
			if (InString) {
				++p;
			} else {
				*p++ = '\0';
				InItem = 0;
			}
			break;

		case '"':					// Begin/End of String
			*p++ = '\0';
			if (!InString) {
				items[item++] = p;
				InItem = ~InItem;
			} else {
				InItem = 0;
			}
			InString = ~InString;	// Toggle
			break;

		default:
			if (!InItem) {
				items[item++] = p;
				InItem = ~InItem;
			}
			++p;
			break;
		}
	}

	return (item - 1);
}

static int parse_name_to_map_index(Mapping * Map, char * s)
{
	int i = 0;

	while (NULL != Map[i].TokenName) {
		if (0 == strcmp(Map[i].TokenName, s))
			return i;
		else 
			++i;
	}

	return -1;
}

static void parse_content(Mapping * Map, char * buf, int bufsize)
{
	char		* items[MAX_ITEMS_TO_PARSE] = {NULL};
	int		MapIndex;
	int		item = 0;
	int		IntContent;
	u32		U32Content;
	double	DoubleContent;
	char		msg[256];
	int i;

	// Stage one: Generate an argc/argv-type list in items[], without comments and whitespace.
	// This is context insensitive and could be done most easily with lex(1).
	item = parse_items(items, buf, bufsize);

	memset(msg, 0, sizeof(msg));
	// Stage two: interpret the Value, context sensitive.
	for (i = 0; i < item; i += 3) {
		if (0 > (MapIndex = parse_name_to_map_index(Map, items[i]))) {
//			printf("\n\tParsing error in config file: Parameter Name [%s] not recognized.\n", items[i]);
			continue;
		}
		if (strcmp("=", items[i+1])) {
			printf(" Parsing error in config file: '=' expected as the second token in each line.\n");
			exit(0);
		}

		// Now interpret the Value, context sensitive...
		switch (Map[MapIndex].Type) {
		case MAP_TO_U32:						// Numerical, unsigned integer (u32)
			if (1 != sscanf(items[i+2], "%u", &U32Content)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (u32 *) (Map[MapIndex].Address) = U32Content;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_U16:						// Numerical, unsigned integer (u16)
			if (1 != sscanf(items[i+2], "%u", &U32Content)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (u16 *) (Map[MapIndex].Address) = U32Content;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_U8:							// Numerical, unsigned integer (u8)
			if (1 != sscanf(items[i+2], "%u", &U32Content)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (u8 *) (Map[MapIndex].Address) = U32Content;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_S32:						// Numerical, signed integer
			if (1 != sscanf(items[i+2], "%d", &IntContent)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (int *) (Map[MapIndex].Address) = IntContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_S16:						// Numerical, signed short
			if (1 != sscanf(items[i+2], "%d", &IntContent)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (s16 *) (Map[MapIndex].Address) = IntContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_S8:						// Numerical, signed char
			if (1 != sscanf(items[i+2], "%d", &IntContent)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (s8 *) (Map[MapIndex].Address) = IntContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_DOUBLE:					// Numerical, double
			if (1 != sscanf(items[i+2], "%lf", &DoubleContent)) {
				printf("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (double *) (Map[MapIndex].Address) = DoubleContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_STRING:						// String
			memset((char *) Map[MapIndex].Address, 0, Map[MapIndex].StringLengthLimit);
			if (NULL != items[i+2]) {
				strncpy((char *) Map[MapIndex].Address, items[i+2], Map[MapIndex].StringLengthLimit - 1);
			} else {
				memset((char *) Map[MapIndex].Address, 0, Map[MapIndex].StringLengthLimit);
			}
			sprintf(msg, "%s.", msg);
			break;

		default:
			MW_ERROR("== parse_content == Unknown value type in the map definition!\n");
			exit(0);
		}
	}
	MW_INFO("Parse parameters : %s\n", msg);
}

static int load_default_params(Mapping * Map)
{
	int i = 0;

	while (NULL != Map[i].TokenName) {
		switch (Map[i].Type) {
		case MAP_TO_U32:
			* (u32 *) (Map[i].Address) = (u32) ((int) Map[i].Default);
			break;
		case MAP_TO_U16:
			* (u16 *) (Map[i].Address) = (u16) ((int) Map[i].Default);
			break;
		case MAP_TO_U8:
			* (u8 *) (Map[i].Address) = (u8) ((int) Map[i].Default);
			break;
		case MAP_TO_S32:
			* (int *) (Map[i].Address) = (int) Map[i].Default;
			break;
		case MAP_TO_S16:
			* (s16 *) (Map[i].Address) = (s16) ((int) Map[i].Default);
			break;
		case MAP_TO_S8:
			* (s8 *) (Map[i].Address) = (s8) ((int) Map[i].Default);
			break;
		case MAP_TO_DOUBLE:
			* (double *) (Map[i].Address) = Map[i].Default;
			break;
		case MAP_TO_STRING:
			* (char *) (Map[i].Address) = '\0';
			break;
		default:
			MW_ERROR("== load_default_params == Unknown value type in the map definition!\n");
			exit(0);
		}
		++i;
	}

	return 0;
}

static int check_params(Mapping * Map)
{
	int i = 0;

	while (Map[i].TokenName != NULL) {
		switch (Map[i].Type) {
		case MAP_TO_U32:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(u32 *)(Map[i].Address) < (u32)(int)(Map[i].MinLimit)) ||
					(*(u32 *)(Map[i].Address) > (u32)(int)(Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d]\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(u32 *)(Map[i].Address) = (u32)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(u32 *)(Map[i].Address) < (u32)(int)(Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(u32 *)(Map[i].Address) = (u32)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(u32 *)(Map[i].Address) > (u32)(int)(Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(u32 *)(Map[i].Address) = (u32)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_U16:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(u16 *)(Map[i].Address) < (u16)(int)(Map[i].MinLimit)) ||
					(*(u16 *)(Map[i].Address) > (u16)(int)(Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d]\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(u16 *)(Map[i].Address) = (u16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(u16 *)(Map[i].Address) < (u16)(int)(Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(u16 *)(Map[i].Address) = (u16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(u16 *)(Map[i].Address) > (u16)(int)(Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(u16 *)(Map[i].Address) = (u16)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_U8:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(u8 *)(Map[i].Address) < (u8)(int)(Map[i].MinLimit)) ||
					(*(u8 *)(Map[i].Address) > (u8)(int)(Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(u8 *)(Map[i].Address) = (u8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(u8 *)(Map[i].Address) < (u8)(int)(Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(u8 *)(Map[i].Address) = (u8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(u8 *)(Map[i].Address) > (u8)(int)(Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(u8 *)(Map[i].Address) = (u8)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_S32:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(int *)(Map[i].Address) < (int)(Map[i].MinLimit)) ||
					(*(int *)(Map[i].Address) > (int)(Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(int *)(Map[i].Address) = (int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(int *)(Map[i].Address) < (int)(Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(int *)(Map[i].Address) = (int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(int *)(Map[i].Address) > (int)(Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(int *)(Map[i].Address) = (int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_S16:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(s16 *)(Map[i].Address) < (s16)(int)(Map[i].MinLimit)) ||
					(*(s16 *)(Map[i].Address) > (s16)(int)(Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(s16 *)(Map[i].Address) = (s16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(s16 *)(Map[i].Address) < (s16)(int)(Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(s16 *)(Map[i].Address) = (s16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(s16 *)(Map[i].Address) > (s16)(int)(Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(s16 *)(Map[i].Address) = (s16)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_S8:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(s8 *)(Map[i].Address) < (s8)(int)(Map[i].MinLimit)) ||
					(*(s8 *)(Map[i].Address) > (s8)(int)(Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(s8 *)(Map[i].Address) = (s8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(s8 *)(Map[i].Address) < (s8)(int)(Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(s8 *)(Map[i].Address) = (s8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(s8 *)(Map[i].Address) > (s8)(int)(Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(s8 *)(Map[i].Address) = (s8)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_DOUBLE:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(double *)(Map[i].Address) < (Map[i].MinLimit)) ||
					(*(double *)(Map[i].Address) > (Map[i].MaxLimit))) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%.2lf, %.2lf] range.\n"
						"Use default value [%.2lf].\n",
						Map[i].TokenName, Map[i].MinLimit,
						Map[i].MaxLimit, Map[i].Default);
					*(double *)(Map[i].Address) = (Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(double *)(Map[i].Address) < (Map[i].MinLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%.2lf].\n"
						"Use default value [%.2lf].\n",
						Map[i].TokenName, Map[i].MinLimit,
						Map[i].Default);
					*(double *)(Map[i].Address) = (Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(double *)(Map[i].Address) > (Map[i].MaxLimit)) {
					printf("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%.2lf].\n"
						"Use default value [%.2lf].\n",
						Map[i].TokenName, Map[i].MaxLimit,
						Map[i].Default);
					*(double *)(Map[i].Address) = (Map[i].Default);
				}
			}
			break;

		default:			// string
			break;
		}
		++i;
	}

	return 0;
}

static int update_params(Mapping * Map)
{
	check_params(Map);
//	memcpy(pInp, &cfg, sizeof(InputParameters));
	return 0;
}

static int configure(Mapping *Map, const char * content)
{
	if (NULL == content) {
		load_default_params(Map);
	} else {
		char *pContent = NULL;
		u32 contentSize = 0;
		contentSize = strlen(content);
		pContent = (char *) malloc(contentSize + 1);
		strncpy(pContent, content, contentSize);
		pContent[contentSize] = 0;
		parse_content(Map, pContent, contentSize);
		update_params(Map);
		free(pContent);
	}
	return 0;
}

/*************************************************
 *
 *		Public Functions, for external used
 *
 *************************************************/

int mw_config_params(Mapping *pMap, char * pContent)
{
	int retv = 0;
	if (NULL == pMap)
		return -1;
	retv = configure(pMap, pContent);
	return retv;
}

int mw_output_params(Mapping * Map, char ** pContentOut)
{
	char * content = *pContentOut;
	int	i = 0;
	u32	unsigned_value;
	int	signed_value;

	content[0] = '\0';
	while (NULL != Map[i].TokenName) {
		sprintf(content, "%s%s", content, Map[i].TokenName);
		switch (Map[i].Type) {
		case MAP_TO_U32:
			unsigned_value = * (u32 *) (Map[i].Address);
			sprintf(content, "%s = %u\n", content, unsigned_value);
			break;
		case MAP_TO_U16:
			unsigned_value = * (u16 *) (Map[i].Address);
			sprintf(content, "%s = %u\n", content, unsigned_value);
			break;
		case MAP_TO_U8:
			unsigned_value = * (u8 *) (Map[i].Address);
			sprintf(content, "%s = %u\n", content, unsigned_value);
			break;
		case MAP_TO_S32:
			signed_value = * (int *) (Map[i].Address);
			sprintf(content, "%s = %d\n", content, signed_value);
			break;
		case MAP_TO_S16:
			signed_value = * (s16 *) (Map[i].Address);
			sprintf(content, "%s = %d\n", content, signed_value);
			break;
		case MAP_TO_S8:
			signed_value = * (s8 *) (Map[i].Address);
			sprintf(content, "%s = %d\n", content, signed_value);
			break;
		case MAP_TO_DOUBLE:
			sprintf(content, "%s = %.2lf\n", content, * (double *) (Map[i].Address));
			break;
		case MAP_TO_STRING:
			sprintf(content, "%s = \"%s\"\n", content, (char *) (Map[i].Address));
			break;
		default:
			sprintf(content, "%s : [%d] unknown value type.\n", content, Map[i].Type);
			MW_ERROR("== Output Parameters == Unknown value type in the map definition !!!\n");
			break;
		}
		++i;
	}
	return 0;
}

#define __END_OF_FILE__

