/*
 * am_qrcode_parser.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 11/03/2013 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_QRCODE_PARSER_H__
#define __AM_QRCODE_PARSER_H__

typedef struct am_qrcode_config_node {
   char *key_word;
   char *key_value;
   struct am_qrcode_config_node *next;
} am_qrcode_config_node_t;

typedef struct am_qrcode_config_head {
   char *config_word;
   struct am_qrcode_config_node *head;
   struct am_qrcode_config_head *next;
} am_qrcode_config_head_t;

class AmQrcodeParser {
public:
   static AmQrcodeParser *Create ();
   bool ParseQrcodeConfig (const char *qrcode_config);
   void PrintQrcodeConfig ();
   char *GetKeyValue (const char *config_name, const char *key_word);

public:
   AmQrcodeParser ();
   virtual ~AmQrcodeParser ();

private:
   bool ParseKeyword (am_qrcode_config_node_t *node, char *entry, int *);

private:
   am_qrcode_config_head_t *mHead;
};

#endif
