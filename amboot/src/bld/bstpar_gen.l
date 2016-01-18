/**
 * @file system/src/bld/bstpar_gen.l
 *
 * Lexer for the bstconfig parameters file.
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

#include <string.h>
#include <bstpar_gen.h>
#include "bstpar_gen.tab.h"

int lineno = 1;
extern struct bstpar_parsed_s g_bstpar_parsed;

%}

ws	[ \t]+
nl	\n
word	[A-Za-z0-9_\.\-/]+
words	[A-Za-z0-9_\.\- /%]+
title	<\?.*\?>

%option never-interactive
%option noyywrap

%x VALUE

%%

\<BSTConfig.*>	{
			if (g_bstpar_parsed.open_bst == NULL)
				g_bstpar_parsed.open_bst = strdup(yytext);

			return OPEN_BSTCONFIG;
		}

"</BSTConfig>"	{
			if (g_bstpar_parsed.close_bst == NULL)
				g_bstpar_parsed.close_bst = strdup(yytext);

			return CLOSE_BSTCONFIG;
		}

"<"		{
			if (g_bstpar_parsed.open_param == NULL)
				g_bstpar_parsed.open_param = strdup(yytext);

			return OPEN_PARAM;
		}

"/>"		{
			if (g_bstpar_parsed.close_param== NULL)
				g_bstpar_parsed.close_param = strdup(yytext);

			return CLOSE_PARAM;
		}

"value=\""	{
			if (g_bstpar_parsed.open_value == NULL)
				g_bstpar_parsed.open_value = strdup(yytext);

			BEGIN(VALUE);
			return OPEN_VALUE;
		}

<VALUE>"\""	{
			if (g_bstpar_parsed.close_value == NULL)
				g_bstpar_parsed.close_value = strdup(yytext);
			
			BEGIN(INITIAL);
			return CLOSE_VALUE;
		}

{ws}		;
{title}		{ yylval.s = strdup(yytext);	return TITLE; }
<VALUE>{words}	{ yylval.s = strdup(yytext);	return STRING; }
{word}		{ yylval.s = strdup(yytext);	return STRING; }
{nl}		{ lineno++; }
.		{ return yytext[0]; }

%%
