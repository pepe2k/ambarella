#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include <sched.h>
#include <signal.h>
#include <getopt.h>

#include "img_abs_filter.h"
#include "img_adv_struct_arch.h"
#include "load_param.c"

#include "test_tuning.h"
static int fd_iav;
static int tuning_mode =AMBA_DSP_IMG_ALGO_MODE_FAST;

static u8 anti_aliasing_strength;
static AMBA_DSP_IMG_COLOR_CORRECTION_REG_s	color_corr_reg;
static AMBA_DSP_IMG_COLOR_CORRECTION_s color_corr;
static EC_INFO ec_info;
static BLC_INFO blc_info;
static  AMBA_DSP_IMG_DEF_BLC_s defblc;
static SHARPEN_PKG sharpen_pkg;
static SA_ASF_PKG sa_asf_pkg;
static AMBA_DSP_IMG_WB_GAIN_s wb_gain ={4096,4096,4096,4096,4096};
static  AMBA_DSP_IMG_WB_GAIN_s tmp_wb_gain;
static AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s chroma_median_setup;
static AMBA_DSP_IMG_VIDEO_MCTF_INFO_s mctf_info;
static AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s mctf_mb_temporal;
static  AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s mctf_level_s;
static AMBA_DSP_IMG_RGB_TO_YUV_s rgb2yuv_matrix;
static AMBA_DSP_IMG_DGAIN_SATURATION_s  d_gain_satuation_level;
static AMBA_DSP_IMG_DBP_CORRECTION_s dbp_correction_setting;
static AMBA_DSP_IMG_CFA_NOISE_FILTER_s cfa_noise_filter;
static AMBA_DSP_IMG_CHROMA_FILTER_s chroma_noise_filter;
static AMBA_DSP_IMG_DEMOSAIC_s demosaic_info;
static AMBA_DSP_IMG_GBGR_MISMATCH_s mismatch_gbgr;
static AMBA_DSP_IMG_CDNR_INFO_s cdnr_info;
static AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s cfa_leakage_filter;
static AMBA_DSP_IMG_CHROMA_SCALE_s cs;
static AMBA_DSP_IMG_SHARPEN_BOTH_s sb_dsp_both;
static AMBA_DSP_IMG_SHARPEN_BOTH_s sa_dsp_both;
static AMBA_DSP_IMG_ASF_INFO_s sa_dsp_asf;
static aaa_cntl_t aaa_cntl_station;
AMBA_DSP_IMG_MODE_CFG_s ik_mode;
//static TILE_PKG tile_pkg;
static TUNING_TABLE_s tuning_tables;
//HHHHHHHHHHHHHHHHHISO
static u8 hiso_anti_aliasing_strength;
static AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s hiso_lkg;
static AMBA_DSP_IMG_DBP_CORRECTION_s hiso_bpc;
static AMBA_DSP_IMG_CFA_NOISE_FILTER_s hiso_cfa;
static AMBA_DSP_IMG_GBGR_MISMATCH_s hiso_gbgrmis;
static AMBA_DSP_IMG_DEMOSAIC_s hiso_demosaic;
static AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s hiso_cmf;
static AMBA_DSP_IMG_CDNR_INFO_s hiso_cdnr;
static AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s hiso_defer_cc;

static TUNING_ASF hiso_tuning_asf;//AMBA_DSP_IMG_ASF_INFO_s
static TUNING_ASF hiso_tuning_high_asf;
static TUNING_ASF hiso_tuning_low_asf;
static TUNING_ASF hiso_tuning_med1_asf;
static TUNING_ASF hiso_tuning_med2_asf;
static TUNING_ASF hiso_tuning_li2nd_asf;

static AMBA_DSP_IMG_ASF_INFO_s hiso_high_asf;//AMBA_DSP_IMG_ASF_INFO_s
static AMBA_DSP_IMG_ASF_INFO_s hiso_low_asf;
static AMBA_DSP_IMG_ASF_INFO_s hiso_med1_asf;
static AMBA_DSP_IMG_ASF_INFO_s hiso_med2_asf;
static AMBA_DSP_IMG_ASF_INFO_s hiso_li2nd_asf;
static AMBA_DSP_IMG_ASF_INFO_s hiso_asf;

static AMBA_DSP_IMG_CHROMA_ASF_INFO_s hiso_chroma_asf;
static TUNING_IMG_CHROMA_ASF_INFO_s hiso_tuning_chroma_asf;

static SHARPEN_PKG hiso_high_sharpen_pkg;
static SHARPEN_PKG hiso_med_sharpen_pkg;
static SHARPEN_PKG hiso_li_sharpen_pkg;
static SHARPEN_PKG hiso_li2nd_sharpen_pkg;

static AMBA_DSP_IMG_SHARPEN_BOTH_s liso_high_sharpen_both;
static AMBA_DSP_IMG_SHARPEN_BOTH_s liso_med_sharpen_both;
static AMBA_DSP_IMG_SHARPEN_BOTH_s liso_li_sharpen_both;
static AMBA_DSP_IMG_SHARPEN_BOTH_s liso_li2nd_sharpen_both;

static AMBA_DSP_IMG_HISO_CHROMA_FILTER_s hiso_chroma_filter_pre;
static AMBA_DSP_IMG_HISO_CHROMA_FILTER_s hiso_chroma_filter_med;
static AMBA_DSP_IMG_HISO_CHROMA_FILTER_s hiso_chroma_filter_low;
static AMBA_DSP_IMG_HISO_CHROMA_FILTER_s hiso_chroma_filter_verylow;
static  AMBA_DSP_IMG_CHROMA_FILTER_s hiso_chroma_filter;
static AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s hiso_chroma_lowverylow_filter;
static AMBA_DSP_IMG_HISO_LUMA_BLEND_s hiso_luma_blend;

static AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s hiso_chroma_filter_combine_med;
static AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s hiso_chroma_filter_combine_low;
static AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s hiso_chroma_filter_combine_verylow;

static TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s hiso_tuning_chroma_filter_combine_med;
static TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s hiso_tuning_chroma_filter_combine_low;
static TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s hiso_tuning_chroma_filter_combine_verylow;

static TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s hiso_tuning_luma_noise_combine;
static TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s hiso_tuning_low_asf_combine;
static AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s hiso_luma_noise_combine;
static AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s hiso_low_asf_combine;

static AMBA_DSP_IMG_HISO_COMBINE_s hiso_high_combine;
static TUNING_IMG_HISO_COMBINE_s hiso_tuning_high_combine;
static AMBA_DSP_IMG_HISO_FREQ_RECOVER_s hiso_freq_recover;


static int receive_msg(int sock_fd,u8* buff,int size)
{
	int length =0;
	while(length < size)
	{
		int retv=0;
	 	retv=recv(sock_fd,buff+length,size - length,0);
		if(retv<=0)
		{
			if (retv == 0)
			{
				printf("Port  closed\n");
				return -2;
			}
			printf("recv() returns %d\n", retv);
			return -1;
		}
		length+=retv;
	//	printf("length %d , size %d\n",length,size);
	}
	return length;
}
#if 0
static int send_msg(int sock_fd,u8* buff,int size)
{
	int length =0;
	while(length < size)
	{
		int retv=0;
	 	retv=send(sock_fd,buff+length,size - length,MSG_NOSIGNAL);
		if(retv<=0)
		{
			if (retv == 0)
			{
				printf("Port  closed\n");
				return -2;
			}
			printf("send() returns %d\n", retv);
			return -1;
		}
		length+=retv;
		printf("length =%d\n",length);
	}
	return length;
}
#endif
static int send_msg_ex(int sock_fd,u8* buff,int size)
{
	int length =0;
	int send_size =0;
	while(length < size)
	{
		int remain =size -length;
		if(remain>2000)
			send_size =2000;
		else
			send_size =remain;
		int retv=0;
	 	retv=send(sock_fd,buff+length,send_size,MSG_NOSIGNAL);
		if(retv<=0)
		{
			if (retv == 0)
			{
				printf("Port  closed\n");
				return -2;
			}
			printf("send() returns %d\n", retv);
			return -1;
		}
		length+=retv;
	//	printf("length =%d\n",length);
	}
	return length;
}
static int SockServer_Setup(int sockfd,int port)
{
	int on =2048;
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		perror("socket");
		return -1;
	}
	setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR,(char*) &on, sizeof(int) );

	struct sockaddr_in  my_addr;
	my_addr.sin_family=AF_INET;
	my_addr.sin_port=htons(port);
	my_addr.sin_addr.s_addr=INADDR_ANY;
	bzero(&(my_addr.sin_zero),0);

	if(bind(sockfd,(struct sockaddr*)&my_addr,sizeof(struct sockaddr))==-1)
	{
		perror("bind");
		return -1;
	}
	if(listen(sockfd,10)==-1)
	{
		perror("listen");
		return -1;
	}
	return sockfd;

}
static int SockServer_free(int sock_fd,int client_fd)
{
	if(sock_fd!=-1)
	{
		close(sock_fd);
		sock_fd =-1;
	}
	if(client_fd!=-1)
	{
		close(client_fd);
		client_fd =-1;
	}
	return 0;
}

