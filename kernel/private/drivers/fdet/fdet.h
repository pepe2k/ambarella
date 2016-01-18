/*
 * ambhw/fdet.h
 *
 * History:
 *	2012/06/28 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 */

#ifndef __FDET_H__
#define __FDET_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>
#include <ambas_fdet.h>

#define FDET_BASE_ADDRESS_OFFSET	0x00
#define FDET_ENABLE_OFFSET		0x04
#define FDET_GO_OFFSET			0x08
#define FDET_CONFIG_DONE_OFFSET		0x0C
#define FDET_RESET_OFFSET		0x10
#define FDET_INPUT_WIDTH_OFFSET		0x14
#define FDET_INPUT_HEIGHT_OFFSET	0x18

#if (CHIP_REV == A7)
#define FDET_DEADLINE_OFFSET		0x1C
#define FDET_SKIP_FIRST_INT_OFFSET	0x20
#define FDET_FS_CMD_LIST_PTR_OFFSET	0x24
#define FDET_FS_CMD_LIST_SIZE_OFFSET	0x28
#define FDET_FS_RESULT_BUF_PTR_OFFSET	0x2C
#define FDET_FS_RESULT_BUF_SIZE_OFFSET	0x30
#define FDET_TS_CMD_LIST_PTR_OFFSET	0x34
#define FDET_TS_CMD_LIST_SIZE_OFFSET	0x38
#define FDET_TS_RESULT_BUF_PTR_OFFSET	0x3C
#define FDET_TS_RESULT_BUF_SIZE_OFFSET	0x40
#define FDET_ORIG_TARGET_PTR_OFFSET	0x44
#define FDET_ORIG_TARGET_PITCH_OFFSET	0x48
#define FDET_TMP_TARGET_PTR_OFFSET	0x4C
#define FDET_SEND_DEADLINE_INT_OFFSET	0x50
#else
#define FDET_DEADLINE_OFFSET		0x20
#define FDET_SKIP_FIRST_INT_OFFSET	0x1C
#define FDET_SEND_DEADLINE_INT_OFFSET	0x24
#define FDET_FS_CMD_LIST_PTR_OFFSET	0x28
#define FDET_FS_CMD_LIST_SIZE_OFFSET	0x2C
#define FDET_FS_RESULT_BUF_PTR_OFFSET	0x30
#define FDET_FS_RESULT_BUF_SIZE_OFFSET	0x34
#define FDET_TS_CMD_LIST_PTR_OFFSET	0x38
#define FDET_TS_CMD_LIST_SIZE_OFFSET	0x3C
#define FDET_TS_RESULT_BUF_PTR_OFFSET	0x40
#define FDET_TS_RESULT_BUF_SIZE_OFFSET	0x44
#define FDET_ORIG_TARGET_PTR_OFFSET	0x50
#define FDET_ORIG_TARGET_PITCH_OFFSET	0x54
#define FDET_TMP_TARGET_PTR_OFFSET	0x58
#endif

#define FDET_RESULT_STATUS_OFFSET	0x100
#define FDET_ERROR_STATUS_OFFSET	0x104
#define FDET_FS_CMD_STATUS_OFFSET	0x108
#define FDET_TS_CMD_STATUS_OFFSET	0x10C
#define FDET_ACTIVE_OFFSET		0x110

#define FDET_SCALE_FACTOR0_OFFSET	0x400
#define FDET_SCALE_FACTOR_OFFSET(n)	(FDET_SCALE_FACTOR0_OFFSET + ((n) << 2))

#if (CHIP_REV == A7)

#define FDET_EVALUATION_ID0_OFFSET	0x800
#define FDET_EVALUATION_ID_OFFSET(n)	(FDET_EVALUATION_ID0_OFFSET + ((n) << 2))

#else

#define FDET_EVALUATION_ID0_OFFSET	0x500
#define FDET_EVALUATION_ID_OFFSET(n)	(FDET_EVALUATION_ID0_OFFSET + ((n) << 2))

#define FDET_EVALUATION_NUM0_OFFSET	0x600
#define FDET_EVALUATION_NUM_OFFSET(n)	(FDET_EVALUATION_NUM0_OFFSET + ((n) << 2))

