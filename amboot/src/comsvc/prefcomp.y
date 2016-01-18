/**
 * system/src/comsvc/prefcomp.y
 *
 * Parser for the 'prefernces' compiler.
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

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comsvc/prefcomp.h>

extern char *yytext;

struct pref_data_s pref_data = {
	NULL,
	NULL,
};

%}

%union {
	char *s;
	int i;
	struct pref_inc_s *h;
	struct pref_item_s *t;
}

%token OPEN_PREF OPEN_INC OPEN_ITEM
%token OPEN_VAR OPEN_TYPE OPEN_DEFAULT FACTORY FACTORY_ONLY
%token CLOSE_PREF CLOSE_INC CLOSE_ITEM
%token CLOSE_VAR CLOSE_TYPE CLOSE_DEFAULT
%token OPEN_ARRAY CLOSE_ARRAY
%token STRING
%token NONE
%type <s> STRING
%type <h> pref_inc_list
%type <h> pref_inc
%type <t> pref_item_list
%type <t> pref_item
%type <t> pref_var
%type <t> pref_type
%type <i> array
%type <t> pref_defval

%%

pref		: OPEN_PREF pref_inc_list pref_item_list CLOSE_PREF
			{
				pref_data.includes = $2;
				pref_data.items = $3;
			}
		;
pref_inc_list	: NONE /* empty */
			{
				$$ = NULL;
			}
		| pref_inc
			{
				$$ = $1;
			}
		| pref_inc_list pref_inc
			{
				struct pref_inc_s *x;
				for (x = $$; x->next != NULL; x = x->next);
				x->next = $2;
				$$ = $1;
			}
		;
pref_inc	: OPEN_INC STRING CLOSE_INC
			{
				$$ = malloc(sizeof(struct pref_inc_s));
				$$->file = $2;
				$$->next = NULL;
			}
		;
pref_item_list	: pref_item
			{
				$$ = $1;
			}
		| pref_item_list pref_item
			{
				struct pref_item_s *x;
				for (x = $$; x->next != NULL; x = x->next);
				x->next = $2;
				$$ = $1;
			}
		;
pref_item	: OPEN_ITEM pref_var CLOSE_ITEM
			{
				$$ = $2;
			}
		;
pref_var	: OPEN_VAR STRING CLOSE_VAR pref_type
			{
				$$ = $4;
				$$->var = $2;
			}
		;
pref_type	: OPEN_TYPE STRING array CLOSE_TYPE pref_defval
			{
				$$ = $5;
				$$->type = $2;
				$$->array = $3;
			}
		;
array		: /* empty */
			{
				$$ = 0;
			}
		| OPEN_ARRAY STRING CLOSE_ARRAY
			{
				$$ = (int) atol($2);
			}
		;
pref_defval	: OPEN_DEFAULT STRING CLOSE_DEFAULT FACTORY
			{
				$$ = malloc(sizeof(struct pref_item_s));
				memset($$, 0x0, sizeof(*$$));
				$$->defval = $2;
				$$->runtime = 1;
				$$->factory = 1;
				$$->next = NULL;
			}
		| OPEN_DEFAULT STRING CLOSE_DEFAULT FACTORY_ONLY
			{
				$$ = malloc(sizeof(struct pref_item_s));
				memset($$, 0x0, sizeof(*$$));
				$$->defval = $2;
				$$->runtime = 0;
				$$->factory = 1;
				$$->next = NULL;
			}
		| OPEN_DEFAULT STRING CLOSE_DEFAULT
			{
				$$ = malloc(sizeof(struct pref_item_s));
				memset($$, 0x0, sizeof(*$$));
				$$->defval = $2;
				$$->runtime = 1;
				$$->factory = 0;
				$$->next = NULL;
			}
		;

%%

