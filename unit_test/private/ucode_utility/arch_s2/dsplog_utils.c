/*
 * dsplog_utils.c   (for S2 )
 *
 * History:
 *	2013/10/10 - [Louis Sun] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#define DSPLOG_MEM_PHY_ADDR  0x80000
#define DSPLOG_MEM_SIZE            0x20000

//three ucode binaries for this ARCH
#define UCODE_CORE_FILE "orccode.bin"
#define UCODE_MEMD_FILE "orcme.bin"
#define UCODE_MDXF_FILE  "orcmdxf.bin"


#define UCODE_CORE_OFFSET   0x800000
#define UCODE_MDXF_OFFSET   0x600000
#define UCODE_MEMD_OFFSET   0x300000


static int get_dsp_module_id(char * input_name)
{
    int module_id;
    //module_id value should conform to dsp api spec

    if (!strcasecmp(input_name, "common")) {
        module_id = 0;
    } else if  (!strcasecmp(input_name, "vcap")) {
        module_id = 1;
    } else if  (!strcasecmp(input_name, "vout")) {
        module_id = 2;
    } else if  (!strcasecmp(input_name, "encoder")) {
        module_id = 3;
    } else if  (!strcasecmp(input_name, "decoder")) {
        module_id = 4;
    } else if  (!strcasecmp(input_name, "idsp")) {
        module_id = 5;
    } else if  (!strcasecmp(input_name, "memd")) {
        module_id = 6;
    } else if  (!strcasecmp(input_name, "perf")) {
        module_id = 7;
    } else if  (!strcasecmp(input_name, "all")) {
        module_id = 255;
    } else {
        printf("DSP module %s not supported to debug for this ARCH \n", input_name);
        return -1;
    }

    return module_id;
}



static int dsp_setup_debug(int iav_fd, iav_dsp_setup_t *dsp_setup)
{
	if (ioctl(iav_fd, IAV_IOC_LOG_SETUP, dsp_setup) < 0) {
		perror("IAV_IOC_LOG_SETUP");
		return -1;
	}
	return 0;
}

static int setup_level(int iav_fd, int module, int op,  int debugmask)
{
       char operation[64];
	iav_dsp_setup_t dsp_setup;
        if (module < 0)  {
            fprintf(stderr, "setup_level failed \n");
            return -1;
       }
	dsp_setup.cmd = 1;
	dsp_setup.args[0] = module;
	dsp_setup.args[1] = op;     //0 is set,  1 is ADD
	dsp_setup.args[2] = debugmask;

       if (op) {
          strcpy(operation, "added (bit OR) with");
       }
       else if (!debugmask) {
          strcpy(operation, "cleared to");
       }else {
          strcpy(operation, "set to");
       }
       fprintf(stderr, "setup_level for module %d, debug mask is %s 0x%x\n", module, operation, debugmask);

	if (dsp_setup_debug(iav_fd, &dsp_setup) < 0)
		return -1;

	return 0;
}


int setup_thread(int iav_fd, int thread_bitmask)
{
	iav_dsp_setup_t dsp_setup;
	dsp_setup.cmd = 2;
	dsp_setup.args[0] = thread_bitmask;

	if (dsp_setup_debug(iav_fd, &dsp_setup) < 0)
		return -1;

	return 0;
}

static int get_dsp_bitmask(char * bitmask_str)
{
    int bitmask;

    if (strlen(bitmask_str) <3) {
        //should only be a decimal value
        bitmask = atoi(bitmask_str);
    } else if  (!strcmp(bitmask_str, "all")) {
        bitmask = 0xFFFFFFFF;
    }else {
        //check whether this is decimal or heximal
        if ((bitmask_str[0]=='0') && ((bitmask_str[1]=='x')||(bitmask_str[1]=='X'))) {
            //it's a hex
            bitmask_str+=2;
            if (EOF == sscanf(bitmask_str, "%x", &bitmask)) {
                    printf("get_dsp_bitmask: error to get hex value from string %s\n", bitmask_str);
                    return 0;
            }
        } else {
            bitmask = atoi(bitmask_str);
        }
     }

// printf("dsp_bitmask is 0x%x \n", bitmask);

    return bitmask;
}


static int get_thread_bitmask(char * bitmask_str)
{
    int bitmask;
    if (strlen(bitmask_str) <3) {
        //should only be a decimal value
        bitmask = atoi(bitmask_str);
    } else {
        //check whether this is decimal or heximal
        if ((bitmask_str[0]=='0') && ((bitmask_str[1]=='x')||(bitmask_str[1]=='X'))) {
            //it's a hex
            bitmask_str+=2;
            if (EOF == sscanf(bitmask_str, "%x", &bitmask)) {
                    printf("get_thread_bitmask: error to get hex value from string %s\n", bitmask_str);
                    return -1;
            }
        } else {
            bitmask = atoi(bitmask_str);
        }
     }

    return bitmask;
}


static int dsplog_setup(int iav_fd)
{
        int i;
        //debug module
        if (dsp_debug_module_id_num <= 0) {
                    return -1;
       }

       for (i=0; i < dsp_debug_module_id_num; i++) {
             if  (setup_level(iav_fd, dsp_debug_module_id[i],  dsp_log_operation_add[i],  dsp_debug_bit_mask[i]) < 0) {
                    return -1;
              }
       }

       if (dsp_debug_thread_bitmask_flag) {
            if (setup_thread(iav_fd, dsp_debug_thread_bitmask) < 0) {
                    return -1;
             }
       }

        return 0;
}


static void extra_usage()
{

	printf("  Enable all debug information :\n");
	printf("  #dsplog_cap -m all -b all -o /tmp/dsplog1.bin \n\n");

	printf("  Enable debug level for IDSP / capture issues:\n");
	printf("  #dsplog_cap -m idsp --add -b 36  -o /tmp/log_idsp.bin\n");
       printf("  Or:   \n");
       printf("  #dsplog_cap -m idsp --add -b 548  -o /tmp/log_idsp.bin\n\n");


	printf("  Enable debug level for VOUT issue:\n");
	printf("  #dsplog_cap -m vout --add -b 1 -o /tmp/log_vout.bin\n\n");

	printf("  Enable debug level for encode issue:\n");
	printf("  #dsplog_cap  -m encoder --add -b 10  -o /tmp/log_encoder.bin\n\n");

	printf("  Enable debug level for CABAC / CAVLC issue:\n");
	printf("  #dsplog_cap  -m encoder --add -b 10240 -o /tmp/log_encoder2.bin\n\n");

       printf("  Use Pinpong files to store DSP log for vcap module with 10MB limit for each pinpong file\n");
	printf("  #dsplog_cap -m vcap  -b all -o /tmp/log1.bin -p 10000000 \n\n");

       printf("  Capture both vcap and vout modules, by using -m multiple times \n");
	printf("  #dsplog_cap -m vcap -m vout -b all -o /mnt/mycap.bin \n\n");

	printf("  Parse a DSP log\n");
	printf("  #dsplog_cap -i /mnt/mycap.bin -f /mnt/myparse.txt  [ -d /ucode_dir ]    \n\n");


}

