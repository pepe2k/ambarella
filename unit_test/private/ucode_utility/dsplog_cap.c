/*
 * dsplog_cap.c
 *
 * History:
 *	2012/05/05 - [Jian Tang] created file
 * 	2013/09/26 - [Louis Sun] improve it to prevent data loss
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sched.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "types.h"
#include "amba_debug.h"
#include "iav_drv.h"
#include "dsplog_drv.h"
#include "datatx_lib.h"  //use datax lib to transfer data

#include <sys/syscall.h>

int maxlogs = 0;

#define AMBA_DEBUG_DSP	(1 << 1)
#define MAX_LOG_READ_SIZE   (1024*1024)
//#define ENABLE_RT_SCHED


static char dsp_ucode_dir[512] ="/lib/firmware";

static int dsp_debug_module_id[8] = {-1,-1,-1,-1,-1,-1,-1,-1};   //debug 8 modules at most, -1 means not selected
static int  dsp_debug_module_id_num = 0;
static int  dsp_debug_module_id_last = 0;

static int  dsp_debug_level = 3;
static int  dsp_debug_level_flag = 0;
static int dsp_debug_thread_bitmask = 0;
static int dsp_debug_thread_bitmask_flag = 0;

static int dsp_log_work_mode= 0; // 0: capture log  1: parse log

static char dsp_log_output_filename[512] = "/tmp/dsplog.dat";       //default dsplog file if not specified
static int dsp_log_output_filename_flag = 0;
static char dsp_log_input_filename[512] = "/tmp/dsplog.dat";        //default dsplog file if not specified
static int dsp_log_input_filename_flag = 0;
static int dsp_log_transfer_tcp_flag = 0;   //use DATAX lib to do transfer to PC
#define DSP_LOG_PORT            2017        //use port 2032 for tcp transfer

static int dsp_debug_bit_mask[8] = {0, 0, 0, 0,0, 0, 0, 0}; //debug_bit_mask is only supported on iOne/S2

static int dsp_log_capture_current_flag = 0;
static char dsp_log_parse_output_filename[512] = "";       //default dsplog file if not specified

static int dsp_log_transfer_fd = -1;
static int dsp_log_transfer_method = 0;


static int dsp_log_show_ucode_version_flag = 0;

#define DSPLOG_VERSION_STRING  "1.5"
static int dsp_log_show_version_flag = 0;

#define DEFAULT_DSP_LOG_PINPONG_FILE_SIZE    (100*1024*1024)
static int dsp_log_pinpong_flag = 0;                                //default it is not pinpong mode.
static int dsp_log_pinpong_filesize_limit = DEFAULT_DSP_LOG_PINPONG_FILE_SIZE;   //default pinpong each file size is 100MB
static int dsp_log_pinpong_state =  0;   //0 or 1
static int dsp_log_pinpong_filesize_now = 0;

static int dsp_log_operation_add[8] = {0, 0, 0, 0, 0, 0, 0, 0 };


//dsplog verify
static int dsp_log_verify_flag = 0;
static char dsp_log_verify_filename[512]="";


//struct idsp_printf_s is compatible for all ARCH
typedef struct idsp_printf_s {
	u32 seq_num;		/**< Sequence number */
	u8  dsp_core;
	u8  thread_id;
	u16 reserved;
	u32 format_addr;	/**< Address (offset) to find '%s' arg */
	u32 arg1;		/**< 1st var. arg */
	u32 arg2;		/**< 2nd var. arg */
	u32 arg3;		/**< 3rd var. arg */
	u32 arg4;		/**< 4th var. arg */
	u32 arg5;		/**< 5th var. arg */
} idsp_printf_t;



static sem_t exit_main_sem;


//include platform related config file
#include <config.h>
#ifdef CONFIG_ARCH_A5S
#include "arch_a5s/dsplog_utils.c"
#elif CONFIG_ARCH_I1
#include "arch_i1/dsplog_utils.c"
#elif CONFIG_ARCH_S2
#include "arch_s2/dsplog_utils.c"
#elif CONFIG_ARCH_A7L
#include "arch_a7l/dsplog_utils.c"
#endif