#define FDET_EVALUATION_NUM_REG(n)	FACE_DETECTION_REG(FDET_EVALUATION_NUM_OFFSET(n))

#define FDET_HAVE_EVAL_NUM_REGS

#endif

#if (CHIP_REV == A7)
#define FDET_IRQF			IRQF_TRIGGER_FALLING
#else
#define FDET_IRQF			IRQF_TRIGGER_RISING
#endif

#ifndef FDET_IRQ
#define FDET_IRQ			FACE_DET_IRQ
#endif

#define FDET_BASE_ADDRESS_REG		FACE_DETECTION_REG(FDET_BASE_ADDRESS_OFFSET)
#define FDET_ENABLE_REG			FACE_DETECTION_REG(FDET_ENABLE_OFFSET)
#define FDET_GO_REG			FACE_DETECTION_REG(FDET_GO_OFFSET)
#define FDET_CONFIG_DONE_REG		FACE_DETECTION_REG(FDET_CONFIG_DONE_OFFSET)
#define FDET_RESET_REG			FACE_DETECTION_REG(FDET_RESET_OFFSET)
#define FDET_INPUT_WIDTH_REG		FACE_DETECTION_REG(FDET_INPUT_WIDTH_OFFSET)
#define FDET_INPUT_HEIGHT_REG		FACE_DETECTION_REG(FDET_INPUT_HEIGHT_OFFSET)
#define FDET_DEADLINE_REG		FACE_DETECTION_REG(FDET_DEADLINE_OFFSET)
#define FDET_SEND_DEADLINE_INT_REG	FACE_DETECTION_REG(FDET_SEND_DEADLINE_INT_OFFSET)
#define FDET_SKIP_FIRST_INT_REG		FACE_DETECTION_REG(FDET_SKIP_FIRST_INT_OFFSET)
#define FDET_FS_CMD_LIST_PTR_REG	FACE_DETECTION_REG(FDET_FS_CMD_LIST_PTR_OFFSET)
#define FDET_FS_CMD_LIST_SIZE_REG	FACE_DETECTION_REG(FDET_FS_CMD_LIST_SIZE_OFFSET)
#define FDET_FS_RESULT_BUF_PTR_REG	FACE_DETECTION_REG(FDET_FS_RESULT_BUF_PTR_OFFSET)
#define FDET_FS_RESULT_BUF_SIZE_REG	FACE_DETECTION_REG(FDET_FS_RESULT_BUF_SIZE_OFFSET)
#define FDET_TS_CMD_LIST_PTR_REG	FACE_DETECTION_REG(FDET_TS_CMD_LIST_PTR_OFFSET)
#define FDET_TS_CMD_LIST_SIZE_REG	FACE_DETECTION_REG(FDET_TS_CMD_LIST_SIZE_OFFSET)
#define FDET_TS_RESULT_BUF_PTR_REG	FACE_DETECTION_REG(FDET_TS_RESULT_BUF_PTR_OFFSET)
#define FDET_TS_RESULT_BUF_SIZE_REG	FACE_DETECTION_REG(FDET_TS_RESULT_BUF_SIZE_OFFSET)
#define FDET_ORIG_TARGET_PTR_REG	FACE_DETECTION_REG(FDET_ORIG_TARGET_PTR_OFFSET)
#define FDET_ORIG_TARGET_PITCH_REG	FACE_DETECTION_REG(FDET_ORIG_TARGET_PITCH_OFFSET)
#define FDET_TMP_TARGET_PTR_REG		FACE_DETECTION_REG(FDET_TMP_TARGET_PTR_OFFSET)

#define FDET_RESULT_STATUS_REG		FACE_DETECTION_REG(FDET_RESULT_STATUS_OFFSET)
#define FDET_ERROR_STATUS_REG		FACE_DETECTION_REG(FDET_ERROR_STATUS_OFFSET)
#define FDET_FS_CMD_STATUS_REG		FACE_DETECTION_REG(FDET_FS_CMD_STATUS_OFFSET)
#define FDET_TS_CMD_STATUS_REG		FACE_DETECTION_REG(FDET_TS_CMD_STATUS_OFFSET)
#define FDET_ACTIVE_REG			FACE_DETECTION_REG(FDET_ACTIVE_OFFSET)

