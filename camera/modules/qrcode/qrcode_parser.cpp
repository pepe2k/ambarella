/*
 * qrcode_parser.h
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <zbar.h>

#include "am_include.h"
#include "am_data.h"
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_qrcode.h"

AmQrcodeParser *AmQrcodeParser::Create ()
{
   return new AmQrcodeParser ();
}

AmQrcodeParser::AmQrcodeParser ():
   mHead (NULL) {}

AmQrcodeParser::~AmQrcodeParser ()
{
   am_qrcode_config_node_t *temp_node;
   am_qrcode_config_head_t *temp_head;

   while (mHead != NULL) {
      while (mHead->head != NULL) {
         temp_node = mHead->head;
         mHead->head = temp_node->next;

         delete temp_node->key_word;
         delete temp_node->key_value;
         delete temp_node;
      }

      temp_head = mHead;
      mHead = mHead->next;

      delete temp_head->config_word;
      delete temp_head;
   }
}

bool AmQrcodeParser::ParseQrcodeConfig (const char *qrcode_config)
{
   int len, i;
   char *start, *end;
   am_qrcode_config_head_t *config;
   am_qrcode_config_node_t *node;

   if (qrcode_config == NULL) {
      NOTICE ("Warning: Empty string");
      return false;
   }

   for (start = (char*)qrcode_config, end = start + strlen(start) - 1;
       end > start; --end) {
     if (*end == ';' && *(end-1) == ';') {
       *(end+1) = '\0';
       break;
     }
   }
   if (end == start) {
     ERROR("Wrong format!");
     return false;
   }

   start = end = (char *)qrcode_config;
   while (*end != '\0') {
      /* Allocate memory to store a certain config. */
      if ((config = new am_qrcode_config_head_t) == NULL) {
         ERROR ("Failed to allocate memory!");
         return false;
      }

      /* Insert the empty config into list. */
      config->next = mHead;
      mHead = config;
      config->head = NULL;

      for (; *end != ':'; end++) {
         if (*end == '\\') {
            end++;
         }
      }

      /*
       * Allocate memory to store the name of config,
       * such as wifi, cld ...
       *
       * '\0' takes up one byte.
       */
      len = (int)(end - start) + 1;
      if ((config->config_word = new char[len]) == NULL) {
         ERROR ("Failed to allocate memory!");
         return false;
      }

      for (i = 0; start != end; start++, i++) {
         if (*start == '\\') {
            config->config_word[i] = *(++start);
         } else {
            config->config_word[i] = *start;
         }
      }

      config->config_word[i] = '\0';
      for (++end; *end != ';'; end++) {
         if ((node = new am_qrcode_config_node_t) == NULL) {
            ERROR ("Failed to allocate memory!");
            return false;
         }

         node->next = config->head;
         config->head = node;

         if (!ParseKeyword (node, end, &len)) {
            ERROR ("Failed to parse one key word!");
            return false;
         }

         end += len - 1;
      }

      start = ++end;
   }

   return true;
}

bool AmQrcodeParser::ParseKeyword (am_qrcode_config_node_t *node,
      char *entry, int *len)
{
   int str_len, i;
   char *start, *end;

   start = end = entry;
   for (; *end != ':'; end++) {
      if (*end == '\\') {
         end++;
      }
   }

   str_len = (int)(end - start) + 1;
   if ((node->key_word = new char[str_len]) == NULL) {
      ERROR ("Failed to allocate memory!");
      return false;
   }

   *len = str_len;
   for (i = 0; start != end; start++, i++) {
      if (*start == '\\') {
         node->key_word[i] = *(++start);
      } else {
         node->key_word[i] = *start;
      }
   }

   node->key_word[i] = '\0';
   for (start = ++end; *end != ';'; end++) {
      if (*end == '\\') {
         end++;
      }
   }

   str_len = (int)(end - start) + 1;
   if ((node->key_value = new char[str_len]) == NULL) {
      ERROR ("Failed to allocate memory!");
      return false;
   }

   *len += str_len;
   for (i = 0; start != end; start++, i++) {
      if (*start == '\\') {
         node->key_value[i] = *(++start);
      } else {
         node->key_value[i] = *start;
      }
   }

   node->key_value[i] = '\0';
   return true;
}

char *AmQrcodeParser::GetKeyValue (const char *config_name, const char *key_word)
{
   am_qrcode_config_node_t *node;
   am_qrcode_config_head_t *head;

   if (!key_word) {
      ERROR ("Key word is null!");
      return NULL;
   }

   for (head = mHead; head; head = head->next) {
      if (config_name != NULL) {
         if (strcmp (head->config_word, config_name)) continue;
      }

      for (node = head->head; node; node = node->next) {
         if (!strcmp (node->key_word, key_word)) {
            return node->key_value;
         }
      }
   }

   return NULL;
}

void AmQrcodeParser::PrintQrcodeConfig ()
{
   am_qrcode_config_node_t *node;
   am_qrcode_config_head_t *head;

   if (mHead == NULL) {
      ERROR ("No qrcode config has been parsed!\n");
      return;
   }

   for (head = mHead; head; head = head->next) {
      NOTICE ("Config name: %s\n", head->config_word);
      for (node = head->head; node; node = node->next) {
         NOTICE ("%s: %s\n", node->key_word, node->key_value);
      }

      NOTICE ("\n");
   }
}