#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{"module",	HAS_ARG,	0,	'm'},     //must be string.
	{"level",		HAS_ARG,	0,	'l'},       //debug level 0: OFF 1: NORMAL 2: VERBOSE 3: DEBUG
	{"thread",	HAS_ARG,	0,	'r'},       //coding orc thread
       {"outfile",	HAS_ARG,	0,	'o'},      //output file (for log capture)
       {"infile",	       HAS_ARG,	0,	'i'},       //input file (for parse log)
       {"tcp",	       NO_ARG,	       0,	't'},       //use tcp to transfer dsp log
	{"bitmask",      HAS_ARG,       0 ,   'b' },  //debug bitmask
	{"ucode_dir",   HAS_ARG,       0 ,   'd' },  //use different ucode dir to parse log
	{"current",     NO_ARG,          0 ,   'c' },   // only capture log in dsp buf only (small log)
	{"logtext",       HAS_ARG,      0,     'f' },  //  log text file (after parse)
	{"pinpong",      HAS_ARG,      0,    'p' },  //  write log to two files, by pinpong mode, each file is limited to size specified by this option, for example, each file is 100MB,
	{"ucode_ver",   NO_ARG,        0,   'v' },                                                           // when file1 has reached the size, go to truncate and write file2, and when file2 reaches the size, go back to truncate and write to file1
	{"version",       NO_ARG,        0,   'w' },                                                           // when file1 has reached the size, go to truncate and write file2, and when file2 reaches the size, go back to truncate and write to file1
	{"verify",       HAS_ARG,          0,   'y' },                                                           // when file1 has reached the size, go to truncate and write file2, and when file2 reaches the size, go back to truncate and write to file1
	{"add",              NO_ARG,         0,    'a'},        //"Add or Set" operation to debug mask.  when specified, it's Add.   else it's set
     	{0, 0, 0, 0}
       };

static const char *short_options = "b:m:l:r:o:i:td:cf:p:vwy:a";

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"string", "\tcommon, vcap, vout, encoder, decoder,  idsp, memd, api, perf, all"},
	{"0~3", "\tdebug level 0: OFF 1: NORMAL 2: VERBOSE 3: DEBUG"},
	{"0~255", "\tDSP thread bitmask, decimal or hex (start with 0x), set bit to 0 to enable"},
	{"filename", "\tfilename of dsplog binary to capture(output)"},
	{"filename", "\tfilename of dsplog binary to parse(input)"},
       {"", "\t\tuse TCP to transfer dsp log (work with ps.exe)"},
       {"32-bit", "\tdebug bitmask, decimal or hex (start with 0x), set bit to 1 to enable, ONLY for S2/iOne"},
       {"path", "\tuse another ucode path to parse the dsplog,  like /tmp/mmcblk0p1/latest"},
       {"",  "\t\tcapture current dsp log in LOGMEM buffer, which is 128KB"},
	{"filename", "\tfilename of dsplog text file after parse"},
	{"filesize", "\tspecify the dsplog file size limit in pinpong mode"},
	{"", "\t\tappend ucode version to ucode binary filename, show ucode version when used alone"},
	{"", "\t\tshow dsplog_cap tool version"},
	{"filename", "\tuse dsp log mem buffer to verify captured log file (DSP should have halted)"},
	{"",  "\tif specified, current bitmask is added to, but not set.  (else it's set by default)"},
};

static void show_quit(void)
{
        fprintf(stderr, "\nPlease press 'Ctrl+C' to quit capture, or use 'killall dsplog_cap' to stop capture.  Don't use 'kill -9' to force quit.\n");
}

static void usage(void)
{
	int i;
	printf("\n\n############################################################\n");
	printf("dsplog_cap ver %s,  it supports A5s and S2.  usage:\n\n", DSPLOG_VERSION_STRING);
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
	printf("\n");
       extra_usage();
}


