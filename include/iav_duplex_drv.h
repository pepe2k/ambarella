
#ifndef __IAV_DUPLEX_DRV_H__
#define __IAV_DUPLEX_DRV_H__

/*
 * duplex APIs
 */

enum {
	// mode
	IOC_DUPLEX_ENTER_MODE = 0x00,
	IOC_DUPLEX_LEAVE_MODE = 0x01,

	//display mode
	IOC_DUPLEX_DISPLAY_MODE = 0x02,

	// vcap
	IOC_DUPLEX_CONFIG_VCAP = 0x03,
	IOC_DUPLEX_START_VCAP = 0x04,
	IOC_DUPLEX_STOP_VCAP = 0x05,

	// encoding
	IOC_DUPLEX_INIT_ENCODER = 0x06,
	IOC_DUPLEX_RELEASE_ENCODER = 0x07,

	IOC_DUPLEX_START_ENCODER = 0x08,
	IOC_DUPLEX_STOP_ENCODER = 0x09,

	IOC_DUPLEX_READ_BITS = 0x0a,
	IOC_DUPLEX_CONFIG_GETTING_RAWDATA = 0x0b,
	IOC_DUPLEX_GET_PREVIEW_BUFFER = 0x0c,

	//update on the fly
	IOC_DUPLEX_UPDATE_BITRATE = 0x0d,
	IOC_DUPLEX_UPDATE_FRAMERATE = 0x0e,
	IOC_DUPLEX_UPDATE_GOP_STRUCTURE = 0x0f,

	IOC_DUPLEX_DEMAND_IDR = 0x10,

	IOC_DUPLEX_UPDATE_DEC_VOUT_CONFIG = 0x11,
	IOC_DUPLEX_UPDATE_PREVIEW_VOUT_CONFIG = 0x12,

	// decoding
	IOC_DUPLEX_INIT_DECODER = 0x13,
	IOC_DUPLEX_RELEASE_DECODER = 0x14,

	IOC_DUPLEX_DECODE = 0x15,
	IOC_DUPLEX_SET_PB_SPEED = 0x16,
	IOC_DUPLEX_DUPLEX_TRICK_PLAY = 0x17,

	IOC_DUPLEX_STOP_DECODER = 0x18,
	IOC_DUPLEX_WAIT_DECODER = 0x19,
	IOC_DUPLEX_GET_DECODER_STATUS = 0x20,
};

// mode
#define IAV_IOC_DUPLEX_ENTER_MODE	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_ENTER_MODE, struct iav_duplex_mode_s *)
#define IAV_IOC_DUPLEX_LEAVE_MODE	_IO(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_LEAVE_MODE)

//display mode
#define IAV_IOC_DUPLEX_DISPLAY_MODE	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_DISPLAY_MODE, struct iav_duplex_display_mode_s *)

// vcap
#define IAV_IOC_DUPLEX_CONFIG_VCAP	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_CONFIG_VCAP, struct iav_duplex_config_vcap_s *)
#define IAV_IOC_DUPLEX_START_VCAP	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_START_VCAP, struct iav_duplex_start_vcap_s *)
#define IAV_IOC_DUPLEX_STOP_VCAP	_IO(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_STOP_VCAP)

// encoding
#define IAV_IOC_DUPLEX_INIT_ENCODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_INIT_ENCODER, struct iav_duplex_init_encoder_s *)
#define IAV_IOC_DUPLEX_RELEASE_ENCODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_RELEASE_ENCODER, unsigned int)

#define IAV_IOC_DUPLEX_START_ENCODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_START_ENCODER, struct iav_duplex_start_encoder_s *)
#define IAV_IOC_DUPLEX_STOP_ENCODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_STOP_ENCODER, struct iav_duplex_start_encoder_s*)

#define IAV_IOC_DUPLEX_READ_BITS	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_READ_BITS, struct iav_duplex_bs_info_s *)
#define IAV_IOC_DUPLEX_CONFIG_GETTING_RAWDATA	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_CONFIG_GETTING_RAWDATA, struct iav_duplex_config_getting_rawdata_s *)
#define IAV_IOC_DUPLEX_GET_PREVIEW_BUFFER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_GET_PREVIEW_BUFFER, struct iav_prev_buf_s *)

