#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "unistd.h"
#include "img_struct_arch.h"
#include "img_dsp_interface_arch.h"
#include "ambas_imgproc_arch.h"
#include "img_api_arch.h"
#include "time.h"
#include <sched.h>
#include"math.h"
#include <sys/ioctl.h>
#include "ambas_vin.h"
#include<pthread.h>
/////////////////////////////////////////////////////////////////////
//used to check ae timing when flash bug happened
////////////////////////////////////////////////////////////////////
extern int hal_get_vin_tick();
extern int hal_init_vin_tick();
extern int get_tick(void);
u32 shutter_index;
u32 dgain;
u32 agc_index;
static aaa_tile_report_t aaa_tile_get_info;//img_dsp_get_aaainfo

static img_aaa_stat_t aaa_stat_info;

static embed_hist_stat_t aaa_hist_info;
static aaa_tile_report_t act_tile;
static histogram_stat_t dsp_histo_info;

static statistics_config_t is_tile_config =
{
	1,
	1,

	24,
	16,
	128,
	48,
	160,
	250,
	160,
	250,
	0,
	16383,

	12,
	8,
	0,
	8,
	340,
	512,

	8,
	5,
	256,
	10,
	448,
	816,
	448,
	816,

	0,
	16383,
};
static int refer_shutter_timing=2 ;
static int refer_agc_timing =1;
static int refer_dgain_timing=1;
static u8 vin_id_exit_flag =0;
static u8 exit_flag =0;
static u32 frame_n =0;
static u32 luma_array[100];
static u32 rgb_luma_array[100];
static int t_vdsp[100];
static int t_vin[100];
static void vin_loop(void* arg)
{
	printf("vin_loop\n");
	int fd_iav;
	fd_iav = (int)arg;
	while(!vin_id_exit_flag)
	{
		hal_get_vin_tick();
		int t =get_tick();
		printf("vin_t=%d,frame=%d\n",t,++frame_n);
		t_vin[frame_n] =t;
		if(frame_n ==10)
		{
			img_set_sensor_shutter_index(fd_iav, 1234);
			printf("t=%d,set shutter =1/100s\n",get_tick());
		}
		else if(frame_n ==20)
		{
			img_set_sensor_shutter_index(fd_iav, 1106);
			printf("t=%d,set shutter =1/50s\n",get_tick());
		}
		else if(frame_n ==30)
		{
			img_set_sensor_agc_index(fd_iav, 0,16);
			printf("t=%d,set agc =0db\n",get_tick());
		}
		else if(frame_n ==40)
		{
			img_set_sensor_agc_index(fd_iav, 128,16);
			printf("t=%d,set agc =6db\n",get_tick());
		}
		else if(frame_n ==50)
		{
			img_dsp_set_dgain(fd_iav,1024);
			printf("t=%d,set dgain =1024\n",get_tick());
		}
		else if(frame_n ==60)
		{
			img_dsp_set_dgain(fd_iav,4096);
			printf("t=%d,set dgain =4096\n",get_tick());
		}
		else if(frame_n ==70)
			vin_id_exit_flag =1;
	}
	printf("exit vin_loop\n");
	return;
}

