/*******************************************************************************
 * cali_fish_center.c
 *
 * History:
 *  Aug 2, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <basetypes.h>

#include "utils.h"

#define ABS(x)            ((x < 0) ? -(x) : (x))

#define MAX_FILENAME_SIZE    256
#define MIN_CLUSTER          16

typedef signed long long   s64; /**< SIGNED 64-bit data type */

typedef struct axis_s {
	int x;
	int y;
} axis_t;

typedef struct arc_s {
	axis_t begin;
	axis_t end;
} arc_t;

typedef enum {
	DOWN = 0,
	UP,
	RIGHT,
	LEFT,
	DIR_TOTAL_NUM
} DIR;

typedef enum {
	NO_CHANGE = 0,
	FRINGE_BEGIN,
	FRINGE_END
} CHANGE_STATE;

// static const char* g_dir_str[DIR_TOTAL_NUM] = { "DOWN", "UP", "RIGHT", "LEFT" };

static char g_yuv_file[MAX_FILENAME_SIZE];
static int g_yuv_width;
static int g_yuv_height;
static int verbose = 0;
static int g_y_threshold_id = 0;
static int g_y_threshold[][2] = {
        { 80, 120 },
        { 100, 150 },
        { 120, 180 }
};

#define MAX_ARC_NUM   4
static arc_t g_arc_table[MAX_ARC_NUM];

#define NO_ARG			0
#define HAS_ARG			1
static struct option long_options[] = {
        { "filename", HAS_ARG, 0, 'f' },
        { "width", HAS_ARG, 0, 'w' },
        { "height", HAS_ARG, 0, 'h' },
        { "threshold", HAS_ARG, 0, 't'},
        { "verbose", NO_ARG, 0, 'v' },
        { 0, 0, 0, 0 }
};

struct hint_s {
	const char *arg;
	const char *str;
};

static const char *short_options = "f:w:h:t:v";

static const struct hint_s hint[] = {
        { "", "\t\tYUV file name" },
        { "", "\t\tYUV width" },
        { "", "\t\tYUV height" },
        { "0~2", "\t\tY threshold to distinguist circle from background. Default is 0." },
        { "", "\t\t print more info"},
};

