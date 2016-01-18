#ifndef __AMBA_IMGPROC_H__
#define __AMBA_IMGPROC_H__


int img_set_phys_start_ptr(u32 input);
int img_need_remap(int remap);
int amba_imgproc_cmd(iav_context_t * context, unsigned int cmd, unsigned long arg);
int amba_imgproc_msg(VCAP_STRM_REPORT * str_rpt, VCAP_SETUP_MODE mode);

#endif	// __AMBA_IMGPROC_H__
