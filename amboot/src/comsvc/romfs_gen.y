/**
 * @file system/src/comsvc/romfs_gen.y
 *
 * Parser for the ROMFS data compiler.
 *
 * History:
 *    2006/04/22 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define __ROMFS_IMPL__
#include <fio/romfs.h>

extern char *yytext;

struct romfs_parsed_s romfs_parsed = {
	NULL,
	NULL,
};

%}

%union {
	char *s;
	struct romfs_parsed_s *p;
	struct romfs_list_s *l;
}

%token OPEN_ROMFS OPEN_TOP OPEN_FILE OPEN_ALIAS
%token CLOSE_ROMFS CLOSE_TOP CLOSE_FILE CLOSE_ALIAS
%token STRING

%type <s> STRING
%type <s> top
%type <l> file_list
%type <l> file
%type <s> alias

%%

romfs		: OPEN_ROMFS top file_list CLOSE_ROMFS
			{
				romfs_parsed.top = $2;
				romfs_parsed.list = $3;
			}
		;
top		: /* empty */
			{
				$$ = NULL;
			}
		| OPEN_TOP STRING CLOSE_TOP
			{
				$$ = $2;
			}
		;
file_list	: file
			{
				$$ = $1;
			}
		| file_list file
			{
				struct romfs_list_s *x;
				for (x = $$; x->next != NULL; x = x->next);
				x->next = $2;
				$$ = $1;
			}
		;
file		: OPEN_FILE STRING alias CLOSE_FILE
			{
				$$ = malloc(sizeof(struct romfs_list_s));
				$$->file = $2;
				$$->alias = $3;
			}
		;
alias		: /* empty */
			{
				$$ = NULL;
			}
		| OPEN_ALIAS STRING CLOSE_ALIAS
			{
				$$ = $2;
			}
		;
%%
