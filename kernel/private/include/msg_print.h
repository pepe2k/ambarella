#ifndef __PRINT_PRIVATE_DRV__
#define __PRINT_PRIVATE_DRV__
extern int print_drv(const char *fmt, ...);
extern int do_drv_syslog(int type, char __user *buf, int len, bool from_file);
#endif