static int get_ucode_version(char * buffer, int max_len)
{
        int ucode_fd = -1;
        int ret = 0;
        ucode_version_t ucode_version;

        do {
            if ((ucode_fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
                        perror("/dev/ucode");
                        ret = -1;
                        break;
            }
            if (ioctl(ucode_fd, IAV_IOC_GET_UCODE_VERSION, &ucode_version) < 0) {
                perror("IAV_IOC_GET_UCODE_VERSION");
                ret = -1;
                break;
            }
            snprintf(buffer, max_len-1, "%d-%d-%d.num%d-ver%d", ucode_version.year, ucode_version.month,
                    ucode_version.day, ucode_version.edition_num, ucode_version.edition_ver);
            buffer[max_len-1] = '\0';
       } while(0);

        if (ucode_fd >=0)
            close(ucode_fd);
        return ret;

}


static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;
       int module_id;
       int bitmask;
       int thread_mask;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'm':
                      if ((module_id=get_dsp_module_id(optarg)) < 0) {
                            fprintf(stderr,"invalid dsp module id \n");
                            return -1;
                      } else {
                            if (dsp_debug_module_id_num >= 8) {
                                   fprintf(stderr, "too many modules to debug \n");
                                   return -1;
                             }
                            dsp_debug_module_id[dsp_debug_module_id_num] = module_id;
                            dsp_debug_module_id_last = dsp_debug_module_id_num;
                            dsp_debug_module_id_num++;
                     }
			break;
		case 'l':
			value = atoi(optarg);
                     if ((value >=0) &&(value <= 3)) {
                            dsp_debug_level = value;
                            dsp_debug_level_flag = 1;
                     } else {
                            fprintf(stderr, "invalid debug level \n");
                            return -1;
                    }
			break;
		case 'r':
                        if ((thread_mask = get_thread_bitmask(optarg)) <  0) {
                            fprintf(stderr, "invalid thread bit mask \n");
                            return -1;
                        } else {
                            dsp_debug_thread_bitmask = thread_mask;
                            dsp_debug_thread_bitmask_flag = 1;
                        }
        		break;
        	case 'o':
                    if (strlen(optarg) > 0) {
                        strncpy(dsp_log_output_filename, optarg, sizeof(dsp_log_output_filename));
                        dsp_log_output_filename[sizeof(dsp_log_output_filename)-1] = '\0';
                        dsp_log_output_filename_flag =1;
                      } else {
                            fprintf(stderr,"invalid output DSP log file name \n");
                            return -1;
                     }
			break;
        	case 'i':
                    if (strlen(optarg) > 0) {
                        strncpy(dsp_log_input_filename, optarg, sizeof(dsp_log_input_filename));
                        dsp_log_input_filename[sizeof(dsp_log_input_filename)-1] = '\0';
                        dsp_log_input_filename_flag =1;
                      } else {
                            fprintf(stderr, "invalid input DSP log file name \n");
                            return -1;
                     }
			break;
        	case 't':
                    dsp_log_transfer_tcp_flag = 1;
			break;
            case  'b':
                //bitmask
                    bitmask = get_dsp_bitmask(optarg);
                   dsp_debug_bit_mask[dsp_debug_module_id_last] = bitmask;
                break;

            case 'd':
                strncpy(dsp_ucode_dir, optarg, sizeof(dsp_ucode_dir));
                dsp_ucode_dir[sizeof(dsp_ucode_dir)-1] = '\0';
                break;

            case 'c':
                    dsp_log_capture_current_flag = 1;
                break;

            case 'f':
                strncpy(dsp_log_parse_output_filename, optarg, sizeof(dsp_log_parse_output_filename));
                dsp_log_parse_output_filename[sizeof(dsp_log_parse_output_filename)-1] = '\0';
                break;

            case 'p':
                dsp_log_pinpong_flag = 1;
                dsp_log_pinpong_filesize_limit = atoi(optarg);
                if (dsp_log_pinpong_filesize_limit < 2*1024*1024) {
                    dsp_log_pinpong_filesize_limit = DEFAULT_DSP_LOG_PINPONG_FILE_SIZE;
                      fprintf(stderr, "Use pinpong buffer with default size %d\n", DEFAULT_DSP_LOG_PINPONG_FILE_SIZE);
               }

                break;

            case 'v':
                dsp_log_show_ucode_version_flag = 1;
                break;

            case 'w':
                dsp_log_show_version_flag = 1;
                break;

            case 'y':
                strcpy(dsp_log_verify_filename, optarg);
                dsp_log_verify_flag = 1;
                break;

            case 'a':
                dsp_log_operation_add[dsp_debug_module_id_last] = 1;
                break;

              default:
                break;
		    }
        }

      //check the options
    if(dsp_log_capture_current_flag|| dsp_log_show_ucode_version_flag ||dsp_log_show_version_flag || dsp_log_verify_flag )
        return 0;

    if ((dsp_log_input_filename_flag)  && (dsp_log_output_filename_flag)) {
        fprintf(stderr,"Cannot enable dsplog capture(output) and dsplog(input) at same time! \n");
        return -1;
    }else if ((!dsp_log_input_filename_flag)  && (!dsp_log_output_filename_flag)) {
        fprintf(stderr,"Please specify cmd to either capture or parse!\n");
        return -1;
   }


    if (dsp_log_input_filename_flag) {
        dsp_log_work_mode = 1;  //parse
    } else {
        dsp_log_work_mode = 0; //capture
    }

    //check filename, do not have dir path in filename when transfer is tcp mode
    if (dsp_log_transfer_tcp_flag) {
        if (strchr(dsp_log_output_filename, '/')) {
            fprintf(stderr, "When tcp transfer mode specified, please do not use '/' in outputfilename, current name is %s \n", dsp_log_output_filename);
            return -1;
        }

        if (dsp_log_output_filename_flag) {
            fprintf(stderr, "Do not support parse dsp log by -t option now. \n");
            return -1;
        }
    }

    return 0;
}