static void usage(void)
{
	int i;

	printf("\ncali_fish_center usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
}

static int init_param(int argc,
                      char** argv)
{
	int ch;
	int option_index = 0;
	opterr = 0;

	if (argc <= 1) {
		usage();
		return -1;
	}

	while ((ch =
	    getopt_long(argc, argv, short_options, long_options, &option_index))
	    != -1) {
		switch (ch) {
			case 'f':
				snprintf(g_yuv_file, sizeof(g_yuv_file), "%s", optarg);
				break;
			case 'w':
				g_yuv_width = atoi(optarg);
				break;
			case 'h':
				g_yuv_height = atoi(optarg);
				break;
			case 't':
				g_y_threshold_id = atoi(optarg);
				if (g_y_threshold_id < 0 ||
				    g_y_threshold_id
				        >= sizeof(g_y_threshold) / sizeof(g_y_threshold[0])) {
					printf("threshold must be in the range [0, %d-1].\n",
					    sizeof(g_y_threshold) / sizeof(g_y_threshold[0]));
					return -1;
				}
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				printf("Unknown option %d.\n", ch);
				return -1;
		}
	}
	if (g_yuv_width <= 0 || g_yuv_height <= 0) {
		printf("YUV width [%d] and height [%d] must be greater than 0.\n",
		    g_yuv_width, g_yuv_height);
		return -1;
	}
	return 0;
}

static inline int is_in_circle(u8 y)
{
	return y >= g_y_threshold[g_y_threshold_id][0];
}

static inline int is_out_circle(u8 y)
{
	return y < g_y_threshold[g_y_threshold_id][1];
}

static u8* get_y_data(void)
{
	int fd = -1, nbytes = 0;
	u8* buffer = NULL;
	if ((fd = open(g_yuv_file, O_RDONLY)) < 0) {
		printf("cannot open %s.\n", g_yuv_file);
		perror("open");
	} else {
		nbytes = g_yuv_width * g_yuv_height;
		buffer = (u8 *) malloc(nbytes * sizeof(u8));
		if (buffer) {
			if (read(fd, buffer, nbytes) != nbytes) {
				printf("YUV file mismatches %dx%d.\n", g_yuv_width,
				    g_yuv_height);
				free(buffer);
				buffer = NULL;
			}
		} else {
			perror("malloc");
		}
		close(fd);
	}
	return buffer;
}

//debug only
//static void print_y_data(u8* buffer)
//{
//	int i, j;
//	u8* p = buffer;
//	for (i = 0; i < g_yuv_height; ++i) {
//		for (j = 0; j < g_yuv_width; ++j) {
//			printf("%3d ", *(p++));
//		}
//		printf("\n");
//	}
//	printf("\n\n");
//}

static inline int find_fringe_one_line(const u8* data,
                                       const int pos,
                                       const DIR scan_dir)
{
	int been_out_run = 0, in_run = 0;
	int range, step, offset, i;
	u8 val;
	switch (scan_dir) {
		case UP:
			range = g_yuv_height;
			step = -g_yuv_width;
			offset = (g_yuv_height - 1) * g_yuv_width + pos;
			break;
		case DOWN:
			range = g_yuv_height;
			step = g_yuv_width;
			offset = pos;
			break;
		case LEFT:
			range = g_yuv_width;
			step = -1;
			offset = g_yuv_width * (pos + 1) - 1;
			break;
		case RIGHT:
			range = g_yuv_width;
			step = 1;
			offset = g_yuv_width * pos;
			break;
		default:
			return -1;
	}
	for (i = 0; i < range; i++) {
		val = *(data + offset);
		if (is_in_circle(val)) {
			if (been_out_run < MIN_CLUSTER) {
				return -1;
			}
			if (in_run++ < MIN_CLUSTER) {
				offset += step;
				continue;
			}
			//SUCCESS
			switch (scan_dir) {
				case DOWN:
					case RIGHT:
					return i;
				case UP:
					case LEFT:
					return (range - 1 - i);
				default:
					return -1;
			}
		} else if (is_out_circle(val)) {
			been_out_run++;
		}
		offset += step;
	}
	return -1;
}

static inline int new_peak(int pos,
                           int old_peak,
                           DIR scan_dir)
{
	switch (scan_dir) {
		case DOWN:
			case RIGHT:
			if (pos < old_peak || old_peak == -1) {
				return -1;
			} else {
				return 0;
			}
			break;
		case LEFT:
			case UP:
			if (pos > old_peak || old_peak == -1) {
				return 1;
			} else {
				return 0;
			}
			break;
		default:
			break;
	}
	return 0;

}

static int find_two_arcs(const u8* data,
                         arc_t* arc1,
                         arc_t* arc2,
                         DIR from)
{
	int start, step, scan_dir, range, cur, pos = 0;
	int fringe_num = 0, i = 0, prev = -1, prev_pos = -1;
	int is_fringe = 0, peak = -1, co_peak = 0;
	arc_t* p = NULL;
	CHANGE_STATE state = NO_CHANGE;
	switch (from) {
		case UP:
			start = 0;
			scan_dir = DOWN;
			step = 1;
			range = g_yuv_width;
			break;
		case DOWN:
			start = g_yuv_width - 1;
			scan_dir = UP;
			step = -1;
			range = g_yuv_width;
			break;
		case LEFT:
			start = g_yuv_height - 1;
			scan_dir = RIGHT;
			step = -1;
			range = g_yuv_height;
			break;
		case RIGHT:
			start = 0;
			scan_dir = LEFT;
			step = 1;
			range = g_yuv_height;
			break;
		default:
			return 0;
	}
	cur = start;
	while (i <= range) {
		if (i < range) {
			pos = find_fringe_one_line(data, cur, scan_dir);
			if (pos < 0 && is_fringe) {
				is_fringe = 0;
				// old fringe end
				state = FRINGE_END;
			} else if (pos >= 0) {
				if (new_peak(pos, peak, scan_dir)) {
					peak = pos;
					co_peak = cur;
				}
				if (!is_fringe) {
					// new fringe begin
					is_fringe = 1;
					fringe_num++;
					state = FRINGE_BEGIN;
				}
			}
		} else {
			if (is_fringe) {
				state = FRINGE_END;
			}
		}

		if (state != NO_CHANGE) {
			DEBUG("%s\n",
			    state == FRINGE_BEGIN ? "new finege!" : "end of fringe!");
			if (fringe_num == 1) {
				p = arc1;
			} else if (fringe_num == 2) {
				p = arc2;
			} else {
				return 0; /*error*/
			}
			switch (scan_dir) {
				case UP:
					case DOWN:
					switch (state) {
						case FRINGE_BEGIN:
							p->begin.x = cur;
							p->begin.y = pos;
							break;
						case FRINGE_END:
							p->end.x = prev;
							p->end.y = prev_pos;
							break;
						default:
							return 0; /*error*/
					}
					break;
				case LEFT:
					case RIGHT:
					switch (state) {
						case FRINGE_BEGIN:
							p->begin.x = pos;
							p->begin.y = cur;
							break;
						case FRINGE_END:
							p->end.x = prev_pos;
							p->end.y = prev;
							break;
						default:
							return 0; /*error*/
					}
					break;
				default:
					return 0; /*error*/
			}
			state = NO_CHANGE; /*restore flag*/
		}
		if (i == range) {
			break;
		}
		cur += step;
		prev = cur;
		prev_pos = pos;
		i++;
	}
	if (fringe_num <= 0 || fringe_num > 2 || (fringe_num == 1 && peak < 0)) {
		return 0;
	}
	if (fringe_num == 1) {
		DEBUG("only one fringe! peak=%d, co_peak=%d\n", peak, co_peak);
		DEBUG("arc1.begin=(%d, %d), arc1.end=(%d, %d)\n",
		    arc1->begin.x, arc1->begin.y, arc1->end.x, arc1->end.y);
		switch (scan_dir) {
			case UP:
				case DOWN:
				if (arc1->end.x == co_peak && arc1->end.y == peak) {
					return 1; // only 1 arc found
				}
				arc2->end = arc1->end;
				arc1->end.x = co_peak;
				arc1->end.y = peak;
				break;
			case LEFT:
				case RIGHT:
				if (arc1->end.y == co_peak && arc1->end.x == peak) {
					return 1; // only 1 arc found
				}
				arc2->end = arc1->end;
				arc1->end.x = peak;
				arc1->end.y = co_peak;
				break;
			default:
				return 0; //error
		}
		arc2->begin = arc1->end;
		return 2; // divide 1 fringe to 2 arcs
	}
	return 2; // 2 fringe as per 2 arcs
}

static inline int find_third_point(const u8* data,
                                   axis_t* p1,
                                   axis_t* p2,
                                   axis_t* p3)
{
	DIR scan_dir;
	int pos = 0;
	if (ABS(p1->x - p2->x) >= ABS(p1->y - p2->y)) {
		pos = (p1->x + p2->x) / 2;
		scan_dir = (p1->x < p2->x) ? DOWN : UP;
		p3->y = find_fringe_one_line(data, pos, scan_dir);
		if (p3->y < 0) {
			return -1;
		}
		p3->x = pos;
	} else {
		pos = (p1->y + p2->y) / 2;
		scan_dir = (p1->x < p2->x) ? RIGHT : LEFT;
		find_fringe_one_line(data, pos, scan_dir);
		p3->x = find_fringe_one_line(data, pos, scan_dir);
		if (p3->x < 0) {
			return -1;
		}
		p3->y = pos;
	}
	return 0;
}

//debug use only
static void print_arcs(int num)
{
	int i;
	arc_t arc;
	if (num == 0) {
		printf("No arc found!\n");
		return;
	}
	printf("Show %d arcs found: ", num);
	for (i = 0; i < num; i++) {
		arc = g_arc_table[i];
		printf("(%d, %d)->(%d, %d)    ",
		    arc.begin.x, arc.begin.y, arc.end.x, arc.end.y);
	}
	printf("\n");
	return;
}


static void print_point(const char* str, axis_t* point)
{
	INFO("%s (%d, %d)\n", str, point->x, point->y);
}

static int find_three_points(const u8* data,
                             axis_t* p1,
                             axis_t* p2,
                             axis_t* p3)
{
	int num_arc_found = 0;

	num_arc_found += find_two_arcs(data, &g_arc_table[num_arc_found],
	    &g_arc_table[num_arc_found + 1], LEFT);

	if (verbose) {
		print_arcs(num_arc_found);
	}

	num_arc_found += find_two_arcs(data, &g_arc_table[num_arc_found],
	    &g_arc_table[num_arc_found + 1], RIGHT);

	if (verbose) {
		print_arcs(num_arc_found);
	}

	switch (num_arc_found) {
		case 1:
			*p1 = g_arc_table[0].begin;
			*p2 = g_arc_table[0].end;
			if (find_third_point(data, p1, p2, p3) != 0) {
				return -1;
			}
			break;
		case 2:
			*p1 = g_arc_table[0].begin;
			*p2 = g_arc_table[0].end;
			if (MAX(ABS(p2->x - g_arc_table[1].begin.x),
				ABS(p2->y - g_arc_table[1].begin.y))
			    > MAX(ABS(p2->x - g_arc_table[1].end.x),
				    ABS(p2->y - g_arc_table[1].end.y))) {
				*p3 = g_arc_table[1].begin;
			} else {
				*p3 = g_arc_table[1].end;
			}
			break;
		case 3:
			case 4:
			*p1 = g_arc_table[0].begin;
			*p2 = g_arc_table[1].begin;
			*p3 = g_arc_table[2].begin;
			break;
		default:
			return -1;
	}

	if (verbose) {
		print_point("Point 1 is", p1);
		print_point("Point 2 is", p2);
		print_point("Point 3 is", p3);
	}
	return 0;

}

static void calc_center(axis_t* center,
                        int* radius,
                        axis_t* p1,
                        axis_t* p2,
                        axis_t* p3)
{
	s64 y3_minus_y1 = p3->y - p1->y;
	s64 y2_minus_y1 = p2->y - p1->y;
	s64 x3_minus_x1 = p3->x - p1->x;
	s64 x2_minus_x1 = p2->x - p1->x;
	s64 sqrt_x1 = p1->x * p1->x;
	s64 sqrt_x2 = p2->x * p2->x;
	s64 sqrt_x3 = p3->x * p3->x;
	s64 sqrt_y1 = p1->y * p1->y;
	s64 sqrt_y2 = p2->y * p2->y;
	s64 sqrt_y3 = p3->y * p3->y;
	s64 tmp = x2_minus_x1 * y3_minus_y1 - x3_minus_x1 * y2_minus_y1;
	s64 tmp1 = sqrt_x2 + sqrt_y2 - sqrt_x1 - sqrt_y1;
	s64 tmp2 = sqrt_x3 + sqrt_y3 - sqrt_x1 - sqrt_y1;

	if (tmp == 0) {
		return;
	}

	center->x = (tmp1 * y3_minus_y1 - tmp2 * y2_minus_y1) / tmp / 2;
	center->y = (tmp2 * x2_minus_x1 - tmp1 * x3_minus_x1) / tmp / 2;
	*radius = sqrt(sqrt_x1 + center->x * center->x - 2 * center->x * p1->x +
		sqrt_y1 + center->y * center->y - 2 * center->y * p1->y);
}

int main(int argc,
         char** argv)
{
	u8* y_data;
	axis_t p[3] = {{0, 0}}, center = {0};
	int radius = 0;

	if (init_param(argc, argv) < 0) {
		return -1;
	}
	if ((y_data = get_y_data()) == NULL) {
		return -1;
	}
//	print_y_data(y_data);

	find_three_points(y_data, &p[0], &p[1], &p[2]);
	free(y_data);

	calc_center(&center, &radius, &p[0], &p[1], &p[2]);
	print_point("Center is", &center);
	INFO("Radius is %d.\n", radius);
	return 0;

}