//encoding on the fly
#define IAV_IOC_DUPLEX_UPDATE_BITRATE	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_UPDATE_BITRATE, struct iav_duplex_update_bitrate_s*)
#define IAV_IOC_DUPLEX_UPDATE_FRAMERATE	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_UPDATE_FRAMERATE, struct iav_duplex_update_framerate_s*)
#define IAV_IOC_DUPLEX_UPDATE_GOP_STRUCTURE	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_UPDATE_GOP_STRUCTURE, struct iav_duplex_update_gop_s*)

#define IAV_IOC_DUPLEX_DEMAND_IDR	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_DEMAND_IDR, struct iav_duplex_demand_idr_s*)

//vout related
#define IAV_IOC_DUPLEX_UPDATE_DEC_VOUT_CONFIG	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_UPDATE_DEC_VOUT_CONFIG, struct iav_udec_vout_configs_s*)
#define IAV_IOC_DUPLEX_UPDATE_PREVIEW_VOUT_CONFIG	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_UPDATE_PREVIEW_VOUT_CONFIG, struct iav_duplex_update_preview_vout_config_s*)

// decoding
#define IAV_IOC_DUPLEX_INIT_DECODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_INIT_DECODER, struct iav_duplex_init_decoder_s *)
#define IAV_IOC_DUPLEX_RELEASE_DECODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_RELEASE_DECODER, unsigned int)

#define IAV_IOC_DUPLEX_DECODE		_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_DECODE, struct iav_duplex_decode_s *)
#define IAV_IOC_DUPLEX_SET_PB_SPEED	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_SET_PB_SPEED, struct iav_duplex_pb_speed_s *)
#define IAV_IOC_DUPLEX_DUPLEX_TRICK_PLAY	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_DUPLEX_TRICK_PLAY, struct iav_duplex_trick_play_s *)
#define IAV_IOC_DUPLEX_STOP_DECODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_STOP_DECODER, struct iav_duplex_start_decoder_s *)

#define IAV_IOC_DUPLEX_WAIT_DECODER	_IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_WAIT_DECODER, struct iav_wait_decoder_s *)
#define IAV_IOC_DUPLEX_GET_DECODER_STATUS _IOW(IAV_IOC_DUPLEX_MAGIC, IOC_DUPLEX_GET_DECODER_STATUS, struct iav_duplex_get_decoder_status_s *)

enum {
	IAV_FLAG_KEEP_IN_DUPLEX_MODE = 0x1,
};

//blow is from dsp, need sync with dsp api document
enum {
	DSP_STREAM_TYPE_FULL_RESOLUTION = 0x11,
	DSP_STREAM_TYPE_LOW_RESOLUTION = 0x12,
};

//not udec
enum {
	DSP_DECODER_STATE_INVALID = 0,
	DSP_DECODER_STATE_IDLE,
	DSP_DECODER_STATE_RUN,
	DSP_DECODER_STATE_IDLE_LAST_PIC,
	DSP_DECODER_STATE_RUN_2_IDLE,
	DSP_DECODER_STATE_RUN_2_IDLE_LAST_PIC,
};

enum {
	DSP_DECODER_ERROR_LEVEL_NONE = 0,
	DSP_DECODER_ERROR_LEVEL_WARNING,
	DSP_DECODER_ERROR_LEVEL_RECOVERABLE,
	DSP_DECODER_ERROR_LEVEL_FATAL,
};

//blow will expose in dsp_cmd.h?
enum {
	HDEC_FULLCOMP_TYPE	=	(0<<0),
	HDEC_NOFMO_TYPE		=	(1<<0),
	HDEC_IONLY_TYPE		=	(1<<1),
	HDEC_AMBA_TYPE		=	(1<<2),
	HDEC_AU_BOUND_TYPE	=	(1<<3),
	HDEC_LOWDELAY_TYPE	=	(1<<4),
	HDEC_WINDOW_MODE	=	(1<<5),
	HDEC_CODING_ONLY_TYPE	=	(1<<7),//coding only, for diag purpose
};