/**********************************************************************

    CAPTURE DSP LOG START


  *********************************************************************/


int dsplog_enable(void)
{
	int fd;
	int debug_flag;
       int ret = 0;

        do {
            if ((fd = open("/dev/ambad", O_RDWR, 0)) < 0) {
                perror("/dev/ambad");
                ret = -1;
                break;
            }
            if (ioctl(fd, AMBA_DEBUG_IOC_GET_DEBUG_FLAG, &debug_flag) < 0) {
                perror("AMBA_DEBUG_IOC_GET_DEBUG_FLAG");
                ret = -1;
                break;
            }
            debug_flag |= AMBA_DEBUG_DSP;
            if (ioctl(fd, AMBA_DEBUG_IOC_SET_DEBUG_FLAG, &debug_flag) < 0) {
                perror("AMBA_DEBUG_IOC_SET_DEBUG_FLAG");
                ret = -1;
                break;
            }
        }while(0);
        if (fd >= 0)
        close(fd);
	 return ret;
}



int dsplog_get_transfer_fd(int method, char * output_filename,  int dsplog_port)
{
        int transfer_fd;
        char filename[512];

        if (!dsp_log_pinpong_flag) {
                if (dsp_log_transfer_fd >=0) {
                    return dsp_log_transfer_fd;
               }
                if ((transfer_fd = amba_transfer_open(output_filename, method,  dsplog_port)) < 0) {
                    fprintf(stderr, "create file for transfer failed %s \n", output_filename);
                    return -1;
                }
                dsp_log_transfer_fd = transfer_fd;
                dsp_log_transfer_method = method;
                return transfer_fd;
        } else {
            if (dsp_log_transfer_fd >=0) {
                //now it's pinpong mode.
               if (dsp_log_pinpong_filesize_now  >= dsp_log_pinpong_filesize_limit) {
                    //oversize file, close current fd and open new
                    amba_transfer_close(dsp_log_transfer_fd, method);
                    //switch pinpong state
                    dsp_log_pinpong_state =  !dsp_log_pinpong_state;
               } else {

                    return dsp_log_transfer_fd;

                }
           }
            //now create new file
             if (!dsp_log_pinpong_state)
                    sprintf(filename, "%s.1", output_filename);
             else
                    sprintf(filename, "%s.2", output_filename);

            if ((transfer_fd = amba_transfer_open(filename, method,  dsplog_port)) < 0) {
                fprintf(stderr, "create file for transfer failed %s \n", filename);
                return -1;
            } else {
                dsp_log_transfer_fd = transfer_fd;
                dsp_log_transfer_method = method;
            }

            //reset to new file with size
             dsp_log_pinpong_filesize_now = 0;
            return transfer_fd;
        }
}


// dsplog log-filename [ 1000 (max log size) ]
// dsplog d module debuglevel coding_thread_printf_disable_mask

// for iOne / S2:
// dsplog level [module] [0:add or 1:set] [debug mask]
// dsplog thread [thread disable bit mask]

