

#ifndef __MW_IMAGE_PRIV_H__
#define __MW_IMAGE_PRIV_H__

//define in mw_image_hiso.c
extern _mw_global_config		G_mw_config;
extern mw_cali_file		G_mw_cali;
extern u8 *iso_cc_reg;
extern u8 *iso_cc_matrix;

//define in mw_image_priv_hiso.c
inline int check_state(void);
int check_aaa_state(int fd_iav);

int do_prepare_hiso_aaa(void);
int do_start_hiso_aaa(void);
int do_stop_hiso_aaa(void);

int get_ae_exposure_lines(mw_ae_line *lines, u32 num);
int set_ae_exposure_lines(mw_ae_line *lines, u32 num);
int load_ae_exp_lines(mw_ae_param *ae);
int create_slowshutter_task(void);
int destroy_slowshutter_task(void);

//define in mw_netlink.c
int init_netlink(void);
void * netlink_loop(void * data);
int deinit_netlink(void);

//define in load_hiso_paramc.
int load_iso_cc_bin(char * sensor_name);
int enable_iso_cc(char* sensor_name, int mode);
int load_iso_containers(u32 sensor_id);
int load_img_iso_aaa_config(u32 sensor_id);


#endif // __MW_IMAGE_PRIV_H__