#define FDET_SCALE_FACTOR_REG(n)	FACE_DETECTION_REG(FDET_SCALE_FACTOR_OFFSET(n))

#define FDET_EVALUATION_ID_REG(n)	FACE_DETECTION_REG(FDET_EVALUATION_ID_OFFSET(n))



#define FDET_CONFIG_BASE_NORMAL		0x0C

#define FDET_ENABLE			0x01
#define FDET_DISABLE			0x00

#define FDET_START			0x01
#define FDET_STOP			0x00

#define FDET_CONFIG_DONE		0x01

#define FDET_RESET			0x01

#define FDET_SKIP_FIRST_INTERRUPT	0x01

#define FDET_ACTIVE			0x01

#define RIGHT_SHIFT_16_ROUND(x)		(((x) + (1 << 15)) >> 16)

#define FDET_DEBUG(format, arg...)	\
	do {	\
		if (pinfo->config.policy & FDET_POLICY_DEBUG) {	\
			printk(format , ## arg);	\
		}	\
	} while (0)

#define FDET_MAX_FACES			32

#define FDET_MAX_INPUT_WIDTH		1024
#define FDET_MAX_INPUT_HEIGHT		1024
#define FDET_TEMPLATE_SIZE		20

#define FDET_MAX_FACE_CLASSIFIERS	24
#define FDET_MAX_ORIENTATIONS		8

#define FDET_SCALED_PARTITION_SIZE	40

#define FDET_NUM_EVALS			1
#define FDET_ORIENTATION_BITMASK	0x01

#define FDET_MAX_UNMERGED_FACES		2048
#define FDET_MAX_MERGED_FACES		128

#define FDET_FS_CMD_BUF_SIZE		(8 * 1024)
#define FDET_FS_RESULT_BUF_SIZE		(8 * 1024)
#define FDET_TS_CMD_BUF_SIZE		(4 * 1024)
#define FDET_TS_RESULT_BUF_SIZE		(64 * 1024)

#define FDET_FS_CMD_BUF_WORDS		(FDET_FS_CMD_BUF_SIZE >> 2)
#define FDET_FS_RESULT_BUF_WORDS	(FDET_FS_RESULT_BUF_SIZE >> 2)
#define FDET_TS_CMD_BUF_WORDS		(FDET_TS_CMD_BUF_SIZE >> 2)
#define FDET_TS_RESULT_BUF_WORDS	(FDET_TS_RESULT_BUF_SIZE >> 2)

#define FDET_ORIG_TARGET_BUF_SIZE	(FDET_MAX_INPUT_WIDTH * FDET_MAX_INPUT_HEIGHT)
#define FDET_TMP_TARGET_BUF_SIZE	(FDET_MAX_INPUT_WIDTH * FDET_MAX_INPUT_HEIGHT)
#define FDET_CLASSIFIER_BINARY_SIZE	(128 * 1024)

#define FDET_VM_DELAY			(300)

#define FDET_NAME			"fdet"
#define FDET_MAJOR			248
#define FDET_MINOR			148

typedef enum {
	OPCODE_CLASSIFIER_LOAD		= 0,
	OPCODE_SEARCH			= 1,
} opcode_t;

typedef enum {
	RESULT_TYPE_UNMERGED_FACES	= 0,
	RESULT_TYPE_MERGED_FACES	= 1,
	RESULT_TYPE_COMMAND_STATUS	= 2,
} result_t;

typedef union {
	unsigned int	w;
	struct {
		unsigned int	exponent		: 3;	/* [2:0] */
		unsigned int	mantissa		: 11;	/* [13:3] */
		unsigned int	reciprocal_exponent	: 3;	/* [16:14] */
		unsigned int	reciprocal_mantissa	: 11;	/* [27:17] */
		unsigned int	reserved		: 4;	/* [31:18] */
	} s;
} fdet_scale_factor_reg_t;

typedef struct {
	unsigned int		op_code			: 2;	/* [1:0] */
	unsigned int		classifier_addr		: 13;	/* [14:2] */
	unsigned int		reserved1		: 2;	/* [16:15] */
	unsigned int		length			: 12;	/* [28:17] */
	unsigned int		reserved2		: 3;	/* [31:29] */

	unsigned int		dram_addr;
} fdet_classifier_load_cmd_t;

typedef struct {
	unsigned int		op_code			: 2;	/* [1:0] */
	unsigned int		cmd_id			: 10;	/* [11:2] */
	unsigned int		center_x		: 10;	/* [21:12] */
	unsigned int		center_y		: 10;	/* [31:22] */

	unsigned int		num_eval		: 2;	/* [1:0] */
	unsigned int		merge			: 1;	/* [2:2] */
	unsigned int		radius_format		: 1;	/* [3:3] */
	unsigned int		si_start		: 5;	/* [8:4] */
	unsigned int		num_si			: 5;	/* [13:9] */
	unsigned int		radius_x		: 9;	/* [22:14] */
	unsigned int		radius_y		: 9;	/* [31:23] */

	unsigned int		bit_mask0		: 8;	/* [7:0] */
	unsigned int		eval_id0		: 5;	/* [12:8] */
	unsigned int		reserved0		: 3;	/* [15:13] */

	unsigned int		bit_mask1		: 8;	/* [23:16] */
	unsigned int		eval_id1		: 5;	/* [28:24] */
	unsigned int		reserved1		: 3;	/* [31:29] */

	unsigned int		bit_mask2		: 8;	/* [7:0] */
	unsigned int		eval_id2		: 5;	/* [12:8] */
	unsigned int		reserved2		: 3;	/* [15:13] */

	unsigned int		bit_mask3		: 8;	/* [23:16] */
	unsigned int		eval_id3		: 5;	/* [28:24] */
	unsigned int		reserved3		: 3;	/* [31:29] */
} fdet_search_cmd_t;

typedef union {
	unsigned int		w;
	struct {
		unsigned int		ts_num		: 16;	/* [15:0] */
		unsigned int		fs_num		: 16;	/* [31:16] */
	} s;
} fdet_result_num_reg_t;

typedef union {
	unsigned int		w;
	struct {
		unsigned int	ts_result_overflow	: 1;	/* [0] */
		unsigned int	ts_merge_overflow	: 1;	/* [1] */
		unsigned int	ts_too_long		: 1;	/* [2] */
		unsigned int	reserved1		: 5;	/* [7:3] */
		unsigned int	fs_result_overflow	: 1;	/* [8] */
		unsigned int	fs_merge_overflow	: 1;	/* [9] */
		unsigned int	reserved2		: 22;	/* [31:10] */
	} s;
} fdet_error_status_reg_t;

typedef struct {
	unsigned int		type			: 2;	/* [1:0] */
	unsigned int		cmd_id			: 10;	/* [11:2] */
	unsigned int		x			: 10;	/* [21:12] */
	unsigned int		y			: 10;	/* [31:22] */

	unsigned int		eval_id			: 5;	/* [4:0] */
	unsigned int		reserved0		: 3;	/* [7:5] */
	unsigned int		si			: 5;	/* [12:8] */
	unsigned int		oi			: 3;	/* [15:13] */
	unsigned int		reserved1		: 16;	/* [31:16] */
} fdet_result_t;

enum fdet_vm_state {
	FDET_VM_STATE_IDLE	= 0x0,
	FDET_VM_STATE_READY,
	FDET_VM_STATE_RUNNING,
};

typedef struct {
	unsigned short		offset;
	unsigned short		sz;
	unsigned short		left_offset;
	unsigned short		top_offset;
	unsigned char		orientation_bitmasks;
	unsigned char		orientation_result[FDET_MAX_ORIENTATIONS];
	unsigned char		reserved;
} fdet_classifier_t;

typedef struct {
	unsigned int			base;

	unsigned int			num_fs_cls;
	unsigned int			num_ts_cls;

	fdet_classifier_t		fs_cls[FDET_MAX_FACE_CLASSIFIERS];
	fdet_classifier_t		ts_cls[FDET_MAX_FACE_CLASSIFIERS];
} fdet_classifier_binary_t;

struct fdet_unmerged_face {
	unsigned int			x;
	unsigned int			y;
	unsigned int			si;
	unsigned int			sz;
	int				cluster;
	int				oi;
};

struct fdet_merged_face {
	unsigned int			x;
	unsigned int			y;
	unsigned int			sz;
	unsigned int			num_si[32];
	unsigned int			hit_count[8];
	unsigned int			hit_sum;
	unsigned int			best_oi;
	unsigned int			best_hitcount;
	enum fdet_result_type		type;
};

typedef struct {
	int				num_evals;
	unsigned char			bm[3];
	unsigned char			id[3];
} fdet_om_t;

struct amba_fdet_info {
	char					fs_cmd_buf[2][FDET_FS_CMD_BUF_SIZE];
	char					fs_result_buf[2][FDET_FS_RESULT_BUF_SIZE];
	char					ts_cmd_buf[2][FDET_TS_CMD_BUF_SIZE];
	char					ts_result_buf[2][FDET_TS_RESULT_BUF_SIZE];
	int					current_fs_buf_id;
	int					current_fs_result_id;
	int					current_ts_buf_id;
	int					current_ts_result_id;
	int					current_tmp_target_id;
	int					current_fs_buf_sz;
	int					current_ts_buf_sz;
	int					last_fs_cmd_id;
	enum fdet_vm_state			vm_state;

	char					*orig_target_buf;
	char					*tmp_target_buf;
	char					*classifier_binary;
	int					orig_len;
	int					cls_bin_len;
	enum fdet_mmap_type			mmap_type;

	struct fdet_configuration		config;

	int					num_scales;
	int					num_sub_scales;
	int					num_total_scales;
	unsigned int				scale_factor[32];
	unsigned int				recip_scale_factor[32];
	fdet_scale_factor_reg_t			scale_factor_regs[32];

	fdet_classifier_binary_t		binary_info;
	unsigned int				eval_id[32];
	unsigned int				eval_num[32];
	fdet_om_t				om;
	unsigned int				min_hitcounts[8];

	struct fdet_unmerged_face		unmerged_faces[FDET_MAX_UNMERGED_FACES];
	struct fdet_unmerged_face		ts_unmerged_faces[FDET_MAX_UNMERGED_FACES];
	struct fdet_merged_face			merged_faces[FDET_MAX_MERGED_FACES];
	struct fdet_merged_face			latest_faces[FDET_MAX_MERGED_FACES];
	int					num_faces;
	int					latest_faces_num;
	unsigned int				fs_found_faces[2];

	int					irq;
	struct cdev				char_dev;

	struct timer_list			timer;
	struct completion			result_completion;

	unsigned long				fs_tick;
	unsigned long				ts_tick;
};

struct fdet_adjacent_update_info {
	unsigned char				eval_id;
	unsigned char				orientation_bitmask;
};
struct fdet_orientation_info {
	unsigned char				eval_id;
	unsigned char				orientation_mode;
	unsigned char				adjacent_num_evals;
	struct fdet_adjacent_update_info	adjacent_update_info[FDET_MAX_ORIENTATIONS];
};

/* ========================================================================== */
const int search_radius[32] = {
	5, 3, 2, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
};

struct fdet_orientation_info fdet_orientation_table[FDET_MAX_ORIENTATIONS] = {
	{0, 0, 3, {{0,0x01}, {1,0x03}, {2,0x03}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{1, 0, 2, {{0,0x01}, {1,0x01}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{1, 1, 2, {{0,0x01}, {1,0x02}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 0, 2, {{0,0x01}, {2,0x09}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 1, 2, {{0,0x01}, {2,0x22}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 3, 1, {{2,0x09}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{2, 5, 1, {{2,0x22}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
	{0, 0, 0, {{0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}, {0,0x00}} },
};

int fdet_get_result_still(void);
int fdet_get_result_video_fs(void);
int fdet_get_result_video_ts(void);
void fdet_print_faces(void);
void fdet_config(void);

#endif