int dsplog_capture(int dsplog_drv_fd, char * log_buffer, int method, char * output_filename,  int dsplog_port)
{
        int transfer_fd;
        //init datax
        amba_transfer_init(method);

        //start capture
        if (ioctl(dsplog_drv_fd, AMBA_IOC_DSPLOG_START_CAPUTRE) < 0) {
            perror("AMBA_IOC_DSPLOG_START_CAPUTRE");
            return -1;
        }

	while (1) {
            transfer_fd = dsplog_get_transfer_fd(method,  output_filename, dsplog_port);
            if (!transfer_fd) {
                       fprintf(stderr, "dsplog_get_transfer_fd failed for %s \n ", output_filename);
                       return -1;
             }

            ssize_t size = read(dsplog_drv_fd, log_buffer, MAX_LOG_READ_SIZE);
		if (size < 0) {
                  if (size ==  -EAGAIN) {
                      //do it again, the previous read fails because of EAGIN (interupted by user)
                        fprintf(stderr, "read it again \n");
                        size = read(dsplog_drv_fd, log_buffer, MAX_LOG_READ_SIZE);
                        if (size < 0)  {
                                    fprintf(stderr, "read fail again \n");
                                    return -1;
                         }
                  }else {
                    perror("dsplog_cap: read\n");
                    return -1;
                 }
	      } else if(size == 0) {
                 // fprintf(stderr, "dsplog read finished. stop amba_transfer.\n");
                    amba_transfer_close(dsp_log_transfer_fd,  dsp_log_transfer_method);
                    amba_transfer_deinit(dsp_log_transfer_method);
                    break;
            }



            if (size > 0) {
                  if (amba_transfer_write(transfer_fd, log_buffer, size, method) < 0) {
                        fprintf(stderr,"dsplog_cap: write failed \n");
                        return -1;
                    } else {
                         //accumulate file size
                          if (dsp_log_pinpong_flag)
                          dsp_log_pinpong_filesize_now += size;
    	            }
		}
	    }

    return 0;
}

int dsplog_capture_stop(int dsplog_drv_fd)
{
        //start capture
        if (ioctl(dsplog_drv_fd,  AMBA_IOC_DSPLOG_STOP_CAPTURE ) < 0) {
            perror("AMBA_IOC_DSPLOG_STOP_CAPUTRE");
            return -1;
        }  else {
            return 0;
       }

}

/**********************************************************************

    CAPTURE DSP LOG END


  *********************************************************************/





/**********************************************************************

    PARSE DSP LOG START


  *********************************************************************/

u8 *mmap_ucode_bin(char *dir, char *name)
{
	u8 *mem = NULL;
	FILE *fp;
	int fsize;
	char filename[512];
      do {
            if ((!dir) || (!name)) {
                mem = NULL;
                break;
            }
            sprintf(filename, "%s/%s", dir, name);
            if ((fp = fopen(filename, "r")) == NULL) {
                fprintf(stderr,"cannot open ucode bin file %s \n", filename);
                perror(filename);
                mem = NULL;
                break;
            }

            if (fseek(fp, 0, SEEK_END) < 0) {
                perror("fseek");
                mem = NULL;
                break;
            }
            fsize = ftell(fp);

            mem = (u8 *)mmap(NULL, fsize, PROT_READ, MAP_SHARED, fileno(fp), 0);
            if ((int)mem == -1) {
                perror("mmap");
                mem = NULL;
                break;
            }
        }while(0);

	return mem;
}


int print_log(idsp_printf_t *record, u8 * pcode, u8 * pmdxf, u8 * pmemd,  FILE *  write_file)
{
	char *fmt;
	u8 *ptr;
	u32 offset;
	if (record->format_addr == 0)
		return -1;
	switch (record->dsp_core) {
		case 0: ptr = pcode; offset = UCODE_CORE_OFFSET; break;
		case 1: ptr = pmdxf; offset = UCODE_MDXF_OFFSET; break;
		case 2: ptr = pmemd; offset = UCODE_MEMD_OFFSET; break;
		default:
			fprintf(stderr, "dsp_core = %d\n", record->dsp_core);
			return -1;
	}
	fmt = (char*)(ptr + (record->format_addr - offset));
	fprintf(write_file, "[core:%d:%d] ", record->thread_id, record->seq_num);
	fprintf(write_file, fmt, record->arg1, record->arg2, record->arg3, record->arg4, record->arg5);
	return 0;
}