enum {
	DSP_ENC_STREAM_TYPE_FULL_RESOLUTION = 0x11,
	DSP_ENC_STREAM_TYPE_PIP_RESOLUTION = 0x12,
};

enum {
	DSP_PREVIEW_ID_A = 0,
	DSP_PREVIEW_ID_B,
	DSP_PREVIEW_ID_C,

	DSP_PREVIEW_ID_NUM,
};

enum {
	DSP_PREVIEW_SRC_DRAM = 0,
	DSP_PREVIEW_SRC_SMEM,
	DSP_PREVIEW_SRC_VOUT_THREAD,

	DSP_PREVIEW_SRC_NONE = 0xf,
};

typedef iav_udec_vout_configs_t iav_decoder_display_t;
typedef iav_udec_vout_config_t iav_decoder_vout_config_t;

typedef struct iav_duplex_config_getting_rawdata_s {
	u8 enable;
	u8 use_preview_buffer_id;//preview A, B or C
	u8 reserved0[2];

	u16 scaled_width;
	u16 scaled_height;

	u16 crop_offset_x;
	u16 crop_offset_y;
	u16 crop_width;
	u16 crop_height;
} iav_duplex_config_getting_rawdata_t;

typedef struct iav_duplex_mode_config_s {
	u8 specifided;
	u8 preview_vout_index;
	u8 vout_index;

	u8 pb_display_enabled;
	u8 preview_display_enabled;

	u8 preview_alpha;
	u8 preview_in_pip;
	u8 pb_display_in_pip;

	//main window
	u16 main_width;
	u16 main_height;

	u16 enc_left;
	u16 enc_top;
	u16 enc_width;
	u16 enc_height;

	u8   second_stream_enabled;
	u8   reserved1[3];
	u16 second_enc_width;
	u16 second_enc_height;

	u16 preview_left;
	u16 preview_top;
	u16 preview_width;
	u16 preview_height;

	//pb display window
	u16 pb_display_left;
	u16 pb_display_top;
	u16 pb_display_width;
	u16 pb_display_height;

	//raw data crop window
	iav_duplex_config_getting_rawdata_t rawdata_config[DSP_PREVIEW_ID_NUM];
} iav_duplex_mode_config_t;

// mode
typedef struct iav_duplex_mode_s {
	u32	flags;			// must be 0
	u8	num_of_enc_chans;	// must be 1
	u8	num_of_dec_chans;	// must be 1
	u8	vout_mask;		// bit 0: use VOUT 0; bit 1: use VOUT 1

	iav_duplex_mode_config_t input_config;
} iav_duplex_mode_t;

typedef struct iav_duplex_display_mode_s {
	u8 set;//get or set: 0 get, 1: set

	u8 preview_vout_index;
	u8 vout_index;

	u8 pb_display_enabled;
	u8 preview_display_enabled;

	u8 preview_alpha;
	u8 preview_in_pip;
	u8 pb_display_in_pip;

	u8 change_preview_params;
	u8 change_pb_display_params;
	u8 reserved0[2];

	//preview related
	u16 preview_left;
	u16 preview_top;
	u16 preview_width;
	u16 preview_height;

	//pb display window
	u16 pb_display_left;
	u16 pb_display_top;
	u16 pb_display_width;
	u16 pb_display_height;
} iav_duplex_display_mode_t;

// vcap
typedef struct iav_duplex_config_vcap_s {
	int	dummy;
} iav_duplex_config_vcap_t;

typedef struct iav_duplex_start_vcap_s {
	int	dummy;
} iav_duplex_start_vcap_t;

// encoding

enum {
	IAV_ENC_STATE_IDLE = 0,
	IAV_ENC_STATE_STOPPED,
	IAV_ENC_STATE_STOPPING,//seems ugly, have sent stop cmd to dsp, but still have remaining some frames in driver
	IAV_ENC_STATE_RUNNING,
};

