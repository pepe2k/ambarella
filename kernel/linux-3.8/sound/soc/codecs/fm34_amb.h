#ifndef FM34_H
#define FM34_H

//parameters accociated with sw process on/off
#define LINE_PASS									0x2300
#define SAMPLE_RATE									0x2301
#define NUM_OF_MICS								0x2302
#define KL_CONFIG									0x2303
#define SP_FLAG										0x2304
#define FT_FLAG										0x2305
#define LINEIN_HPF_SEL								0x2310
#define MIC_REVERT_MODE							0x22ee
#define PWRDWN_SET									0x22f1
#define SW_BYPASS_CTRL								0x22f5
#define PROFILE_INDEX								0x22f8
#define PARSER_SYNC_FLAG							0x22fb

//parameters associate with device enable/format
#define DAC_CTRL									0x230b
#define DAC_OUPUT_SELECT							0x23ce
#define CLK_DIV										0x22c6
#define CLK_MUL										0x22c7
#define PLL_DIV_TYPE								0x22c8
#define CHI_BCLK_SET								0x22ca
#define CHI_FCLK_SET								0x22cb
#define CHI_BCLK_SET_8K								0x22cc
#define CHI_FCLK_SET_8K								0x22cd
#define I2S_BCLK_SET								0x22ce
#define I2S_LRCK_SET								0x22cf
#define I2S_BCLK_SET_8K								0x22d0
#define I2S_LRCK_SET_8K								0x22d1
#define I2S_SPECIAL_MODE							0x22d2
#define BYPASS123_EQ_DRC_CTRL						0x22e5
#define BYPASS123_RATE								0x22eb
#define SRC_UPRATIO									0x22ed
#define SAMPLERATE1_CODEC_RATE					0x22fe
#define SAMPLERATE1_PROC_RATIO_CONFIG			0x22ff
#define MIPS_SETTING								0x22f2
#define PWD_MIPS_SET								0x22f4
#define DAC_MODE_SELECT							0x22f6
#define DIGI_CONTROL								0x22f9
#define DV_ENABLE_B									0x22fa

//parameters associate with gain
#define MIC_PGAGAIN									0x2307
#define MIC_DIFF_GAIN								0x2308
#define LINEIN_PGAGAIN								0x2309
#define DAC_PGAGAINA								0x230a
#define MIC_VOLUME									0x230c
#define SPK_VOLUME									0x230d
#define SPK_MUTE									0x230e
#define MIC_MUTE									0x230f
#define MIC0_SW_I2SGAIN							0x2365
#define LOUTDRC_POST_GAIN							0x237a
#define VOL_INC_STEP								0x22e3
#define VOL_INDEX									0x22e4

//parameters associate with spk and drc
#define SPK_DB_DROP									0x2325
#define SPK_DB_DECAY								0x2326
#define VAD3P_ALPHA								0x2327
#define LOUT_DRC_LEVEL								0x23d3
#define LOUT_DRC_SLANT								0x23d4
#define SPKOUT_DRC_TH1								0x23e7
#define SPKOUT_DRC_TH2								0x23e8
#define SPKOUT_DRC_SLOP1							0x23e9
#define SPKOUT_DRC_SLOP2							0x23ea

//parameters associated with mic-in auto calibration
#define MICGAIN0									0x2348
#define MICGAIN1									0x2349
#define MIC_CAL_DIFF								0x2350
#define MIC_CAL_CENTER								0x2351
#define MIC_CAL_MAX_RATIO							0x2353
#define MIC_CAL_MIN_RATIO							0x2354
#define MIC_CAL_RATIO_GAP							0x2355
#define MIC_CAL_RATIO_INIT							0x2356

//parameters associated with mic-in AGC and far-field pickup<Enable: bit 15 of 0x2304>
#define MICAGC_MINAGC								0x2360
#define MICAGC_MAXAGC								0x2361
#define MICAGC_ALPHA_UP							0x2362
#define MICAGC_ALPHA_DOWN							0x2363
#define MICAGC_REF									0x2364
#define MICAGC_NOISE_THRD							0x23a6

//parameters associated with line-in AGC<Enable: bit 11 of 0x2304>
#define LINEAGC_REF									0x236a
#define LINEAGC_MINAGC								0x236b
#define LINEAGC_MAXAGC								0x236c

//parameter associated with noise paste back<Enable: bit 0 of 0x2305>
#define NOISEGAIN									0x2372

//parameter associate with acousite echo cancellation
#define MIC_SAT_TH									0x2328
#define AEC_REF_GAIN								0x232f
#define FE_VAD_TH_HIGH								0x2332
#define FE_VAD_TH									0x2333
#define AEC_MINMU_NOFE								0x2336
#define AEC_NW_SHIFT								0x2337
#define AEC_DELAY_LENTH							0x2339
#define PSEUDO_AEC_REF_GAIN						0x233c
#define FQ_FEVAD_GAIN_LIMIT						0x238c
#define ECHO_DOMAIN_VAD_THRD						0x2396
#define ECHO_DOMAIN_EQUAL_VAD_THRD				0x2397
#define ECHO_DOMAIN_VAD_GND						0x2398
#define PF_START_BIN								0x23a5
#define PF_Z_FACTOR_EXP_HIGH						0x23b3
#define PF_Z_FACTOR_EXP_LOW						0x23b4
#define PF_WAIT										0x23b7
#define PF_MIN_ATTN_ECHO_DOMAIN					0x23bb
#define LFVAD_NOISE_THRD							0x23bc
#define LFVAD_ADDON_THRD							0x23bd
#define LF_NOISE_VAD_THRD							0x23be