int dsplog_parse(char * log_filename, u8 * pcode, u8 * pmdxf, u8 * pmemd,  char * output_text_file)
{
    FILE * input = NULL;
    FILE * parse_out = NULL;
    idsp_printf_t record;
    int first = 0;
    int last = 0;
    int total = 0;
    int loss;

     do {
        if ((input = fopen(log_filename, "rb")) == NULL) {
            fprintf(stderr, "cannot open dsplog file %s to parse, please use '-i' option to specify \n", log_filename);
            break;
        }

        if ((parse_out = fopen(output_text_file, "wb")) == NULL) {
            fprintf(stderr, "cannot open dsplog parse text file %s to write, please use '-f' option to specify\n", output_text_file);
            break;
        }
     } while(0);

    if (!input || !parse_out) {
         if (input)
            fclose(input);
         if (parse_out)
            fclose(parse_out);
        return -1;
   }

     fprintf(stderr, "You may press Ctrl+C to quit parsing. \n");
    for (;;) {
        int rval;
        if ((rval = fread(&record, sizeof(record), 1, input)) != 1) {
            break;
        }
        if (first == 0) {
            first = record.seq_num;
            last = first - 1;
        }
        print_log(&record, pcode, pmdxf, pmemd, parse_out);

        total++;
        ++last;
        last = record.seq_num;
        if ((total % 1000) == 0) {
            fprintf(stderr, "\r%d", total);
            fflush(stderr);
        }
    }

    fprintf(stderr, "\r");
    fprintf(stderr, "first record: %d\n", first);
    fprintf(stderr, "total record: %d\n", total);
    fprintf(stderr, "last record: %d\n", last);

    loss = (last - first + 1) - total;
    if ((loss < 0) && (loss + total == 0)) {
        fprintf(stderr, "this is a LOGMEM ring buf direct dump\n");
    } else {
        fprintf(stderr, "lost records: %d\n", loss);
    }

    fclose(input);
    return 0;

}





/**********************************************************************

    PARSE DSP LOG ENDS


  *********************************************************************/

typedef struct dsplog_cap_s {
    int logdrv_fd;
    char * log_buffer;
    int datax_method;
    char * output_filename;
    int  log_port;
}dsplog_cap_t;


int block_signals()
{
        sigset_t   signal_mask;  // signals to block
        sigemptyset (&signal_mask);
        sigaddset (&signal_mask, SIGINT);
        sigaddset (&signal_mask, SIGQUIT);
        sigaddset (&signal_mask, SIGTERM);
        pthread_sigmask (SIG_BLOCK, &signal_mask, NULL);
        return 0;
}


static void * dsp_log_capture_func(void * data)
{

        dsplog_cap_t * p_cap = (dsplog_cap_t *) data;

//        fprintf(stderr,"dsplog_capture_thread, tid = %ld\n", syscall(SYS_gettid));

        if (!p_cap)
            return NULL;
        if (dsplog_capture(p_cap->logdrv_fd, p_cap->log_buffer, p_cap->datax_method, p_cap->output_filename, p_cap->log_port) < 0) {
           fprintf(stderr, "dsplog_cap: capture failed. \n");
           exit(1);
         }
        fprintf(stderr, "dsplog_cap: capture thread finished.\n");
        return NULL;
}



static void * dsp_log_capture_sig_thread_func(void * context)
{
	int       sig_caught;    /* signal caught       */
        sigset_t   signal_mask;  // signals to block
//        fprintf(stderr,"dsplog_capture_sig_thread, tid = %ld\n", syscall(SYS_gettid));
        sigemptyset (&signal_mask);
        sigaddset (&signal_mask, SIGINT);
        sigaddset (&signal_mask, SIGQUIT);
        sigaddset (&signal_mask, SIGTERM);
        sigwait (&signal_mask, &sig_caught);
	switch (sig_caught)	{
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
		// send exit event to MainLoop
		if (dsp_log_work_mode == 0)  {
                  //  fprintf(stderr, "signal captured in sig thread!tell main process to exit\n");
                    sem_post(&exit_main_sem);
	       }
             else  {
                    fprintf(stderr, "dsplog_cap: parse mode force quit. \n");
                    exit(1);
            }
	      break;
	default:         /* should normally not happen */
		fprintf (stderr, "\nUnexpected signal caught in sig thread%d\n", sig_caught);
		break;
	}
	return NULL;
}


