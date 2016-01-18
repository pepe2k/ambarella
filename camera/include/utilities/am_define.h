/*******************************************************************************
 * am_define.h
 *
 * Histroy:
 *  2012-3-1 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMDEFINE_H_
#define AMDEFINE_H_

#define ATTR_OFF       0
#define ATTR_BOLD      1
#define ATTR_BLINK     5
#define ATTR_REVERSE   7
#define ATTR_CONCEALED 8

#define FG_BLACK       30
#define FG_RED         31
#define FG_GREEN       32
#define FG_YELLOW      33
#define FG_BLUE        34
#define FG_MAGENTA     35
#define FG_CYAN        36
#define FG_WHITE       37

#define BG_BLACK       40
#define BG_RED         41
#define BG_GREEN       42
#define BG_YELLOW      43
#define BG_BLUE        44
#define BG_MAGENTA     45
#define BG_CYAN        46
#define BG_WHITE       47

#define B_RED(str)     "\033[1;31m"str"\033[0m"
#define B_GREEN(str)   "\033[1;32m"str"\033[0m"
#define B_YELLOW(str)  "\033[1;33m"str"\033[0m"
#define B_BLUE(str)    "\033[1;34m"str"\033[0m"
#define B_MAGENTA(str) "\033[1;35m"str"\033[0m"
#define B_CYAN(str)    "\033[1;36m"str"\033[0m"
#define B_WHITE(str)   "\033[1;37m"str"\033[0m"

#define RED(str)       "\033[31m"str"\033[0m"
#define GREEN(str)     "\033[32m"str"\033[0m"
#define YELLOW(str)    "\033[33m"str"\033[0m"
#define BLUE(str)      "\033[34m"str"\033[0m"
#define MAGENTA(str)   "\033[35m"str"\033[0m"
#define CYAN(str)      "\033[36m"str"\033[0m"
#define WHITE(str)     "\033[37m"str"\033[0m"

#define AM_LIKELY(x)   (__builtin_expect(!!(x),1))
#define AM_UNLIKELY(x) (__builtin_expect(!!(x),0))

#define minimum_str_len(a,b) \
  ((strlen(a) <= strlen(b)) ? strlen(a) : strlen(b))

#define is_str_equal(a,b) \
  ((strlen(a) == strlen(b)) && (0 == strcasecmp(a,b)))

#define is_str_n_equal(a,b,n) \
  ((strlen(a) >= n) && (strlen(b) >= n) && (0 == strncasecmp(a,b,n)))

#define is_str_same(a,b) \
  ((strlen(a) == strlen(b)) && (0 == strcmp(a,b)))

#define is_str_n_same(a,b,n) \
  ((strlen(a) >= n) && (strlen(b) >= n) && (0 == strncmp(a,b,n)))

#define is_str_start_with(str,prefix) \
  ((strlen(str)>strlen(prefix))&&(0 == strncasecmp(str,prefix,strlen(prefix))))

#define round_div(divident, divider) (((divident)+((divider)>>1)) / (divider))
#define round_up(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#define round_down(num, align) ((num) & ~((align) - 1))
#define AM_ABS(x) (((x) < 0) ? -(x) : (x))

char* amstrdup(const char* str);

#endif /* AMDEFINE_H_ */