//parameters associate with noise suppression
#define MIC_NS_LEVEL								0x236e
#define MIC_NS_GAIN									0x236f
#define MIC_EXTRA_NS_GAIN							0x2370
#define FQ_GNDL_DIV									0x2373
#define FQ_GNDM_DIV								0x2374
#define FQ_GNDH_DIV								0x2375
#define FQ_MIN_RECURSIVE							0x2376
#define FQ_PERIOD									0x237b
#define FQ_BETA_V1									0x237c
#define FQ_BETA_V2									0x237d
#define FQ_BETA_MIX2								0x237e
#define FQ_BETA_UV2									0x237f
#define FQ_BETA_UV2_FE								0x2380
#define FQ_BETA_MIX2_FE							0x2381
#define SNR_ORDER									0x2384
#define FQ_SEVERE_SNR								0x2385
#define FQ_MIN_WEIGHT_STAT						0x2386
#define FQ_REFEO_GAIN_INIT							0x238a
#define FQ_SHAPE_SLANT								0x238e
#define FQ_INBEAM_DEC								0x239c
#define OUTBEAM_DEC_MID							0x239d
#define OUTBEAM_DEC_HI								0x239e
#define FENS_GAIN_LIMIT								0x23db
#define FENS_VAD_THRD								0x23dc
#define IDLE_NOISE_THRD							0x23ed
#define IDLE_INS_ATTN								0x23ee
#define INS_ALPHA_UP								0x23ef
#define INS_ALPHA_DOWN							0x23f0

//parameter associate with lineout/spkout equalizer
#define FQ_EQUAL1									0x2390
#define FQ_EQUAL2									0x2391
#define FQ_EQUAL3									0x2392
#define FQ_EQUAL4									0x2393
#define FQ_EQUAL5									0x2394
#define FQ_EQUAL6									0x2395
#define FENS_EQUAL1									0x23df
#define FENS_EQUAL2									0x23e0
#define FENS_EQUAL3									0x23e1
#define FENS_EQUAL4									0x23e2
#define FENS_EQUAL5									0x23e3
#define FENS_EQUAL6									0x23e4

//parameter associate with board-size array
#define OUTBEAM_VAD_DOMAIN_THRD					0x2387
#define SS_BOUNDS_HIGH_LOWSNR_0					0x22a0
#define SS_BOUNDS_HIGH_LOWSNR_1					0x22a1
#define FQ_GNDL_DIV_LOWSNR						0x22a2
#define FQ_GNDM_DIV_LOWSNR						0x22a3
#define FQ_GNDH_DIV_LOWSNR						0x22a4
#define FQ_SNR_ORDER_LOWSNR						0x22aa
#define VAD1_FRAME_CNT_THRD_BS_LOWSNR			0x22ad
#define VAD2_FRAME_CNT_THRD_BS_LOWSNR			0x22ae
#define VAD2_EQUAL_FRAME_CNT_THRD_BS_LOWSNR	0x22af
#define BFILTER_XAB_TAIL							0x22b0
#define BFILTER_CON3_THRD							0x22b1
#define BFILTER_BEAM1_TAIL							0x22b2
#define BFILTER_BEAM1_DELAY						0x22b3
#define BFILTER_BEAM2_TAIL							0x22b4
#define BFILTER_XAB_THRD							0x22b5
#define VAD1_FRAME_CNT_THRD_BS					0x22b7
#define VAD2_FRAME_CNT_THRD_BS					0x22d8

//parameter associated with vad
#define VAD1_GND_MIN								0x2358
#define VAD2_GND_MIN								0x2359
#define VAD1_THRD									0x235a
#define VAD2_THRD									0x235b
#define VAD2_EQUAL_THRD							0x2377
#define FQ_VAD_THRD_LOW							0x2382
#define FQ_VAD_THRD_HIGH							0x2383
#define VAD0_RAT_THRD_FE							0x23cf
#define VAD0_RAT_THRD_NOFE						0x23d0
#define VAD01_THRD									0x23d1
#define VAD02_THRD									0x23d2
#define VAD3_RAT_THRD								0x23d5

//parameter associate with side tone generation
#define SIDETONE_GAIN								0x22f7

//parameter associated with bright voice engine
#define BVE_LIN_CALIBRATION_BAND123				0x23c4
#define BVE_LIN_CALIBRATION_BAND45				0x23c5
#define BVE_MICPOW_BAND123						0x23c6
#define BVE_MICPOW_BAND45							0x23c7
#define BVE_GAIN_LIMIT_BAND123					0x23c8
#define BVE_GAIN_LIMIT_BAND45						0x23c9
#define BVE_MASKING									0x23cd
#define BVE_LFATTENUATE							0x23ca

//parameter associated for status indicator
#define FRAME_COUNTER								0x2306
#define MIC_AGC_GAIN1								0x2366
#define MIC_AGC_GAIN2								0x2367
#define LINEIN_AGC_GAIN							0x236d

//parameter associate with signal status
#define MAX_MIC0_READOUT							0x22d3
#define MAX_MIC1_READOUT							0x22d4
#define MAX_LIN_READOUT							0x22d5
#define MAX_LOUT_READOUT							0x22d6
#define MAX_SPK_READOUT							0x22d7
#define DEBUG_FLAG									0x22fc

#endif