typedef struct iav_duplex_init_encoder_s {
	u32	flags;

	u8	enc_id;
	u8	second_enc_enable;
	u8	profile_idc;
	u8	level_idc;

	u16	encode_w_sz;
	u16	encode_h_sz;
	u16	encode_w_ofs;
	u16	encode_h_ofs;
	u8	num_mbrows_per_bitspart;
	u8	M;
	u8	N;
	u8	idr_interval;
	u8	gop_structure;
	u8	numRef_P;
	u8	numRef_B;
	u8	use_cabac;
	u16	quality_level;

	u8	second_stream_enabled;
	u8	change_main_buffer_size;
	u16	mainbuffer_width;
	u16	mainbuffer_height;

	u16	second_encode_w_sz;
	u16	second_encode_h_sz;
	u16	second_encode_w_ofs;
	u16	second_encode_h_ofs;

	u32	average_bitrate;
	u32	second_average_bitrate;

	u8	vbr_setting;		// 1 - CBR, 2 - Pseudo CBR, 3 - VBR
	u8	calibration;
	u8	vbr_ness;
	u8	min_vbr_rate_factor;	// 0 - 100
	u16	max_vbr_rate_factor;	// 0 - 400

	// out
	u8	*bits_fifo_start;
	u32	bits_fifo_size;
} iav_duplex_init_encoder_t;

typedef struct iav_duplex_start_encoder_s {
	u8	enc_id;
	u8	stream_type;
	u8	reserved[2];
} iav_duplex_start_encoder_t;

typedef struct iav_duplex_start_decoder_s {
	u8	dec_id;
	u8	stop_flag;
	u8	reserved[2];
} iav_duplex_start_decoder_t;

#define DUPLEX_NUM_USER_DESC	4
typedef struct iav_duplex_bs_info_s {
	u8		enc_id;		// in
	u8		count;		// out. valid entries of desc
	u8		more;		// more encoded frames in driver
	u32		reserved;
	bits_info_t	desc[DUPLEX_NUM_USER_DESC];
} iav_duplex_bs_info_t;

//update on the fly
typedef struct iav_duplex_update_bitrate_s {
	u8		enc_id;		// in
	u8		stream_type;	//in
	u16		reserved0;
	u32		average_bitrate;	//in
	u64		pts_to_change_bitrate;//in
} iav_duplex_update_bitrate_t;

typedef struct iav_duplex_update_framerate_s {
	u8		enc_id;		// in
	u8		stream_type;	//in
	u8		framerate_reduction_factor;		//vin frame rate/factor = encodng frame rate
	u8		framerate_code;	//in 0: 29.97 interlaced, 1: 29.97 progressive, 2: 59.94 interlaced, 3: 59.94 progressive, 4-255 integer frame rate
} iav_duplex_update_framerate_t;

typedef struct iav_duplex_update_gop_s {
	u8		enc_id;		// in
	u8		stream_type;	//in

	u8		change_gop_option;//0: not change, 1: change gop, 2: change gop and force first I/P to be IDR
	u8		follow_gop;

	u8		fgop_max_N;
	u8		fgop_min_N;
	u8		M;
	u8		N;

	u8		gop_structure;
	u8		idr_interval;
	u16		reserved0;

	u64		pts_to_change;
} iav_duplex_update_gop_t;

typedef struct iav_duplex_demand_idr_s {
	u8		enc_id;		// in
	u8		stream_type;	//in

	u8		on_demand_idr;//0: not change, 1: change next I/P, 2: change next I/P when pts>pts_to_change
	u8		reserved0;

	u64		pts_to_change;
} iav_duplex_demand_idr_t;

typedef struct iav_duplex_update_preview_vout_config_s {
	u8 enc_id;
	u8 preview_vout_index;
	u8 preview_alpha;
	u8 vout_index;

	u16 preview_left;
	u16 preview_top;
	u16 preview_width;
	u16 preview_height;
} iav_duplex_update_preview_vout_config_t;

// decoding

