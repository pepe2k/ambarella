/**
 * system/src/comsvc/prefcomp.l
 *
 * Lexer for the 'preferences' compiler.
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

#include <string.h>
#include "prefcomp.tab.h"

int lineno = 1;

%}

ws	[ \t]
blank	[ \t]*
comment	<!-.*->
word	[A-Za-z0-9_{}\., \\/]+
nl	\n

%option never-interactive
%option noyywrap

%%

"<pref>"	{ return OPEN_PREF; }
"</pref>"	{ return CLOSE_PREF; }
"<include>"	{ return OPEN_INC; }
"</include>"	{ return CLOSE_INC; }
"<item>"	{ return OPEN_ITEM; }
"</item>"	{ return CLOSE_ITEM; }
"<var>"		{ return OPEN_VAR; }
"</var>"	{ return CLOSE_VAR; }
"<type>"	{ return OPEN_TYPE; }
"</type>"	{ return CLOSE_TYPE; }
"<array>"	{ return OPEN_ARRAY; }
"</array>"	{ return CLOSE_ARRAY; }
"<default>"	{ return OPEN_DEFAULT; }
"</default>"	{ return CLOSE_DEFAULT; }
"<factory>"	{ return FACTORY; }
"<factory-only>"	{ return FACTORY_ONLY; }
{ws}		;
{blank}		;
{comment}	;
{word}		{ yylval.s = strdup(yytext); return STRING; }
{nl}		{ lineno++; }
.		{ return yytext[0]; }		/* a single char token */

%%