//verify dsplog binary file to see whether the log file is correct, missing, and consistent with the current dsplog
//can only be used when platform is not boot yet, so that amba_debug can dump the dsplog buffer to verify.
static int verify_dsplog_file(char * dsplog_binary_filename)
{
    FILE * fp;
    int length;
    int ret = 0;
    u32 last_record_in_logfile;
    idsp_printf_t   record;
    char cmdstr[256];
    u32 log_entry_max;
    int log_lines_diff;
    do {
        fp = fopen(dsplog_binary_filename, "r");
        if (!fp) {
            fprintf(stderr, "dsplog verify cannot open %s \n", dsplog_binary_filename);
            ret = -1;
            break;
        }

        fseek(fp, 0, SEEK_END);
        length = ftell(fp);
        if (length % sizeof(idsp_printf_t)) {
            fprintf(stderr, "dsplog verify found file %s has size not multiple of 32, should have data missing\n", dsplog_binary_filename);
            ret = -1;
            break;
        }
        fseek(fp, 0-sizeof(idsp_printf_t), SEEK_END);
        if (fread(&record, 1, sizeof(idsp_printf_t), fp)!= sizeof(idsp_printf_t)) {
            fprintf(stderr, "dsplog verify read file error \n");
            ret = -1;
            break;
        }
        fclose(fp);
        fp = NULL;
        last_record_in_logfile = record.seq_num;

        fprintf(stderr, "last record in log file = %d \n", last_record_in_logfile);
        //now check dsp log mem buffer to find which is latest one

        //dump the dsplog mem to file

        sprintf(cmdstr, "amba_debug -r 0x%x -s 0x%x -f /tmp/.dsplogmem.bin", DSPLOG_MEM_PHY_ADDR, DSPLOG_MEM_SIZE);
       if (system(cmdstr) < 0) {
            ret = -1;
            perror("amba_debug");
            break;
        }

        fp = fopen("/tmp/.dsplogmem.bin", "r");
        if (!fp) {
            fprintf(stderr, "dsplog verify cannot open log mem file\n");
            ret = -1;
            break;
        }

        log_entry_max = 0 ;
        while(fread(&record, 1, sizeof(idsp_printf_t), fp)){
            if (record.seq_num > log_entry_max)
                 log_entry_max = record.seq_num;
            else
                break;
        }
        fclose(fp);
        fp = NULL;
        unlink("/tmp/.dsplogmem.bin");

        fprintf(stderr, "log_entry_max = %d \n", log_entry_max);

        log_lines_diff =  log_entry_max -  last_record_in_logfile;
        if (last_record_in_logfile == log_entry_max) {
            fprintf(stderr, "dsplog verify PERFECT! \n");
        } else if (log_lines_diff < 10) {
            fprintf(stderr, "dsplog verify result GOOD, only a few lines (%d) of logs missing. \n", log_lines_diff );
        } else {
            fprintf(stderr, "dsplog verify result BAD, some lines (%d) of logs are missing. \n",  log_lines_diff);
        }


    }while(0);

    if (fp)
        fclose(fp);

    return ret;
}