static void get_luma_stat(int fd_iav,u32 *luma,u32 *rgb_luma)
{
	img_dsp_get_statistics(fd_iav,&aaa_stat_info ,&aaa_tile_get_info);
	int tile_num =aaa_tile_get_info.ae_tile;
	int i=0;
	u32 luma_sum =0,rgb_luma_sum=0;
	for(i = 0; i < tile_num; i++) {

		luma_sum += aaa_stat_info.ae_info[i].lin_y;
		rgb_luma_sum += aaa_stat_info.ae_info[i].non_lin_y;
	}
	*luma=luma_sum/tile_num;
	*rgb_luma =rgb_luma_sum /tile_num;
}
static void calc_result(u32* luma_arr,u32* rgb_luma_arr,u8 type)// type=1,0
{
//	printf("type =%d\n",type);
	int i =0;
	for(i=1;i<4;i++)
	{
		u32 index,base,rgb_base;
		index =i*20;
		base =luma_arr[index];
		rgb_base=rgb_luma_arr[index];
		for(index =i*20;index<i*20+10;index++)
		{
			if(luma_arr[index]>base*3/2||rgb_luma_arr[index]-rgb_base>20)
			{
				if(i ==1)
					printf("shutter timing: N+%d......reference: N+%d\n",index -i*20-type,refer_shutter_timing);
				else if(i ==2)
					printf("agc timing: N+%d......reference: N+%d\n",index -i*20-type,refer_agc_timing);
				else if(i ==3)
					printf("dgain timing: N+%d......reference: N+%d\n",index -i*20-type,refer_dgain_timing);
			//	printf("i=%d,index=%d,luma_arr[index] =%d,base =%d\n",i,index,luma_arr[index],base);
				break;
			}
		}
	}
}
static int kbhit(void)
{
	fd_set rfds;
	struct timeval tv;
	int retval;
	FD_ZERO(&rfds);
	FD_SET(0,&rfds);

	tv.tv_sec=1;
	tv.tv_usec=0;
	retval = select(1, &rfds, NULL, NULL, &tv);
	if (retval == -1) {
		perror("select()");
		return -1;
	}
	else if (retval)
	{
		return 1;
	}
	return 0;
}

int main( )
{
	int fd_iav;
	if(img_lib_init(0,0)<0) {
		perror("/dev/iav");
		return -1;
	}

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	struct amba_vin_source_info vin_info;
	if (ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_INFO, &vin_info) < 0) {
		printf("IAV_IOC_VIN_SRC_GET_INFO error\n");
		return -1;
	}

	if (vin_info.sensor_id ==SENSOR_MT9T002||
	vin_info.sensor_id ==SENSOR_AR0331||
	vin_info.sensor_id ==SENSOR_AR0130)
	{
		refer_shutter_timing =2;
		refer_agc_timing =2;
	}
	hal_init_vin_tick();


	img_dsp_config_statistics_info(fd_iav, &is_tile_config);

	pthread_t vin_id;
	img_set_sensor_agc_index(fd_iav, 0,16);
	img_set_sensor_shutter_index(fd_iav, 1234);
	img_dsp_set_dgain(fd_iav,1024);

	while(1)
	{
		u32 curr_luma =0,curr_rgb_luma =0;
		get_luma_stat(fd_iav,&curr_luma,&curr_rgb_luma);
		printf("current luma=%d, pls make luma =800--->1200.press 'n' key to continue\n",curr_luma);
		if(kbhit()){
			char key =getchar();
			if(key =='n')
				break;
		}
	}
	printf("\n\n\n >>>>>>>test start now!!!<<<<<\n\n");

	if(pthread_create(&vin_id, NULL, (void*)vin_loop, (void*)fd_iav)!=0)
		printf("thread create failed!\n");
	while(!exit_flag)
	{
		u32 luma =0,rgb_luma =0;
		get_luma_stat(fd_iav,&luma,&rgb_luma);
		luma_array[frame_n] =luma;
		rgb_luma_array[frame_n] =rgb_luma;
		int t =get_tick();
		printf("t=%d, frame =%d,luma =%d, rgb =%d\n\n",t,frame_n,luma,rgb_luma);
		t_vdsp[frame_n] =t;
		if(frame_n ==70)
			exit_flag =1;
	}
	u8 type =0;
	printf("t_vdsp =%d,t_vin =%d\n",t_vdsp[20],t_vin[20]);
	if(t_vdsp[20] -t_vin[20]<10)//10ms is a threshold,should be [(frame_time -vb)-offset]
		type =1;
	calc_result(luma_array,rgb_luma_array,type);

	printf("if the result is not the same as reference, pls run test_soft_vsync to check vsync first!");
	return 0;

}
