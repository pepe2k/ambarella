
/**
 * filter_list.h
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

class IFileReader;

struct parse_file_s {
	const char 	*pFileName;
	IFileReader	*pFileReader;
	AM_UINT		flags;
};

struct parser_obj_s {
	void	*context;
	void	(*free)(void *context);
};

typedef IFilter* (*FilterCreateFunc)(IEngine *pEngine);
typedef int (*MediaParseFunc)(parse_file_s *pParseFile, parser_obj_s *pParser);
typedef int (*AcceptMediaFunc)(CMediaFormat& format);

struct filter_entry
{
	const char		*pName;
	FilterCreateFunc	create;
	MediaParseFunc		parse;
	AcceptMediaFunc		acceptMedia;
};

extern filter_entry *g_filter_table[];