int main(int argc, char **argv)
{
        int iav_fd = -1;
        int dsp_log_drv_fd = -1;
        char * dsp_log_buffer=NULL;
        int datax_method;
        u8 * pcode;
        u8 * pmemd;
        u8 * pmdxf;
        pthread_t  sig_thread;
        pthread_t cap_thread;
        dsplog_cap_t  log_cap;
        int ret = 0;
        int sem_created = 0;
//        fprintf(stderr, "dsplog main tid = %d \n", getpid());


#ifdef ENABLE_RT_SCHED
        struct sched_param param;
        param.sched_priority = 50;
        if (sched_setscheduler(0, SCHED_RR, &param) < 0)
            perror("sched_setscheduler");
#endif

        do {

                if (init_param(argc, argv) < 0) {
                    usage();
                    ret = -1;
                    break;
                }

                if (dsp_log_show_ucode_version_flag) {
                    char tmp_str[128];
                    get_ucode_version(tmp_str, 128);
                    fprintf(stderr, "Current ucode version is %s \n", tmp_str);
                    ret = 0;
                    break;
                }

                if (dsp_log_verify_flag) {
                    ret = verify_dsplog_file(dsp_log_verify_filename);
                    break;
                }

                if (dsp_log_show_version_flag) {
                       fprintf(stderr,"dsplog tool version is %s \n",  DSPLOG_VERSION_STRING);
                       ret = 0;
                       break;
                }

                if (dsp_log_capture_current_flag) {
                     char cmdstr[256];
                     sprintf(cmdstr, "amba_debug -r 0x%x -s 0x%x -f /tmp/dspmem_dump.bin", DSPLOG_MEM_PHY_ADDR, DSPLOG_MEM_SIZE);
                    if (system(cmdstr) < 0) {
                        perror("amba_debug");
                        ret = -1;
                       break;
                     } else {
                        fprintf(stderr, "log got and save to  /tmp/dspmem_dump.bin, please check \n");
                        ret = 0;
                        break;
                    }
                }

                    /*********************************************************/
                if (dsp_log_work_mode == 0)  {
                    //capture log mode
                    //block signals
                    block_signals();

                    //init sem
                   if(sem_init(&exit_main_sem, 0, 0) < 0) {
                        fprintf(stderr, "dsplog_cap: create sem for exit failed \n");
                        ret =  -1;
                        break;
                    } else {
                        sem_created = 1;
                     }

                    //create sig thread
                    if (pthread_create(&sig_thread, NULL, dsp_log_capture_sig_thread_func, &log_cap)!=0) {
                        fprintf(stderr, "dsplog_cap: create sig thread failed \n");
                        ret = -1;
                        break;
                   }

                    if ((iav_fd =  open("/dev/iav", O_RDWR, 0)) < 0) {
                        perror("/dev/iav");
                       ret = -1;
                       break;
                    }

                    if (dsplog_setup(iav_fd) < 0 ) {
                        fprintf(stderr, "dsplog_cap: dsplog_setup failed \n");
                        ret = -1;
                        break;
                    } else {
                        close(iav_fd);
                     }


                    //open dsplog driver
                    if ((dsp_log_drv_fd = open("/dev/dsplog", O_RDONLY, 0)) < 0) {
                        perror("cannot open /dev/dsplog");
                        ret = -1;
                        break;
                    }

                    //allocate mem for DSP log capture
                     dsp_log_buffer = malloc(MAX_LOG_READ_SIZE);
                    if (!dsp_log_buffer) {
                        fprintf(stderr, "dsplog_cap: insufficient memory for cap buffer \n");
                        ret = -1;
                        break;
                     }

                    //enable dsplog  (TO remove to reduce complexity with Amba_debug ? )
                    if (dsplog_enable() < 0) {
                        ret = -1;
                        break;
                     }

                    //start capture log
                    if (!dsp_log_transfer_tcp_flag) {
                        datax_method = TRANS_METHOD_NFS;
                    } else {
                        datax_method = TRANS_METHOD_TCP;
                    }


                    //prepare dsplog thread
                    memset(&log_cap, 0, sizeof(log_cap));
                    log_cap.datax_method = datax_method;
                    log_cap.logdrv_fd = dsp_log_drv_fd;
                    log_cap.log_buffer =  dsp_log_buffer;
                    log_cap.log_port = DSP_LOG_PORT;
                    log_cap.output_filename = dsp_log_output_filename;
                    if (pthread_create(&cap_thread, NULL, dsp_log_capture_func, &log_cap)!=0) {
                        fprintf(stderr, "dsplog_cap: create cap thread failed \n");
                        ret = -1;
                        break;
                    }

                    show_quit();

                    //wait for sig thread to give sem to stop capture & exit
                    sem_wait(&exit_main_sem);

                    //stop capture
                    fprintf(stderr,"try to stop dsp log capture.\n");
                    if (dsplog_capture_stop(log_cap.logdrv_fd) < 0) {
                        fprintf(stderr, "dsplog cap stopped failed \n");
                        ret = -1;
                        break;
                    }
                    fprintf(stderr,"dsp log capture stopped OK.\n");
                    if (pthread_join(sig_thread, NULL)!=0) {
                        perror("pthread_join sig thread! \n");
                    }
                    if (pthread_join(cap_thread, NULL)!=0) {
                        perror("pthread_join cap thread! \n");
                    }
                    fprintf(stderr, "dsplog_cap: capture done and main thread exit!\n");


                }else {
                    /*********************************************************/
                    //DSP log parse mode
                    //mmap ucode binary to memory for parsing
                    if (!(pcode = mmap_ucode_bin(dsp_ucode_dir, UCODE_CORE_FILE))) {
                        fprintf(stderr,"mmap ucode bin %s failed \n", UCODE_CORE_FILE);
                        ret = -1;
                        break;
                    }
                    if (!(pmemd =  mmap_ucode_bin(dsp_ucode_dir, UCODE_MEMD_FILE))) {
                         fprintf(stderr,"mmap ucode bin %s failed \n", UCODE_MEMD_FILE);
                        ret = -1;
                        break;
                    }

                    if  (!(pmdxf =  mmap_ucode_bin(dsp_ucode_dir, UCODE_MDXF_FILE))){
                        fprintf(stderr,"mmap ucode bin %s failed \n", UCODE_MDXF_FILE);
                        ret = -1;
                        break;
                    }

                    if (dsplog_parse(dsp_log_input_filename, pcode, pmemd, pmdxf, dsp_log_parse_output_filename) <  0) {
                        fprintf(stderr,"dsplog_cap: parse dsp log failed \n");
                        ret = -1;
                        break;
                    }
                }

        }while(0);


        if (sem_created)
            sem_destroy(&exit_main_sem);

        if (dsp_log_buffer)
            free (dsp_log_buffer);
        return ret;
}