static void init_tuning_table()//only for test_tuning
{
	sa_dsp_both.ThreeD.pTable =tuning_tables.SharpenBothThreeDTable;
	sa_dsp_asf.Adapt.ThreeD.pTable =tuning_tables.AsfInfoThreeDTable;
	sb_dsp_both.ThreeD.pTable =tuning_tables.FinalSharpenBothThreeDTable;
	hiso_asf.Adapt.ThreeD.pTable =tuning_tables.HisoAsfThreeDTable;
	hiso_high_asf.Adapt.ThreeD.pTable =tuning_tables.HisoHighAsfThreeDTable;
	hiso_med1_asf.Adapt.ThreeD.pTable =tuning_tables.HisoMed1AsfThreeDTable;
	hiso_med2_asf.Adapt.ThreeD.pTable =tuning_tables.HisoMed2AsfThreeDTable;
	hiso_low_asf.Adapt.ThreeD.pTable =tuning_tables.HisoLowAsfThreeDTable;
	hiso_li2nd_asf.Adapt.ThreeD.pTable =tuning_tables.HisoLi2ndAsfThreeDTable;
	hiso_chroma_asf.ThreeD.pTable=tuning_tables.HisoChromaAsfThreeDTable;
	liso_high_sharpen_both.ThreeD.pTable =tuning_tables.HisoHighSharpenBothThreeDTable;
	liso_med_sharpen_both.ThreeD.pTable =tuning_tables.HisoMedSharpenBothThreeDTable;
	liso_li_sharpen_both.ThreeD.pTable =tuning_tables.HisoLiso1SharpenBothThreeDTable;
	liso_li2nd_sharpen_both.ThreeD.pTable =tuning_tables.HisoLiso2SharpenBothThreeDTable;
	hiso_chroma_filter_combine_med.ThreeD.pTable =tuning_tables.HisoChromaFilterMedCombineThreeDTable;
	hiso_chroma_filter_combine_low.ThreeD.pTable =tuning_tables.HisoChromaFilterLowCombineThreeDTable;
	hiso_chroma_filter_combine_verylow.ThreeD.pTable =tuning_tables.HisoChromaFilterVeryLowCombineThreeDTable;
	hiso_luma_noise_combine.ThreeD.pTable =tuning_tables.HisoLumaNoiseCombineThreeDTable;
	hiso_low_asf_combine.ThreeD.pTable =tuning_tables.HisoLowASFCombineThreeDTable;
	hiso_high_combine.ThreeD.pTable =tuning_tables.HighIsoCombineThreeDTable;

}
static int start_aaa_init(int fd_iav)
{
	char sensor_name[64];
	u32 sensor_id;
	IMG_PIPELINE_SEL img_pipe =IMG_PIPELINE_LISO;
	struct amba_vin_source_info vin_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}
	sensor_id =(u32)vin_info.sensor_id;
	memset(&ik_mode, 0, sizeof(ik_mode));
	ik_mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	ik_mode.BatchId = 0xff;
	if(tuning_mode ==HISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
		img_pipe =IMG_PIPELINE_HISO;
	}
	else if(tuning_mode ==LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
		img_pipe =IMG_PIPELINE_LISO;
	}

	load_containers(sensor_id,sensor_name);
	load_cc_bin(sensor_name);
	enable_cc(fd_iav,sensor_name,&ik_mode);

	img_hiso_config_pipeline(img_pipe, 1);// 1:exposure number =1
	img_aaa_config(fd_iav,sensor_id);
	img_hiso_prepare_idsp(fd_iav);

	return 0;

}
static void process_other(int sock_fd,TUNING_ID* p_tuning_id)
{
/*
	switch(p_tuning_id->tab_id)
	{
		case OnlineTuning:
		{
			switch(p_tuning_id->item_id)
			{
				case CHECK_IP:
				{
					char test[100] ="test_tuning";
					send(sock_fd,&test,strlen(test),0);
					printf("Available IP addess!\n");
					break;
				}
				case GET_RAW:
				{
					if(is_cali_mode(sock_fd))
					{
						get_raw(sock_fd);
						printf("get raw data done!\n");
					}
					break;
				}
				case DSP_DUMP:
				{
					u8 sec_id =0;
					recv(sock_fd,&sec_id,sizeof(u8),MSG_WAITALL);
					idsp_dump(sock_fd,sec_id);
					printf("dump done!\n");
					break;
				}
				case BW_MODE:
				{
					u8 bw_mode =0;
					recv(sock_fd,&bw_mode,sizeof(u8),MSG_WAITALL);
					img_set_bw_mode(bw_mode);
					printf("bw mode =%d\n",bw_mode);
					break;
				}
				case BP_CALI:
				{
					recv(sock_fd,&fpn_pkg.fpn_info,sizeof(FPN_INFO),MSG_WAITALL);
					if(fpn_pkg.fpn_info.mode ==0)//correct
					{
						load_fixed_pattern_noise(sock_fd);
						printf("BP cali:correct done!\n");
					}
					else if(fpn_pkg.fpn_info.mode ==1)//detect
					{
						if(is_cali_mode(sock_fd))
						{
							fixed_pattern_noise_cal(sock_fd);
							printf("BP cali:detect done!\n");
						}
					}
					break;
				}
				case LENS_CALI:
				{
					u8 mode=0;
					recv(sock_fd,&mode,sizeof(u8),MSG_WAITALL);
					if(mode ==0)//calc
					{
						if(is_cali_mode(sock_fd))
						{
							lens_shading_cal(sock_fd);
							printf("Lens cali: calc bin done!\n");
						}
					}
					else
					{
						vignette_compensation(sock_fd);
						printf("Lens cali: load bin done!\n");
					}
					break;
				}
			}
		}
		break;
	}
*/
}
static void process_load(int sock_fd,TUNING_ID* p_tuning_id)
{
	printf("Load!\n");
	switch(p_tuning_id->tab_id)
	{
	case OnlineTuning:
	{
		switch(p_tuning_id->item_id)
		{
			case CHECK_IP:
			{
			//	char test[100] ="test_hiso_tuning";
			//	send(sock_fd,&test,strlen(test),0);
				printf("Available IP addess!\n");
				break;
			}
			case BlackLevelCorrection:
				AmbaDSP_ImgGetStaticBlackLevel(&ik_mode,&blc_info.blc);
				AmbaDSP_ImgGetDeferredBlackLevel(&ik_mode,&defblc);
				blc_info.defblc_enable =defblc.Enb;
				send(sock_fd,(char*)&blc_info,sizeof(BLC_INFO),0);
				printf("Black Level Correction!\n");
				break;

			case ColorCorrection:
			{
				u8 * cc_info =malloc(cc_reg_size+cc_matrix_size);
				memset(cc_info,0,cc_reg_size+cc_matrix_size);
				AmbaDSP_ImgGetColorCorrectionReg(&ik_mode,&color_corr_reg);
				AmbaDSP_ImgGetColorCorrection(&ik_mode,&color_corr);
				memcpy(	cc_info,(u8*)color_corr_reg.RegSettingAddr,cc_reg_size);
				memcpy(	cc_info+cc_reg_size,(u8*)color_corr.MatrixThreeDTableAddr,cc_matrix_size);
				send_msg_ex(sock_fd,cc_info,cc_reg_size+cc_matrix_size);
				send(sock_fd,"!",1,0);
				free(cc_info);
				printf("Color Correction\n");
				break;
			}

			case ToneCurve:
				AmbaDSP_ImgGetToneCurve(&ik_mode,&tone_curve);
				send(sock_fd,(char*)&tone_curve,sizeof(AMBA_DSP_IMG_TONE_CURVE_s),0);
				printf("Tone Curve!\n ");
				break;

			case RGBtoYUVMatrix:
				AmbaDSP_ImgGetRgbToYuvMatrix(&ik_mode,&rgb2yuv_matrix);
				send(sock_fd,(char*)&rgb2yuv_matrix,sizeof(AMBA_DSP_IMG_RGB_TO_YUV_s),0);
				printf("RGB to YUV Matrix!\n");
				break;

			case WhiteBalanceGains:
				AmbaDSP_ImgGetWbGain(&ik_mode,&wb_gain);

				send(sock_fd,(char*)&wb_gain,sizeof(AMBA_DSP_IMG_WB_GAIN_s),0);
				printf("White Balance Gains!\n");
				break;

			case DGainSaturaionLevel:
				AmbaDSP_ImgGetDgainSaturationLevel(&ik_mode,&d_gain_satuation_level);
				send(sock_fd,(char*)&d_gain_satuation_level,sizeof(AMBA_DSP_IMG_DGAIN_SATURATION_s),0);
				printf("DGain Saturation Level!\n");
				break;

			case LocalExposure:
				AmbaDSP_ImgGetLocalExposure(&ik_mode,&local_exposure);
				send(sock_fd,(char*)&local_exposure,sizeof(AMBA_DSP_IMG_LOCAL_EXPOSURE_s),0);
				printf("Local Exposure!\n");
				break;

			case ChromaScale:
				AmbaDSP_ImgGetChromaScale(&ik_mode,&cs);
				send(sock_fd,(char*)&cs,sizeof(AMBA_DSP_IMG_CHROMA_SCALE_s),0);
				printf("Chroma Scale!\n");
				break;

			case FPNCorrection://not finished
				break;

			case BadPixelCorrection:
				AmbaDSP_ImgGetDynamicBadPixelCorrection(&ik_mode,&dbp_correction_setting);
				send(sock_fd,(char*)&dbp_correction_setting,sizeof(AMBA_DSP_IMG_DBP_CORRECTION_s),0);
				printf("Bad Pixel Correction!\n");
				break;

			case CFALeakageFilter:
				AmbaDSP_ImgGetCfaLeakageFilter(&ik_mode,&cfa_leakage_filter);
				send(sock_fd,(char*)&cfa_leakage_filter,sizeof(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s),0);
				printf("CFA Leakage Filter!\n");
				break;

			case AntiAliasingFilter:
				AmbaDSP_ImgGetAntiAliasing(&ik_mode,&anti_aliasing_strength);
				send(sock_fd,(char*)&anti_aliasing_strength,sizeof(u8),0);
				printf("Anti-Aliasing Filter!\n");
				break;

			case CFANoiseFilter:
				AmbaDSP_ImgGetCfaNoiseFilter(&ik_mode,&cfa_noise_filter);
				send(sock_fd,(char*)&cfa_noise_filter,sizeof(AMBA_DSP_IMG_CFA_NOISE_FILTER_s),0);
				printf("CFA Noise Filter!\n");
				break;

			case ChromaMedianFiler:
				AmbaDSP_ImgGetChromaMedianFilter(&ik_mode,&chroma_median_setup);
				send(sock_fd,(char*)&chroma_median_setup,sizeof(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s),0);
				printf("Chroma Median Filer!\n");
				break;

			case SharpeningA_ASF:
				AmbaDSP_ImgGet1stLumaProcessingMode(&ik_mode,(AMBA_DSP_IMG_SHP_A_SELECT_e*)&sa_asf_pkg.select_mode);
				AmbaDSP_ImgGetAdvanceSpatialFilter(&ik_mode,&sa_dsp_asf);
				feed_tuning_asf(&sa_asf_pkg.asf_info,&sa_dsp_asf);
				AmbaDSP_ImgGet1stSharpenNoiseBoth(&ik_mode,&sa_dsp_both);
				feed_tuning_both( &sa_asf_pkg.sa_info.both,&sa_dsp_both);
				AmbaDSP_ImgGet1stSharpenNoiseNoise(&ik_mode,&sa_asf_pkg.sa_info.noise);
				AmbaDSP_ImgGet1stSharpenNoiseSharpenCoring(&ik_mode,&sa_asf_pkg.sa_info.coring);
				AmbaDSP_ImgGet1stSharpenNoiseSharpenFir(&ik_mode,&sa_asf_pkg.sa_info.fir);
				AmbaDSP_ImgGet1stSharpenNoiseSharpenMinCoringResult(&ik_mode,&sa_asf_pkg.sa_info.MinCoringResult);
				AmbaDSP_ImgGet1stSharpenNoiseSharpenCoringIndexScale(&ik_mode,&sa_asf_pkg.sa_info.CoringIndexScale);
				AmbaDSP_ImgGet1stSharpenNoiseSharpenScaleCoring(&ik_mode,&sa_asf_pkg.sa_info.ScaleCoring);
				send_msg_ex(sock_fd,(u8*)&sa_asf_pkg,sizeof(SA_ASF_PKG));
				send(sock_fd,"!",1,0);
				printf("SharpeningA_ASF!\n");
				break;

			case MCTFControl:
				AmbaDSP_ImgGetVideoMctf(&ik_mode,&mctf_info);
				send(sock_fd,(char*)&mctf_info,sizeof(AMBA_DSP_IMG_VIDEO_MCTF_INFO_s),0);
				printf("MCTF Control!\n");
				break;

			case SharpeningBControl:
				AmbaDSP_ImgGetFinalSharpenNoiseBoth(&ik_mode,&sb_dsp_both);
				feed_tuning_both(&sharpen_pkg.both,&sb_dsp_both);
				AmbaDSP_ImgGetFinalSharpenNoiseNoise(&ik_mode,&sharpen_pkg.noise);
				AmbaDSP_ImgGetFinalSharpenNoiseSharpenCoring(&ik_mode,&sharpen_pkg.coring);
				AmbaDSP_ImgGetFinalSharpenNoiseSharpenFir(&ik_mode,&sharpen_pkg.fir);
				AmbaDSP_ImgGetFinalSharpenNoiseSharpenMinCoringResult(&ik_mode,&sharpen_pkg.MinCoringResult);
				AmbaDSP_ImgGetFinalSharpenNoiseSharpenCoringIndexScale(&ik_mode,&sharpen_pkg.CoringIndexScale);
				AmbaDSP_ImgGetFinalSharpenNoiseSharpenScaleCoring(&ik_mode,&sharpen_pkg.ScaleCoring);
				send_msg_ex(sock_fd,(u8*)&sharpen_pkg,sizeof(SHARPEN_PKG));
				send(sock_fd,"!",1,0);
				printf("SharpeningBControl!\n");
				break;

			case ColorDependentNoiseReduction:
				AmbaDSP_ImgGetColorDependentNoiseReduction(&ik_mode,&cdnr_info);
				send(sock_fd,(char*)&cdnr_info,sizeof(AMBA_DSP_IMG_CDNR_INFO_s),0);
				printf("Color Dependent Noise Reduction\n");
				break;

			case ChromaNoiseFilter:
				AmbaDSP_ImgGetChromaFilter(&ik_mode,&chroma_noise_filter);
				send(sock_fd,(char*)&chroma_noise_filter,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
				printf("Chroma Noise Filter!\n");
				break;

			case TileConfiguration:
			//	AmbaDSP_Img3aGetAaaStatInfo( &ik_mode, &tile_pkg.stat_config_info);
			//	send(sock_fd,(char*)&tile_pkg,sizeof(TILE_PKG),0);
			//	printf("AF Tile Configuration!\n");
				break;

			case DEMOSAIC:
				AmbaDSP_ImgGetDemosaic(&ik_mode, &demosaic_info);
				send(sock_fd,(char*)&demosaic_info,sizeof(AMBA_DSP_IMG_DEMOSAIC_s),0);
				printf("Demosaic!\n");
				break;

			case MisMatchGr_Gb:
				AmbaDSP_ImgGetGbGrMismatch(&ik_mode, &mismatch_gbgr);
				send(sock_fd,(char*)&mismatch_gbgr,sizeof(AMBA_DSP_IMG_GBGR_MISMATCH_s),0);
				printf("MisMatchGr_Gb!\n");
				break;

			case ConfigAAAControl:
				img_hiso_get_3a_cntl_status(&aaa_cntl_station);
				send(sock_fd,(char*)&aaa_cntl_station,sizeof(aaa_cntl_t),0);
				printf(" Config AAA Control!\n");
				break;

			case AFStatisticSetupEx:
			//	img_dsp_get_af_statistics_ex(&af_statistic_setup_ex);
			//	send(sock_fd,(char*)&af_statistic_setup_ex,sizeof(af_statistics_ex_t),0);
			//	printf("AF Statistic Setup Ex!\n");
			//	break;

			case ExposureControl:
				ec_info.gain_tbl_idx=img_hiso_get_sensor_agc();
				ec_info.shutter_row=img_hiso_get_sensor_shutter();
				AmbaDSP_ImgGetWbGain(&ik_mode ,&wb_gain);
				ec_info.dgain =wb_gain.AeGain;
				send(sock_fd,(char*)&ec_info,sizeof(EC_INFO),0);
				printf("Exposure Control!\n");
				break;
			case MCTFMBTemporal:
				AmbaDSP_ImgGetVideoMctfMbTemporal(&ik_mode,&mctf_mb_temporal);
				send(sock_fd,(char*)&mctf_mb_temporal,sizeof(AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s),0);
				printf("MCTFMBTemporal \n");
				break;
			case MCTFLEVEL:
				AmbaDSP_ImgGetVideoMctfLevel(&ik_mode,&mctf_level_s);
				send(sock_fd,(char*)&mctf_level_s,sizeof(AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s),0);
				printf("MCTF LEVEL \n");
				break;
			case ITUNER_HISO_ANTI_ALIASING_STRENGTH:
				AmbaDSP_ImgGetHighIsoAntiAliasing(&ik_mode,&hiso_anti_aliasing_strength);
				send(sock_fd,(char*)&hiso_anti_aliasing_strength,sizeof(u8),0);
				printf("ITUNER_HISO_ANTI_ALIASING_STRENGTH\n");
				break;
			case ITUNER_HISO_CFA_LEAKAGE_FILTER:
				AmbaDSP_ImgGetHighIsoCfaLeakageFilter(&ik_mode,&hiso_lkg);
				send(sock_fd,(char*)&hiso_lkg,sizeof(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s),0);
				printf("ITUNER_HISO_CFA_LEAKAGE_FILTER\n");
				break;
			case ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION:
				AmbaDSP_ImgGetHighIsoDynamicBadPixelCorrection(&ik_mode,&hiso_bpc);
				send(sock_fd,(char*)&hiso_bpc,sizeof(AMBA_DSP_IMG_DBP_CORRECTION_s),0);
				printf("ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION\n");
				break;
			case ITUNER_HISO_CFA_NOISE_FILTER:
				AmbaDSP_ImgGetHighIsoCfaNoiseFilter(&ik_mode,&hiso_cfa);
				send(sock_fd,(char*)&hiso_cfa,sizeof(AMBA_DSP_IMG_CFA_NOISE_FILTER_s),0);
				printf("ITUNER_HISO_CFA_NOISE_FILTER\n");
				break;
			case ITUNER_HISO_GB_GR_MISMATCH:
				AmbaDSP_ImgGetHighIsoGbGrMismatch(&ik_mode,&hiso_gbgrmis);
				send(sock_fd,(char*)&hiso_gbgrmis,sizeof(AMBA_DSP_IMG_GBGR_MISMATCH_s),0);
				printf("ITUNER_HISO_GB_GR_MISMATCH\n");
				break;
			case ITUNER_HISO_DEMOSAIC_FILTER:
				AmbaDSP_ImgGetHighIsoDemosaic(&ik_mode,&hiso_demosaic);
				send(sock_fd,(char*)&hiso_demosaic,sizeof(AMBA_DSP_IMG_DEMOSAIC_s),0);
				printf("ITUNER_HISO_DEMOSAIC_FILTER\n");
				break;
			case ITUNER_HISO_CHROMA_MEDIAN_FILTER:
				AmbaDSP_ImgGetHighIsoChromaMedianFilter(&ik_mode,&hiso_cmf);
				send(sock_fd,(char*)&hiso_cmf,sizeof(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_MEDIAN_FILTER\n");
				break;
			case ITUNER_HISO_CDNR:
				AmbaDSP_ImgGetHighIsoColorDependentNoiseReduction(&ik_mode,&hiso_cdnr);
				send(sock_fd,(char*)&hiso_cdnr,sizeof(AMBA_DSP_IMG_CDNR_INFO_s),0);
				printf("ITUNER_HISO_CDNR\n");
				break;
			case ITUNER_HISO_DEFER_COLOR_CORRECTION:
				AmbaDSP_ImgGetHighIsoDeferColorCorrection(&ik_mode,&hiso_defer_cc);
				send(sock_fd,(char*)&hiso_defer_cc,sizeof(AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s),0);
				printf("ITUNER_HISO_DEFER_COLOR_CORRECTION\n");
				break;
			case ITUNER_HISO_ASF:
				AmbaDSP_ImgGetHighIsoAdvanceSpatialFilter(&ik_mode,&hiso_asf);
				feed_tuning_asf(&hiso_tuning_asf,&hiso_asf);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_asf,sizeof(TUNING_ASF));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_ASF\n");
				break;
			case ITUNER_HISO_HIGH_ASF:
				AmbaDSP_ImgGetHighIsoHighAdvanceSpatialFilter(&ik_mode,&hiso_high_asf);
				feed_tuning_asf(&hiso_tuning_high_asf,&hiso_high_asf);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_high_asf,sizeof(TUNING_ASF));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_HIGH_ASF\n");
				break;
			case ITUNER_HISO_LOW_ASF:
				AmbaDSP_ImgGetHighIsoLowAdvanceSpatialFilter(&ik_mode,&hiso_low_asf);
				feed_tuning_asf(&hiso_tuning_low_asf,&hiso_low_asf);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_low_asf,sizeof(TUNING_ASF));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_LOW_ASF\n");
				break;
			case ITUNER_HISO_MED1_ASF:
				AmbaDSP_ImgGetHighIsoMed1AdvanceSpatialFilter(&ik_mode,&hiso_med1_asf);
				feed_tuning_asf(&hiso_tuning_med1_asf,&hiso_med1_asf);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_med1_asf,sizeof(TUNING_ASF));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_MED1_ASF\n");
				break;
			case ITUNER_HISO_MED2_ASF:
				AmbaDSP_ImgGetHighIsoMed2AdvanceSpatialFilter(&ik_mode,&hiso_med2_asf);
				feed_tuning_asf(&hiso_tuning_med2_asf,&hiso_med2_asf);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_med2_asf,sizeof(TUNING_ASF));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_MED2_ASF\n");
				break;
			case ITUNER_HISO_LI2ND_ASF:

			//	AmbaDSP_ImgGetHighIsoLi2ndAdvanceSpatialFilter(&ik_mode,&hiso_li2nd_asf);
			//	feed_tuning_asf(&hiso_tuning_li2nd_asf,&hiso_li2nd_asf);
				printf("ITUNER_HISO_LI2ND_ASF\n");
				break;

			case ITUNER_HISO_CHROMA_ASF:
				AmbaDSP_ImgGetHighIsoChromaAdvanceSpatialFilter(&ik_mode,&hiso_chroma_asf);
				feed_tuning_chroma_asf(&hiso_tuning_chroma_asf,&hiso_chroma_asf);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_chroma_asf,sizeof(TUNING_IMG_CHROMA_ASF_INFO_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_CHROMA_ASF\n");
				break;
			case ITUNER_HISO_SHARPEN_HI_HIGH:
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseBoth(&ik_mode,&liso_high_sharpen_both);
				feed_tuning_both(&hiso_high_sharpen_pkg.both,&liso_high_sharpen_both);
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseNoise(&ik_mode,&hiso_high_sharpen_pkg.noise);
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenFir(&ik_mode,&hiso_high_sharpen_pkg.fir);
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenCoring(&ik_mode,&hiso_high_sharpen_pkg.coring);
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_high_sharpen_pkg.CoringIndexScale);
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_high_sharpen_pkg.MinCoringResult);
				AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_high_sharpen_pkg.ScaleCoring);
				send_msg_ex(sock_fd,(u8*)&hiso_high_sharpen_pkg,sizeof(SHARPEN_PKG));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_SHARPEN_HI_HIGH\n");
				break;
			case ITUNER_HISO_SHARPEN_HI_MED:
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseBoth(&ik_mode,&liso_med_sharpen_both);
				feed_tuning_both(&hiso_med_sharpen_pkg.both,&liso_med_sharpen_both);
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseNoise(&ik_mode,&hiso_med_sharpen_pkg.noise);
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenFir(&ik_mode,&hiso_med_sharpen_pkg.fir);
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenCoring(&ik_mode,&hiso_med_sharpen_pkg.coring);
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_med_sharpen_pkg.CoringIndexScale);
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_med_sharpen_pkg.MinCoringResult);
				AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_med_sharpen_pkg.ScaleCoring);
				send_msg_ex(sock_fd,(u8*)&hiso_med_sharpen_pkg,sizeof(SHARPEN_PKG));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_SHARPEN_HI_MED\n");
				break;
			case ITUNER_HISO_SHARPEN_HI_LI:

				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseBoth(&ik_mode,&liso_li_sharpen_both);
				feed_tuning_both(&hiso_li_sharpen_pkg.both,&liso_li_sharpen_both);
				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseNoise(&ik_mode,&hiso_li_sharpen_pkg.noise);
				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenFir(&ik_mode,&hiso_li_sharpen_pkg.fir);
				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenCoring(&ik_mode,&hiso_li_sharpen_pkg.coring);
				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_li_sharpen_pkg.CoringIndexScale);
				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_li_sharpen_pkg.MinCoringResult);
				AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_li_sharpen_pkg.ScaleCoring);
				send_msg_ex(sock_fd,(u8*)&hiso_li_sharpen_pkg,sizeof(SHARPEN_PKG));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_SHARPEN_HI_LI\n");
				break;
			case ITUNER_HISO_SHARPEN_HI_LI2ND:
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseBoth(&ik_mode,&liso_li2nd_sharpen_both);
				feed_tuning_both(&hiso_li2nd_sharpen_pkg.both,&liso_li2nd_sharpen_both);
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseNoise(&ik_mode,&hiso_li2nd_sharpen_pkg.noise);
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenFir(&ik_mode,&hiso_li2nd_sharpen_pkg.fir);
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenCoring(&ik_mode,&hiso_li2nd_sharpen_pkg.coring);
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_li2nd_sharpen_pkg.CoringIndexScale);
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_li2nd_sharpen_pkg.MinCoringResult);
				AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_li2nd_sharpen_pkg.ScaleCoring);
				send_msg_ex(sock_fd,(u8*)&hiso_li2nd_sharpen_pkg,sizeof(SHARPEN_PKG));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_SHARPEN_HI_LI2ND\n");
				break;

			case ITUNER_HISO_CHROMA_FILTER_HIGH:

				AmbaDSP_ImgHighIsoGetChromaFilterHigh(&ik_mode,&hiso_chroma_filter);
				send(sock_fd,(char*)&hiso_chroma_filter,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_FILTER_HIGH\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW:

				AmbaDSP_ImgHighIsoGetChromaFilterLowVeryLow(&ik_mode,&hiso_chroma_lowverylow_filter);
				send(sock_fd,(char*)&hiso_chroma_lowverylow_filter,sizeof(AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_PRE:

				AmbaDSP_ImgHighIsoGetChromaFilterPre(&ik_mode,&hiso_chroma_filter_pre);
				send(sock_fd,(char*)&hiso_chroma_filter_pre,sizeof(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_FILTER_PRE\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_MED:

				AmbaDSP_ImgHighIsoGetChromaFilterMed(&ik_mode,&hiso_chroma_filter_med);
				send(sock_fd,(char*)&hiso_chroma_filter_med,sizeof(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_FILTER_MED\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_LOW:

				AmbaDSP_ImgHighIsoGetChromaFilterLow(&ik_mode,&hiso_chroma_filter_low);
				send(sock_fd,(char*)&hiso_chroma_filter_low,sizeof(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_FILTER_LOW\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_VERY_LOW:

				AmbaDSP_ImgHighIsoGetChromaFilterVeryLow(&ik_mode,&hiso_chroma_filter_verylow);
				send(sock_fd,(char*)&hiso_chroma_filter_verylow,sizeof(AMBA_DSP_IMG_HISO_CHROMA_FILTER_s),0);
				printf("ITUNER_HISO_CHROMA_FILTER_VERY_LOW\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_MED_COMBINE:

				AmbaDSP_ImgHighIsoGetChromaFilterMedCombine(&ik_mode,&hiso_chroma_filter_combine_med);
				feed_tuning_chroma_filter_combine(&hiso_tuning_chroma_filter_combine_med,&hiso_chroma_filter_combine_med);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_chroma_filter_combine_med,sizeof(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_CHROMA_FILTER_MED_COMBINE\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE:

				AmbaDSP_ImgHighIsoGetChromaFilterLowCombine(&ik_mode,&hiso_chroma_filter_combine_low);
				feed_tuning_chroma_filter_combine(&hiso_tuning_chroma_filter_combine_low,&hiso_chroma_filter_combine_low);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_chroma_filter_combine_low,sizeof(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE\n");
				break;
			case ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE:

				AmbaDSP_ImgHighIsoGetChromaFilterVeryLowCombine(&ik_mode,&hiso_chroma_filter_combine_verylow);
				feed_tuning_chroma_filter_combine(&hiso_tuning_chroma_filter_combine_verylow,&hiso_chroma_filter_combine_verylow);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_chroma_filter_combine_verylow,sizeof(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE\n");
				break;

			case ITUNER_HISO_LUMA_NOISE_COMBINE:

				AmbaDSP_ImgHighIsoGetLumaNoiseCombine(&ik_mode,&hiso_luma_noise_combine);
				feed_tuning_luma_noise_combine(&hiso_tuning_luma_noise_combine,&hiso_luma_noise_combine);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_luma_noise_combine,sizeof(TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_LUMA_NOISE_COMBINE\n");
				break;
			case ITUNER_HISO_LOW_ASF_COMBINE:
				AmbaDSP_ImgHighIsoGetLowASFCombine(&ik_mode,&hiso_low_asf_combine);
				feed_tuning_luma_noise_combine(&hiso_tuning_low_asf_combine,&hiso_low_asf_combine);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_low_asf_combine,sizeof(TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HISO_LOW_ASF_COMBINE\n");
				break;
			case ITUNER_HIGH_ISO_COMBINE:
				AmbaDSP_ImgHighIsoGetHighIsoCombine(&ik_mode,&hiso_high_combine);
				feed_tuning_high_combine(&hiso_tuning_high_combine,&hiso_high_combine);
				send_msg_ex(sock_fd,(u8*)&hiso_tuning_high_combine,sizeof(TUNING_IMG_HISO_COMBINE_s));
				send(sock_fd,"!",1,0);
				printf("ITUNER_HIGH_ISO_COMBINE\n");
				break;
			case ITUNER_HISO_FREQ_RECOVER:
				AmbaDSP_ImgHighIsoGetHighIsoFreqRecover(&ik_mode,&hiso_freq_recover);
				send(sock_fd,(char*)&hiso_freq_recover,sizeof(AMBA_DSP_IMG_HISO_FREQ_RECOVER_s),0);
				printf("ITUNER_HISO_FREQ_RECOVER\n");
				break;
			case ITUNER_HISO_LOW2_LUMA_BLEND://TODO
				AmbaDSP_ImgHighIsoGetHighIsoLumaBlend(&ik_mode,&hiso_luma_blend);
				send(sock_fd,(char*)&hiso_luma_blend,sizeof(AMBA_DSP_IMG_HISO_LUMA_BLEND_s),0);
				printf("ITUNER_HISO_LOW2_LUMA_BLEND\n");
				break;
			}
			break;
		}


		case HDRTuning:
			break;

		case OTHER:
			printf("to be supported\n");
			break;

		default:
			printf("Unknow tab id [%c]\n",p_tuning_id->tab_id);
			break;
	}
}
int cfg_id = 0;
//#define DSP_DEBUG
static void process_apply(int sock_fd,TUNING_ID* p_tuning_id)
{
	printf("Apply!\n");
	int rval =0;
	switch(p_tuning_id->tab_id)
	{
	case OnlineTuning:
	{
		switch(p_tuning_id->item_id)
		{
		case TUNING_DONE:
		{
			ik_mode.ConfigId = cfg_id;
			AmbaDSP_ImgPostExeCfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 0);
#ifdef DSP_DEBUG

			AMBA_DSP_IMG_DEBUG_MODE_s debug = {0};
			debug.Step = 0;
			debug.Mode = 0;
			AmbaDSP_ImgSetDebugMode(&ik_mode, &debug);
			usleep(1000*1000);
			static AMBA_DSP_IMG_CFG_INFO_s CfgInfo;
			CfgInfo.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
			CfgInfo.CfgId =ik_mode.ConfigId;
			AmbaDSP_ImgHighIsoDumpCfg(CfgInfo, "/mnt/tuning_cfg");
			AmbaDSP_ImgHighIsoPrintCfg(&ik_mode, CfgInfo);
#endif
			cfg_id ^= 0x1;
			ik_mode.ConfigId = cfg_id;
			AmbaDSP_ImgPostExeCfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
			printf("TUNING DONE!!!!!!!!!!!!!!!!!\n\n");
			break;
		}
		case BlackLevelCorrection:
			rval=recv(sock_fd,(char*)&blc_info,sizeof(BLC_INFO),0);
			AmbaDSP_ImgSetStaticBlackLevel(fd_iav,&ik_mode, &blc_info.blc);
			printf("aaaaa\n");
			defblc.Enb =blc_info.defblc_enable;
			AmbaDSP_ImgSetDeferredBlackLevel(fd_iav,&ik_mode, &defblc);
			printf("Black Level Correction!\n");
			break;

		case ColorCorrection:
		{
			u8* cc_reg =malloc(cc_reg_size);
			u8* cc_matrix=malloc(cc_matrix_size);
			receive_msg(sock_fd,cc_reg,cc_reg_size);
			receive_msg(sock_fd,cc_matrix,cc_matrix_size);
			color_corr_reg.RegSettingAddr= (u32)cc_reg;
			color_corr.MatrixThreeDTableAddr = (u32)cc_matrix;
			rval = AmbaDSP_ImgSetColorCorrectionReg(&ik_mode,&color_corr_reg);
			CHECK_RVAL
			rval = AmbaDSP_ImgSetColorCorrection(fd_iav,&ik_mode, &color_corr);
			CHECK_RVAL
			free(cc_reg);
			free(cc_matrix);
			printf("Color Correction!\n");
			break;
		}
		case ToneCurve:
			{
				rval=recv(sock_fd,(char*)&tone_curve,sizeof(AMBA_DSP_IMG_TONE_CURVE_s),0);
				rval = AmbaDSP_ImgSetToneCurve(fd_iav,&ik_mode,&tone_curve);
				CHECK_RVAL
				printf("Tone Curve!\n ");
			}
			break;

		case RGBtoYUVMatrix:
			rval=recv(sock_fd,(char*)&rgb2yuv_matrix,sizeof(AMBA_DSP_IMG_RGB_TO_YUV_s),0);
			rval = AmbaDSP_ImgSetRgbToYuvMatrix(fd_iav,&ik_mode,&rgb2yuv_matrix);
			CHECK_RVAL
			printf("RGB to YUV Matrix!\n");
			break;

		case WhiteBalanceGains:
			rval=recv(sock_fd,(char*)&tmp_wb_gain,sizeof(AMBA_DSP_IMG_WB_GAIN_s),0);
			wb_gain.GainR =tmp_wb_gain.GainR;
			wb_gain.GainG =tmp_wb_gain.GainG;
			wb_gain.GainB=tmp_wb_gain.GainB;
			wb_gain.GlobalDGain =tmp_wb_gain.GlobalDGain;
		//	wb_gain.AeGain =29840;
			rval = AmbaDSP_ImgSetWbGain(fd_iav,&ik_mode, &wb_gain);
			CHECK_RVAL
			printf("White Balance !\n");
			break;

		case DGainSaturaionLevel:
			rval=recv(sock_fd,(char*)&d_gain_satuation_level,sizeof(AMBA_DSP_IMG_DGAIN_SATURATION_s),0);
			rval = AmbaDSP_ImgSetDgainSaturationLevel( fd_iav,&ik_mode,&d_gain_satuation_level);
			CHECK_RVAL
			printf("DGain Saturation Level!\n");
			break;

		case LocalExposure:
			rval=recv(sock_fd,(char*)&local_exposure,sizeof(AMBA_DSP_IMG_LOCAL_EXPOSURE_s),0);
			rval = AmbaDSP_ImgSetLocalExposure(fd_iav,&ik_mode, &local_exposure);
			CHECK_RVAL
			printf("Local Exposure!\n");
			break;

		case ChromaScale:
			rval=recv(sock_fd,(char*)&cs,sizeof(AMBA_DSP_IMG_CHROMA_SCALE_s),0);
			rval = AmbaDSP_ImgSetChromaScale(fd_iav,&ik_mode,&cs);
			CHECK_RVAL
			printf("Chroma Scale!\n");
			break;

		case FPNCorrection://not finished

			break;
		case BadPixelCorrection:
			rval=recv(sock_fd,(char*)&dbp_correction_setting,sizeof(AMBA_DSP_IMG_DBP_CORRECTION_s),0);
			AmbaDSP_ImgSetDynamicBadPixelCorrection(fd_iav,&ik_mode,&dbp_correction_setting);
			printf("Bad Pixel Correction!\n");
			break;

		case CFALeakageFilter:
			rval=recv(sock_fd,(char*)&cfa_leakage_filter,sizeof(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s),0);
			AmbaDSP_ImgSetCfaLeakageFilter(fd_iav,&ik_mode,&cfa_leakage_filter);
			printf("CFA Leakage Filter!\n");
			break;

		case AntiAliasingFilter:
			rval=recv(sock_fd,(char*)&anti_aliasing_strength,sizeof(u8),0);
			AmbaDSP_ImgSetAntiAliasing(fd_iav,&ik_mode,anti_aliasing_strength);
			printf("Anti-Aliasing Filter!\n");
			break;

		case CFANoiseFilter:
			rval=recv(sock_fd,(char*)&cfa_noise_filter,sizeof(AMBA_DSP_IMG_CFA_NOISE_FILTER_s),0);
			AmbaDSP_ImgSetCfaNoiseFilter(fd_iav,&ik_mode,&cfa_noise_filter);
			printf("CFA Noise Filter!\n");
			break;

		case ChromaMedianFiler:
			rval=recv(sock_fd,(char*)&chroma_median_setup,sizeof(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s),0);
			AmbaDSP_ImgSetChromaMedianFilter(fd_iav,&ik_mode,&chroma_median_setup);
			printf("Chroma Median Filer!\n");
			break;

		case SharpeningA_ASF:
		{
			receive_msg(sock_fd,(u8*)&sa_asf_pkg,sizeof(SA_ASF_PKG));
			if(sa_asf_pkg.select_mode==0)
			{
				AmbaDSP_ImgSet1stLumaProcessingMode(fd_iav,&ik_mode,0);
				sa_dsp_asf.Adapt.ThreeD.pTable =tuning_tables.AsfInfoThreeDTable;
				feed_dsp_asf(&sa_asf_pkg.asf_info,&sa_dsp_asf);
				AmbaDSP_ImgSetAdvanceSpatialFilter(fd_iav, &ik_mode, &sa_dsp_asf);
				printf("ASF\n");
			}
			else if(sa_asf_pkg.select_mode ==1)
			{
				AmbaDSP_ImgSet1stLumaProcessingMode(fd_iav,&ik_mode,1);
				sa_dsp_both.ThreeD.pTable =tuning_tables.SharpenBothThreeDTable;
				feed_shp_dsp_both(&sa_asf_pkg.sa_info,&sa_dsp_both);

				AmbaDSP_ImgSet1stSharpenNoiseBoth(fd_iav,&ik_mode,&sa_dsp_both);
				AmbaDSP_ImgSet1stSharpenNoiseNoise(fd_iav,&ik_mode,&sa_asf_pkg.sa_info.noise);
				AmbaDSP_ImgSet1stSharpenNoiseSharpenFir(fd_iav,&ik_mode,&sa_asf_pkg.sa_info.fir);
				AmbaDSP_ImgSet1stSharpenNoiseSharpenCoring(fd_iav,&ik_mode,&sa_asf_pkg.sa_info.coring);
				AmbaDSP_ImgSet1stSharpenNoiseSharpenCoringIndexScale(fd_iav,&ik_mode,&sa_asf_pkg.sa_info.CoringIndexScale);
				AmbaDSP_ImgSet1stSharpenNoiseSharpenMinCoringResult(fd_iav,&ik_mode,&sa_asf_pkg.sa_info.MinCoringResult);
				AmbaDSP_ImgSet1stSharpenNoiseSharpenScaleCoring(fd_iav,&ik_mode,&sa_asf_pkg.sa_info.ScaleCoring);
				printf("SA\n");
			}
			else if(sa_asf_pkg.select_mode== 2)
			{
				AmbaDSP_ImgSet1stLumaProcessingMode(fd_iav,&ik_mode,0);
				printf("disable SharpeningA_ASF\n");
			}
		}
		break;

		case MCTFControl:
			rval=recv(sock_fd,(char*)&mctf_info,sizeof(AMBA_DSP_IMG_VIDEO_MCTF_INFO_s),0);
			AmbaDSP_ImgSetVideoMctf(fd_iav,&ik_mode,&mctf_info);
			printf("MCTF Control!\n");
			break;
		case SharpeningBControl:
			receive_msg(sock_fd,(u8*)&sharpen_pkg,sizeof(SHARPEN_PKG));
			sb_dsp_both.ThreeD.pTable =tuning_tables.FinalSharpenBothThreeDTable;
			feed_shp_dsp_both(&sharpen_pkg,&sb_dsp_both);
			AmbaDSP_ImgSetFinalSharpenNoiseBoth(fd_iav,&ik_mode,&sb_dsp_both);
			AmbaDSP_ImgSetFinalSharpenNoiseNoise(fd_iav,&ik_mode,&sharpen_pkg.noise);
			AmbaDSP_ImgSetFinalSharpenNoiseSharpenFir(fd_iav,&ik_mode,&sharpen_pkg.fir);
			AmbaDSP_ImgSetFinalSharpenNoiseSharpenCoring(fd_iav,&ik_mode,&sharpen_pkg.coring);
			AmbaDSP_ImgSetFinalSharpenNoiseSharpenCoringIndexScale(fd_iav,&ik_mode,&sharpen_pkg.CoringIndexScale);
			AmbaDSP_ImgSetFinalSharpenNoiseSharpenMinCoringResult(fd_iav,&ik_mode,&sharpen_pkg.MinCoringResult);
			AmbaDSP_ImgSetFinalSharpenNoiseSharpenScaleCoring(fd_iav,&ik_mode,&sharpen_pkg.ScaleCoring);
			printf("SharpeningBControl\n");
			break;

		case ColorDependentNoiseReduction:
			rval=recv(sock_fd,(char*)&cdnr_info,sizeof(AMBA_DSP_IMG_CDNR_INFO_s),0);
			AmbaDSP_ImgSetColorDependentNoiseReduction( fd_iav, &ik_mode, &cdnr_info);
			printf("Color Dependent Noise Reduction\n");
			break;

		case ChromaNoiseFilter:
			rval=recv(sock_fd,(char*)&chroma_noise_filter,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
			AmbaDSP_ImgSetChromaFilter(fd_iav,&ik_mode,&chroma_noise_filter);
			printf("Chroma Noise Filter!\n");
			break;
		case TileConfiguration:
		//	rval=recv(sock_fd,(char*)&tile_pkg.stat_config_info,sizeof(TILE_PKG),0);
		//	AmbaDSP_Img3AConfigAaaStat( &ik_mode, &tile_pkg.stat_config_info);
		//	printf("Tile Configuration!\n");
			break;
		case DEMOSAIC:
			rval=recv(sock_fd,(char*)&demosaic_info,sizeof(AMBA_DSP_IMG_DEMOSAIC_s),0);
			AmbaDSP_ImgSetDemosaic(fd_iav, &ik_mode, &demosaic_info);
			printf("Demosaic!\n");
			break;
		case MisMatchGr_Gb:
			rval=recv(sock_fd,(char*)&mismatch_gbgr,sizeof(AMBA_DSP_IMG_GBGR_MISMATCH_s),0);
			AmbaDSP_ImgSetGbGrMismatch(fd_iav,&ik_mode, &mismatch_gbgr);
			printf("MisMatchGr_Gb!\n");
			break;

		case ConfigAAAControl:
			rval=recv(sock_fd,(char*)&aaa_cntl_station,sizeof(aaa_cntl_t),0);
			img_hiso_enable_ae(aaa_cntl_station.ae_enable);
			img_hiso_enable_awb(aaa_cntl_station.awb_enable);
			img_hiso_enable_af(aaa_cntl_station.af_enable);
			img_hiso_enable_adj(aaa_cntl_station.adj_enable);
			printf(" Config AAA Control!\n");
			break;

		case AFStatisticSetupEx:
		//	rval=recv(sock_fd,(char*)&af_statistic_setup_ex,sizeof(af_statistics_ex_t),0);
		//	img_dsp_set_af_statistics_ex(fd_iav,&af_statistic_setup_ex,1);
		//	printf("AF Statistic Setup Ex!\n");
		//	break;

		case ExposureControl:
			rval=recv(sock_fd,(char*)&ec_info,sizeof(EC_INFO),0);
		   	img_hiso_set_sensor_shutter(fd_iav,ec_info.shutter_row);
		  	img_hiso_set_sensor_agc(fd_iav,ec_info.gain_tbl_idx);
			wb_gain.AeGain =ec_info.dgain;
			rval = AmbaDSP_ImgSetWbGain(fd_iav,&ik_mode, &wb_gain);
			printf("Exposure Control!\n");
			break;
		case MCTFMBTemporal:
			rval=recv(sock_fd,(char*)&mctf_mb_temporal,sizeof(AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s),0);
			AmbaDSP_ImgSetVideoMctfMbTemporal(&ik_mode,&mctf_mb_temporal);
			printf("MCTFMBTemporal \n");
			break;
		case MCTFLEVEL:
			rval=recv(sock_fd,(char*)&mctf_level_s,sizeof(AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s),0);
			AmbaDSP_ImgSetVideoMctfLevel(fd_iav,&ik_mode,&mctf_level_s);
			printf("MCTF LEVEL\n");
			break;
		case ITUNER_HISO_ANTI_ALIASING_STRENGTH:
			rval=recv(sock_fd,(char*)&hiso_anti_aliasing_strength,sizeof(u8),0);
			AmbaDSP_ImgSetHighIsoAntiAliasing(&ik_mode,hiso_anti_aliasing_strength);
			printf("ITUNER_HISO_ANTI_ALIASING_STRENGTH\n");
			break;
		case ITUNER_HISO_CFA_LEAKAGE_FILTER:
			rval=recv(sock_fd,(char*)&hiso_lkg,sizeof(AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s),0);
			AmbaDSP_ImgSetHighIsoCfaLeakageFilter(&ik_mode,&hiso_lkg);
			printf("ITUNER_HISO_CFA_LEAKAGE_FILTER\n");
			break;
		case ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION:
			rval=recv(sock_fd,(char*)&hiso_bpc,sizeof(AMBA_DSP_IMG_DBP_CORRECTION_s),0);
			AmbaDSP_ImgSetHighIsoDynamicBadPixelCorrection(&ik_mode,&hiso_bpc);
			printf("ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION\n");
			break;
		case ITUNER_HISO_CFA_NOISE_FILTER:
			rval=recv(sock_fd,(char*)&hiso_cfa,sizeof(AMBA_DSP_IMG_CFA_NOISE_FILTER_s),0);
			AmbaDSP_ImgSetHighIsoCfaNoiseFilter(&ik_mode,&hiso_cfa);
			printf("ITUNER_HISO_CFA_NOISE_FILTER\n");
			break;
		case ITUNER_HISO_GB_GR_MISMATCH:
			rval=recv(sock_fd,(char*)&hiso_gbgrmis,sizeof(AMBA_DSP_IMG_GBGR_MISMATCH_s),0);
			AmbaDSP_ImgSetHighIsoGbGrMismatch(&ik_mode,&hiso_gbgrmis);
			printf("ITUNER_HISO_GB_GR_MISMATCH\n");
			break;
		case ITUNER_HISO_DEMOSAIC_FILTER:
			rval=recv(sock_fd,(char*)&hiso_demosaic,sizeof(AMBA_DSP_IMG_DEMOSAIC_s),0);
			AmbaDSP_ImgSetHighIsoDemosaic(&ik_mode,&hiso_demosaic);
			printf("ITUNER_HISO_DEMOSAIC_FILTER\n");
			break;
		case ITUNER_HISO_CHROMA_MEDIAN_FILTER:
			rval=recv(sock_fd,(char*)&hiso_cmf,sizeof(AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s),0);
			AmbaDSP_ImgSetHighIsoChromaMedianFilter(&ik_mode,&hiso_cmf);
			printf("ITUNER_HISO_CHROMA_MEDIAN_FILTER\n");
			break;
		case ITUNER_HISO_CDNR:
			rval=recv(sock_fd,(char*)&hiso_cdnr,sizeof(AMBA_DSP_IMG_CDNR_INFO_s),0);
			AmbaDSP_ImgSetHighIsoColorDependentNoiseReduction(&ik_mode,&hiso_cdnr);
			printf("ITUNER_HISO_CDNR\n");
			break;
		case ITUNER_HISO_DEFER_COLOR_CORRECTION:
			rval=recv(sock_fd,(char*)&hiso_defer_cc,sizeof(AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s),0);
			AmbaDSP_ImgSetHighIsoDeferColorCorrection(&ik_mode,&hiso_defer_cc);
			printf("ITUNER_HISO_DEFER_COLOR_CORRECTION\n");
			break;
		case ITUNER_HISO_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_asf,sizeof(TUNING_ASF));
			hiso_asf.Adapt.ThreeD.pTable =tuning_tables.HisoAsfThreeDTable;
			feed_dsp_asf(&hiso_tuning_asf,&hiso_asf);
			AmbaDSP_ImgSetHighIsoAdvanceSpatialFilter(&ik_mode,&hiso_asf);
			printf("ITUNER_HISO_ASF\n");
			break;
		case ITUNER_HISO_HIGH_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_high_asf,sizeof(TUNING_ASF));
			hiso_high_asf.Adapt.ThreeD.pTable =tuning_tables.HisoHighAsfThreeDTable;
			feed_dsp_asf(&hiso_tuning_high_asf,&hiso_high_asf);
			AmbaDSP_ImgSetHighIsoHighAdvanceSpatialFilter(&ik_mode,&hiso_high_asf);
			printf("ITUNER_HISO_HIGH_ASF\n");
			break;
		case ITUNER_HISO_LOW_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_low_asf,sizeof(TUNING_ASF));
			hiso_low_asf.Adapt.ThreeD.pTable =tuning_tables.HisoLowAsfThreeDTable;
			feed_dsp_asf(&hiso_tuning_low_asf,&hiso_low_asf);
			AmbaDSP_ImgSetHighIsoLowAdvanceSpatialFilter(&ik_mode,&hiso_low_asf);
			printf("ITUNER_HISO_LOW_ASF\n");
			break;
		case ITUNER_HISO_MED1_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_med1_asf,sizeof(TUNING_ASF));
			hiso_med1_asf.Adapt.ThreeD.pTable =tuning_tables.HisoMed1AsfThreeDTable;
			feed_dsp_asf(&hiso_tuning_med1_asf,&hiso_med1_asf);
			AmbaDSP_ImgSetHighIsoMed1AdvanceSpatialFilter(&ik_mode,&hiso_med1_asf);
			printf("ITUNER_HISO_MED1_ASF\n");
			break;
		case ITUNER_HISO_MED2_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_med2_asf,sizeof(TUNING_ASF));
			hiso_med2_asf.Adapt.ThreeD.pTable =tuning_tables.HisoMed2AsfThreeDTable;
			feed_dsp_asf(&hiso_tuning_med2_asf,&hiso_med2_asf);
			AmbaDSP_ImgSetHighIsoMed2AdvanceSpatialFilter(&ik_mode,&hiso_med2_asf);
			printf("ITUNER_HISO_MED2_ASF\n");
			break;
		case ITUNER_HISO_LI2ND_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_li2nd_asf,sizeof(TUNING_ASF));
			hiso_li2nd_asf.Adapt.ThreeD.pTable =tuning_tables.HisoLi2ndAsfThreeDTable;
			feed_dsp_asf(&hiso_tuning_li2nd_asf,&hiso_li2nd_asf);
			AmbaDSP_ImgSetHighIsoLi2ndAdvanceSpatialFilter(&ik_mode,&hiso_li2nd_asf);
			printf("ITUNER_HISO_LI2ND_ASF\n");
			break;

		case ITUNER_HISO_CHROMA_ASF:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_chroma_asf,sizeof(TUNING_IMG_CHROMA_ASF_INFO_s));
			hiso_chroma_asf.ThreeD.pTable=tuning_tables.HisoChromaAsfThreeDTable;
			feed_dsp_chroma_asf(&hiso_tuning_chroma_asf,&hiso_chroma_asf);
			AmbaDSP_ImgSetHighIsoChromaAdvanceSpatialFilter(&ik_mode,&hiso_chroma_asf);
			printf("ITUNER_HISO_CHROMA_ASF\n");
			break;
		case ITUNER_HISO_SHARPEN_HI_HIGH:
			rval=receive_msg(sock_fd,(u8*)&hiso_high_sharpen_pkg,sizeof(SHARPEN_PKG));
			liso_high_sharpen_both.ThreeD.pTable =tuning_tables.HisoHighSharpenBothThreeDTable;
			feed_shp_dsp_both(&hiso_high_sharpen_pkg,&liso_high_sharpen_both);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseBoth(&ik_mode,&liso_high_sharpen_both);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseNoise(&ik_mode,&hiso_high_sharpen_pkg.noise);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenFir(&ik_mode,&hiso_high_sharpen_pkg.fir);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoring(&ik_mode,&hiso_high_sharpen_pkg.coring);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_high_sharpen_pkg.CoringIndexScale);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_high_sharpen_pkg.MinCoringResult);
			AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_high_sharpen_pkg.ScaleCoring);
			printf("ITUNER_HISO_SHARPEN_HI_HIGH\n");
			break;
		case ITUNER_HISO_SHARPEN_HI_MED:
			rval=receive_msg(sock_fd,(u8*)&hiso_med_sharpen_pkg,sizeof(SHARPEN_PKG));
			liso_med_sharpen_both.ThreeD.pTable =tuning_tables.HisoMedSharpenBothThreeDTable;
			feed_shp_dsp_both(&hiso_med_sharpen_pkg,&liso_med_sharpen_both);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseBoth(&ik_mode,&liso_med_sharpen_both);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseNoise(&ik_mode,&hiso_med_sharpen_pkg.noise);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenFir(&ik_mode,&hiso_med_sharpen_pkg.fir);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoring(&ik_mode,&hiso_med_sharpen_pkg.coring);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_med_sharpen_pkg.CoringIndexScale);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_med_sharpen_pkg.MinCoringResult);
			AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_med_sharpen_pkg.ScaleCoring);
			printf("ITUNER_HISO_SHARPEN_HI_MED\n");
			break;
		case ITUNER_HISO_SHARPEN_HI_LI:
			rval=receive_msg(sock_fd,(u8*)&hiso_li_sharpen_pkg,sizeof(SHARPEN_PKG));
			liso_li_sharpen_both.ThreeD.pTable =tuning_tables.HisoLiso1SharpenBothThreeDTable;
			feed_shp_dsp_both(&hiso_li_sharpen_pkg,&liso_li_sharpen_both);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseBoth(&ik_mode,&liso_li_sharpen_both);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseNoise(&ik_mode,&hiso_li_sharpen_pkg.noise);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenFir(&ik_mode,&hiso_li_sharpen_pkg.fir);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenCoring(&ik_mode,&hiso_li_sharpen_pkg.coring);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_li_sharpen_pkg.CoringIndexScale);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_li_sharpen_pkg.MinCoringResult);
			AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_li_sharpen_pkg.ScaleCoring);
			printf("ITUNER_HISO_SHARPEN_HI_LI\n");
			break;
		case ITUNER_HISO_SHARPEN_HI_LI2ND:
			rval=receive_msg(sock_fd,(u8*)&hiso_li2nd_sharpen_pkg,sizeof(SHARPEN_PKG));
			liso_li2nd_sharpen_both.ThreeD.pTable =tuning_tables.HisoLiso2SharpenBothThreeDTable;
			feed_shp_dsp_both(&hiso_li2nd_sharpen_pkg,&liso_li2nd_sharpen_both);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseBoth(&ik_mode,&liso_li2nd_sharpen_both);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseNoise(&ik_mode,&hiso_li2nd_sharpen_pkg.noise);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenFir(&ik_mode,&hiso_li2nd_sharpen_pkg.fir);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenCoring(&ik_mode,&hiso_li2nd_sharpen_pkg.coring);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenCoringIndexScale(&ik_mode,&hiso_li2nd_sharpen_pkg.CoringIndexScale);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenMinCoringResult(&ik_mode,&hiso_li2nd_sharpen_pkg.MinCoringResult);
			AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenScaleCoring(&ik_mode,&hiso_li2nd_sharpen_pkg.ScaleCoring);
			printf("ITUNER_HISO_SHARPEN_HI_LI2ND\n");
			break;

		case ITUNER_HISO_CHROMA_FILTER_HIGH:
			rval=recv(sock_fd,(char*)&hiso_chroma_filter,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
			AmbaDSP_ImgHighIsoSetChromaFilterHigh(&ik_mode,&hiso_chroma_filter);
			printf("ITUNER_HISO_CHROMA_FILTER_HIGH\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW:
			rval=recv(sock_fd,(char*)&hiso_chroma_lowverylow_filter,sizeof(AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s),0);
			AmbaDSP_ImgHighIsoSetChromaFilterLowVeryLow(&ik_mode,&hiso_chroma_lowverylow_filter);
			printf("ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_PRE:
			rval=recv(sock_fd,(char*)&hiso_chroma_filter_pre,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
			AmbaDSP_ImgHighIsoSetChromaFilterPre(&ik_mode,&hiso_chroma_filter_pre);
			printf("ITUNER_HISO_CHROMA_FILTER_PRE\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_MED:
			rval=recv(sock_fd,(char*)&hiso_chroma_filter_med,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
			AmbaDSP_ImgHighIsoSetChromaFilterMed(&ik_mode,&hiso_chroma_filter_med);
			printf("ITUNER_HISO_CHROMA_FILTER_MED\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_LOW:
			rval=recv(sock_fd,(char*)&hiso_chroma_filter_low,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
			AmbaDSP_ImgHighIsoSetChromaFilterLow(&ik_mode,&hiso_chroma_filter_low);
			printf("ITUNER_HISO_CHROMA_FILTER_LOW\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_VERY_LOW:
			rval=recv(sock_fd,(char*)&hiso_chroma_filter_verylow,sizeof(AMBA_DSP_IMG_CHROMA_FILTER_s),0);
			AmbaDSP_ImgHighIsoSetChromaFilterVeryLow(&ik_mode,&hiso_chroma_filter_verylow);
			printf("ITUNER_HISO_CHROMA_FILTER_VERY_LOW\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_MED_COMBINE:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_chroma_filter_combine_med,sizeof(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s));
			hiso_chroma_filter_combine_med.ThreeD.pTable =tuning_tables.HisoChromaFilterMedCombineThreeDTable;
			feed_dsp_chroma_filter_combine(&hiso_tuning_chroma_filter_combine_med,&hiso_chroma_filter_combine_med);
			AmbaDSP_ImgHighIsoSetChromaFilterMedCombine(&ik_mode,&hiso_chroma_filter_combine_med);
			printf("ITUNER_HISO_CHROMA_FILTER_MED_COMBINE\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_chroma_filter_combine_low,sizeof(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s));
			hiso_chroma_filter_combine_low.ThreeD.pTable =tuning_tables.HisoChromaFilterLowCombineThreeDTable;
			feed_dsp_chroma_filter_combine(&hiso_tuning_chroma_filter_combine_low,&hiso_chroma_filter_combine_low);
			AmbaDSP_ImgHighIsoSetChromaFilterLowCombine(&ik_mode,&hiso_chroma_filter_combine_low);
			printf("ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE\n");
			break;
		case ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE:

			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_chroma_filter_combine_verylow,sizeof(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s));
			hiso_chroma_filter_combine_verylow.ThreeD.pTable =tuning_tables.HisoChromaFilterVeryLowCombineThreeDTable;
			feed_dsp_chroma_filter_combine(&hiso_tuning_chroma_filter_combine_verylow,&hiso_chroma_filter_combine_verylow);
			AmbaDSP_ImgHighIsoSetChromaFilterVeryLowCombine(&ik_mode,&hiso_chroma_filter_combine_verylow);
			printf("ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE\n");
			break;

		case ITUNER_HISO_LUMA_NOISE_COMBINE:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_luma_noise_combine,sizeof(TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s));
			hiso_luma_noise_combine.ThreeD.pTable =tuning_tables.HisoLumaNoiseCombineThreeDTable;
			feed_dsp_luma_noise_combine(&hiso_tuning_luma_noise_combine,&hiso_luma_noise_combine);
			AmbaDSP_ImgHighIsoSetLumaNoiseCombine(&ik_mode,&hiso_luma_noise_combine);
			printf("ITUNER_HISO_LUMA_NOISE_COMBINE\n");
			break;
		case ITUNER_HISO_LOW_ASF_COMBINE:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_low_asf_combine,sizeof(TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s));
			hiso_low_asf_combine.ThreeD.pTable =tuning_tables.HisoLowASFCombineThreeDTable;
			feed_dsp_luma_noise_combine(&hiso_tuning_low_asf_combine,&hiso_low_asf_combine);
			AmbaDSP_ImgHighIsoSetLowASFCombine(&ik_mode,&hiso_low_asf_combine);
			printf("ITUNER_HISO_LOW_ASF_COMBINE\n");
			break;
		case ITUNER_HIGH_ISO_COMBINE:
			rval=receive_msg(sock_fd,(u8*)&hiso_tuning_high_combine,sizeof(TUNING_IMG_HISO_COMBINE_s));
			hiso_high_combine.ThreeD.pTable =tuning_tables.HighIsoCombineThreeDTable;
			feed_dsp_high_combine(&hiso_tuning_high_combine,&hiso_high_combine);
			AmbaDSP_ImgHighIsoSetHighIsoCombine(&ik_mode,&hiso_high_combine);
			printf("ITUNER_HIGH_ISO_COMBINE\n");
			break;
		case ITUNER_HISO_FREQ_RECOVER:
			rval=recv(sock_fd,(char*)&hiso_freq_recover,sizeof(AMBA_DSP_IMG_HISO_FREQ_RECOVER_s),0);
			AmbaDSP_ImgHighIsoSetHighIsoFreqRecover(&ik_mode,&hiso_freq_recover);
			printf("ITUNER_HISO_FREQ_RECOVER\n");
			break;
		case ITUNER_HISO_LOW2_LUMA_BLEND://TODO
			AmbaDSP_ImgHighIsoSetHighIsoLumaBlend(&ik_mode,&hiso_luma_blend);
			printf("ITUNER_HISO_LOW2_LUMA_BLEND\n");
			break;
	}
	}
	break;
	default:
		printf("undefined\n");
	}
}
static int process_tuning_id(int sock_fd,TUNING_ID* p_tuning_id)
{
	switch(p_tuning_id->req_id)
	{
	case APPLY:
		process_apply(sock_fd,p_tuning_id);
		break;
	case LOAD:
		process_load(sock_fd,p_tuning_id);
		break;
	case OTHER:
		process_other(sock_fd,p_tuning_id);
		break;
	default:
		printf("Unknown req_id [%c]\n",p_tuning_id->req_id);
		break;
	}
	return 0;
}
static const char* short_options = "lh";
static struct option long_options[] = {
	{"liso", 0, 0, 'l'},
	{"hiso", 0, 0, 'h'},
};
static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(ch) {
		case 'l':
			tuning_mode = LISO;
			break;
		case 'h':
			tuning_mode = HISO;
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}
	return 0;
}
struct hint_s {
	const char *arg;
	const char *str;
};
static const struct hint_s hint[] = {
	{"-l", "\tliso tuning"},
	{"-h", "\thiso tuning"},
};
static void usage()
{
	int cnt = sizeof(hint)/sizeof(hint[0]);
	int i;
	for(i=0; i<cnt; i++) {
		printf("%s %s\n", hint[i].arg, hint[i].str);
	}
}
int main(int argc, char **argv)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	if(argc<=1){
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		return -1;
	}
	img_hiso_lib_init(fd_iav,1,1);
	if(start_aaa_init(fd_iav)<0)
		return -1;
	init_tuning_table();
#ifndef DSP_DEBUG
	img_hiso_start_aaa(fd_iav);
	img_hiso_set_work_mode(0);
#endif

#ifdef DSP_DEBUG

	AMBA_DSP_IMG_SENSOR_INFO_s SensorInfo;
	SensorInfo.SensorID = 0;
	SensorInfo.NumFieldsPerFormat = 1;
	SensorInfo.SensorResolution = 14;
	SensorInfo.SensorPattern = 3;
	SensorInfo.SensorReadOutMode = 0;
	AmbaDSP_ImgSetVinSensorInfo(fd_iav, &ik_mode, &SensorInfo);
	AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s def_cc;
	def_cc.Enable = 0;
	AmbaDSP_ImgSetHighIsoDeferColorCorrection(&ik_mode, &def_cc);
	AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s vig_pass;
	memset(&vig_pass, 0, sizeof(vig_pass));
	AmbaDSP_ImgSetVignetteCompensationByPass(&ik_mode, &vig_pass);

	AMBA_DSP_IMG_DGAIN_SATURATION_s s_lvl;
	s_lvl.LevelGreenEven = s_lvl.LevelGreenOdd = s_lvl.LevelBlue = s_lvl.LevelRed = 16383;
	AmbaDSP_ImgSetDgainSaturationLevel(fd_iav, &ik_mode, &s_lvl);

	AMBA_DSP_IMG_LOCAL_EXPOSURE_s le;
	memset(&le, 0, sizeof(le));
	AmbaDSP_ImgSetLocalExposure(fd_iav, &ik_mode, &le);

//	u8 CDNRtoneCurveSelect =1;
//	AmbaDSP_ImgSetCDNRToneCurveSelect(&ik_mode,&CDNRtoneCurveSelect);

#endif

	int sockfd =-1;
	int new_fd =-1;
	struct sockaddr_in  their_addr;
	sockfd=SockServer_Setup(sockfd, ALL_ITEM_SOCKET_PORT);
	while(1)
	{

		if(new_fd!=-1)
			close(new_fd);
		socklen_t sin_size=sizeof(struct sockaddr_in);
		signal(SIGFPE, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		if((new_fd=accept(sockfd,(struct sockaddr*)&their_addr,&sin_size))==-1)
		{
			perror("accapt");
			continue;
		}
		TUNING_ID tuning_id;
		int rev =-1;
		rev=recv(new_fd,(char*)&tuning_id,sizeof(TUNING_ID),0);
		if(rev <0)
		{
			printf("recv error[%d]\n",rev);
			break;
		}
	//	printf("size %d,require %d,item %d,tab %d\n",sizeof(TUNING_ID),
	//		tuning_id.req_id,tuning_id.item_id,tuning_id.tab_id);
		process_tuning_id(new_fd,&tuning_id);
	}
	SockServer_free(sockfd,new_fd);
	return 0;

}

