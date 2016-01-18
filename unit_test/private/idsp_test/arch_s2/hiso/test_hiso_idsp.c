#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#include "img_abs_filter.h"


#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"
#include "AmbaDSP_ImgUtility.h"
#include "AmbaDSP_ImgFilter.h"
#include "AmbaDSP_ImgHighIsoFilter.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "load_param.c"
#define	IMGPROC_PARAM_PATH	"/etc/idsp"
static int fd_iav;
AMBA_DSP_IMG_MODE_CFG_s ik_mode;
u8 aaa_enable =0;
static int tuning_mode =AMBA_DSP_IMG_ALGO_MODE_FAST;


static const char* short_options = "a";
static struct option long_options[] = {
	{"3a", 0, 0, 'a'},

};

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(ch) {
		case 'a':
			aaa_enable = 1;
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
	{"-a", "\tenable 3a"},
};
static void usage()
{
	int cnt = sizeof(hint)/sizeof(hint[0]);
	int i;
	for(i=0; i<cnt; i++) {
		printf("%s %s\n", hint[i].arg, hint[i].str);
	}
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
	return 0;

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
	img_hiso_lib_init(fd_iav,0,1);
	if(start_aaa_init(fd_iav)<0)
		return -1;

	img_hiso_start_aaa(fd_iav);
	img_hiso_set_work_mode(0);
	while(1) {
		getchar();
	}
#if 0
	int fd_iav;
	AMBA_DSP_IMG_MODE_CFG_s ik_mode;

	memset(&ik_mode, 0, sizeof(ik_mode));
	ik_mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
	ik_mode.ConfigId = 0;
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	init_img_dsp(fd_iav);
	load_containers();
	load_cc_bin("mn34210pl");
	enable_cc(fd_iav,"mn34210pl",&ik_mode);
	img_adj_reset_filters();



	AMBA_DSP_IMG_WB_GAIN_s wb_gain;
	wb_gain.AeGain = 4096;
	wb_gain.GlobalDGain = 4096;
	wb_gain.GainG = 4096;
	wb_gain.GainR = 8400;
	wb_gain.GainB = 8800;
	img_adj_init_misc(fd_iav, &ik_mode, &wb_gain,GR);
	img_adj_retrieve_filters(fd_iav, &ik_mode);


//	AMBA_DSP_IMG_DEF_BLC_s def_blc;
//	def_blc.Enb = 1;
//	AmbaDSP_ImgSetDeferredBlackLevel(fd_iav, &ik_mode, &def_blc);




	ioctl(fd_iav, IAV_IOC_IMG_HISO_TIMING);
	AmbaDSP_ImgPostExeCfg(fd_iav, &ik_mode, 2, 1);
	ik_mode.ConfigId = 1;
	AmbaDSP_ImgPostExeCfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
	getchar();


	while(1){
		img_runtime_adj(fd_iav, &ik_mode);

		AMBA_DSP_IMG_HISO_COMBINE_s hili_combine;
		AmbaDSP_ImgHighIsoGetHighIsoCombine(&ik_mode, &hili_combine);
		hili_combine.MaxChangeY = 255*ik_mode.ConfigId;
		AmbaDSP_ImgHighIsoSetHighIsoCombine(&ik_mode, &hili_combine);
//		AMBA_DSP_IMG_VIDEO_MCTF_INFO_s video_mctf;
//		AmbaDSP_ImgGetVideoMctf(&ik_mode,&video_mctf);
//		video_mctf.YCombinedStrength = 255*ik_mode.ConfigId;
//		AmbaDSP_ImgSetVideoMctf(fd_iav, &ik_mode,&video_mctf);
		ioctl(fd_iav, IAV_IOC_IMG_HISO_TIMING);
		AmbaDSP_ImgPostExeCfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 0);
		ik_mode.ConfigId ^= 1;
		AmbaDSP_ImgPostExeCfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
		getchar();
	}

/*	AMBA_DSP_IMG_CFG_INFO_s CfgInfo;
	CfgInfo.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	CfgInfo.CfgId = 0;
	AmbaDSP_ImgHighIsoDumpCfg(CfgInfo, "/mnt/cfg");
	AmbaDSP_ImgHighIsoPrintCfg(&ik_mode, CfgInfo);*/
//	img_runtime_adj(fd_iav, &ik_mode);


//	set all necessary fitlers
//	enable 3

	return 0;

#endif

}

