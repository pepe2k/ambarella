/**
 * @file system/src/bld/bstpar_gen.y
 *
 * Parser for the bstconfig parameters file.
 *
 * History:
 *    2009/03/04 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <bstpar_gen.h>

struct bstpar_parsed_s g_bstpar_parsed = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
};

%}

%union {
	char *s;
	struct bstpar_list_s *l;
}

%token OPEN_BSTCONFIG OPEN_PARAM OPEN_VALUE
%token CLOSE_BSTCONFIG CLOSE_PARAM CLOSE_VALUE
%token STRING TITLE

%type <s> 	STRING
%type <s> 	TITLE	
%type <l> 	param_list
%type <l> 	param
%type <s>	value	

%%

bstpar_file	: TITLE OPEN_BSTCONFIG param_list CLOSE_BSTCONFIG
			{
				g_bstpar_parsed.title = $1;
				g_bstpar_parsed.list = $3;
			}
		;

param_list	: param
			{
				$$ = $1;
			}
		| param_list param
			{
				struct bstpar_list_s *pl;
				for (pl = $$; pl->next != NULL; pl = pl->next);
				pl->next = $2;
				$$ = $1;
			}
		;

param		: OPEN_PARAM STRING value CLOSE_PARAM
			{
				$$ = malloc(sizeof(struct bstpar_list_s));
				$$->name = $2;
				$$->value= $3;
				$$->next = NULL;
			}
		;

value		: OPEN_VALUE STRING CLOSE_VALUE
			{
				$$ = $2;
			}
		;
%%