enum {
	IAV_DEC_STATE_IDLE = 0,
	IAV_DEC_STATE_STOPPED,
	IAV_DEC_STATE_RUNNING,
};

enum {
	IAV_DEC_TYPE_NONE = 0,
	IAV_DEC_TYPE_H264,
	IAV_DEC_TYPE_MPEG2,
	IAV_DEC_TYPE_MPEG4_SW,
	IAV_DEC_TYPE_MPEG4_HW,
	IAV_DEC_TYPE_VC1,
	IAV_DEC_TYPE_RV,
};

typedef struct iav_duplex_init_decoder_s {
	u8	dec_id;		// must be 0
	u8	dec_type;	// IAV_DEC_TYPE_H264, IAV_DEC_TYPE_MPEG2

	union {
		struct {
			u8	enable_pic_info;
			u8	use_tiled_dram;
			u32	rbuf_smem_size;	// reference smem cache size
			u32	fbuf_dram_size;	// (AVC) frame buffer size in DRAM
			u32	pjpeg_buf_size;	// (AVC) pseudo JPEG buffer size in DRAM
			u32	svc_fbuf_dram_size;	// (SVC) frame buffer size in DRAM
			u32	svc_pjpeg_buf_size;	// (SVC) pseudo JPEG buffer size in DRAM
			u8	cabac_2_recon_delay;
			u8	force_fld_tiled;
			u8	ec_mode;
			u8	svc_ext;
			u8	warp_enable;
			u8	max_frm_num_of_dpb;
			u16	max_frm_buf_width;
			u16	max_frm_buf_height;
		} h264;

		struct {
			u8	enable_pic_info;
			u32	fbuf_smem_size;
			u32	fbuf_dram_size;
		} mpeg2;
	} u;

	iav_decoder_display_t display_config;

	u8	*bits_fifo_start;	// out, user space
	u32	bits_fifo_size;		// out

	u8	*mv_fifo_start;		// out, user space
	u32	mv_fifo_size;		// out
} iav_duplex_init_decoder_t;

typedef struct iav_duplex_decode_s {
	u8	dec_id;		// must be 0
	u8	dec_type;	// not used now, keep same with iav_duplex_init_decoder_t
	u8	reserved0[2];

	union {
		struct {
			u8	*start_addr;
			u8	*end_addr;	// exclusive
			u32	first_pts_high;
			u32	first_pts_low;
			u32	num_pics;
			u32	num_frame_decode;
		} h264;

		struct {
			u8	*start_addr;
			u8	*end_addr;
		} mpeg2;
	} u;
} iav_duplex_decode_t;

typedef struct iav_duplex_get_decoder_status_s {
	u8	dec_id;// in

	u8	dec_state;	// IAV_DEC_STATE_IDLE, IAV_DEC_STATE_STOPPED, IAV_DEC_STATE_RUNNING
	u8	dec_type;	// IAV_DEC_TYPE_NONE, IAV_DEC_TYPE_H264, ...
	u8	decode_state;	// from DSP

	u8	last_pts_valid;
	u8	reserved0[3];

	u32	last_pts_low;	// from DSP
	u32	last_pts_high;	// from DSP

	u8	recommand_error_handling_method;//out: 0: do nothing, 1: stop(1), and restart playback, 2: stop(0) and exit playback
	u8	decoder_error_level;	//out: from DSP
	u16	decoder_error_type;	//out: from DSP

	u32	bsb_free_room;//out: from DSP
	u32	bsb_tot_size;
} iav_duplex_get_decoder_status_t;

typedef struct iav_duplex_pb_speed_s {
	u8 dec_id;
	u8 reserved[3];

	u16 speed;//8bits integer, 8bits fraction, default 0 or 0x100
	u8 scan_mode;//0: all frames, 1: play only I
	u8 direction;//0: forward, 1: backward
} iav_duplex_pb_speed_t;

typedef struct iav_duplex_trick_play_s {
	u8 dec_id;

	u8 tp_mode;//0: pause, 1: resume, 2: step
	u8 reserved[2];
} iav_duplex_trick_play_t;

#endif

