/**
 * @file system/src/comsvc/romfs_gen.l
 *
 * Lexer for the ROMFS data compiler.
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
#include "romfs_gen.tab.h"

int lineno = 1;

%}

ws	[ \t]
blank	[ \t]*
comment	<!-.*->
word	[A-Za-z0-9_\./\-]+
nl	\n

%option never-interactive
%option noyywrap

%%

"<romfs>"	{ return OPEN_ROMFS; }
"</romfs>"	{ return CLOSE_ROMFS; }
"<top>"		{ return OPEN_TOP; }
"</top>"	{ return CLOSE_TOP; }
"<file>"	{ return OPEN_FILE; }
"</file>"	{ return CLOSE_FILE; }
"<alias>"	{ return OPEN_ALIAS; }
"</alias>"	{ return CLOSE_ALIAS; }

{ws}		;
{blank}		;
{comment}	;
{word}		{ yylval.s = strdup(yytext); return STRING; }
{nl}		{ lineno++; }
.		{ return yytext[0]; }		/* a single char token */

%%
