/*
 * sdupdate_daemon.c
 * the program can setup VIN , preview and start encoding/stop
 * encoding for flexible multi streaming encoding.
 * after setup ready or start encoding/stop encoding, this program
 * will exit
 *
 * History:
 *	2012/11/22 - [Zhong Xu] create this file
 *	2012/11/23 - [Bo YU] modify some type and clean
 *
 * Copyright (C) 2007-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "nand_update.h"

#define MSG_TYPE		2
#define MAGIC_NUMBER	"AMBUPGD"
#define QUEUE_NAME		"/updatemsg_queue"
#define DESC_NAME		"desc.p"
#define KERN_NAME		"Image"
#define FS_NAME			"ubifs"
#define BLD_NAME		"amboot"
#define HAL_NAME		"hal"
#define PBA_NAME		"zImage"
#define BST_NAME		"bst"

#define dbg_printf printf
#define MAX_SIZE  128

int kernel_flag, fs_flag, bld_flag, hal_flag, pba_flag, bst_flag;

const unsigned int crc32_tab[] = { 0x00000000, 0x77073096, 0xee0e612c,
                                   0x990951ba, 0x076dc419, 0x706af48f,
                                   0xe963a535, 0x9e6495a3, 0x0edb8832,
                                   0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
                                   0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
                                   0x90bf1d91, 0x1db71064, 0x6ab020f2,
                                   0xf3b97148, 0x84be41de, 0x1adad47d,
                                   0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
                                   0x136c9856, 0x646ba8c0, 0xfd62f97a,
                                   0x8a65c9ec, 0x14015c4f, 0x63066cd9,
                                   0xfa0f3d63, 0x8d080df5, 0x3b6e20c8,
                                   0x4c69105e, 0xd56041e4, 0xa2677172,
                                   0x3c03e4d1, 0x4b04d447, 0xd20d85fd,
                                   0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
                                   0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
                                   0x45df5c75, 0xdcd60dcf, 0xabd13d59,
                                   0x26d930ac, 0x51de003a, 0xc8d75180,
                                   0xbfd06116, 0x21b4f4b5, 0x56b3c423,
                                   0xcfba9599, 0xb8bda50f, 0x2802b89e,
                                   0x5f058808, 0xc60cd9b2, 0xb10be924,
                                   0x2f6f7c87, 0x58684c11, 0xc1611dab,
                                   0xb6662d3d, 0x76dc4190, 0x01db7106,
                                   0x98d220bc, 0xefd5102a, 0x71b18589,
                                   0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
                                   0x7807c9a2, 0x0f00f934, 0x9609a88e,
                                   0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
                                   0x91646c97, 0xe6635c01, 0x6b6b51f4,
                                   0x1c6c6162, 0x856530d8, 0xf262004e,
                                   0x6c0695ed, 0x1b01a57b, 0x8208f4c1,
                                   0xf50fc457, 0x65b0d9c6, 0x12b7e950,
                                   0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf,
                                   0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
                                   0x4db26158, 0x3ab551ce, 0xa3bc0074,
                                   0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
                                   0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
                                   0x346ed9fc, 0xad678846, 0xda60b8d0,
                                   0x44042d73, 0x33031de5, 0xaa0a4c5f,
                                   0xdd0d7cc9, 0x5005713c, 0x270241aa,
                                   0xbe0b1010, 0xc90c2086, 0x5768b525,
                                   0x206f85b3, 0xb966d409, 0xce61e49f,
                                   0x5edef90e, 0x29d9c998, 0xb0d09822,
                                   0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
                                   0xb7bd5c3b, 0xc0ba6cad, 0xedb88320,
                                   0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
                                   0xead54739, 0x9dd277af, 0x04db2615,
                                   0x73dc1683, 0xe3630b12, 0x94643b84,
                                   0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b,
                                   0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
                                   0xf00f9344, 0x8708a3d2, 0x1e01f268,
                                   0x6906c2fe, 0xf762575d, 0x806567cb,
                                   0x196c3671, 0x6e6b06e7, 0xfed41b76,
                                   0x89d32be0, 0x10da7a5a, 0x67dd4acc,
                                   0xf9b9df6f, 0x8ebeeff9, 0x17b7be43,
                                   0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
                                   0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
                                   0xa6bc5767, 0x3fb506dd, 0x48b2364b,
                                   0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
                                   0x41047a60, 0xdf60efc3, 0xa867df55,
                                   0x316e8eef, 0x4669be79, 0xcb61b38c,
                                   0xbc66831a, 0x256fd2a0, 0x5268e236,
                                   0xcc0c7795, 0xbb0b4703, 0x220216b9,
                                   0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
                                   0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
                                   0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
                                   0x9b64c2b0, 0xec63f226, 0x756aa39c,
                                   0x026d930a, 0x9c0906a9, 0xeb0e363f,
                                   0x72076785, 0x05005713, 0x95bf4a82,
                                   0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
                                   0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7,
                                   0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
                                   0x68ddb3f8, 0x1fda836e, 0x81be16cd,
                                   0xf6b9265b, 0x6fb077e1, 0x18b74777,
                                   0x88085ae6, 0xff0f6a70, 0x66063bca,
                                   0x11010b5c, 0x8f659eff, 0xf862ae69,
                                   0x616bffd3, 0x166ccf45, 0xa00ae278,
                                   0xd70dd2ee, 0x4e048354, 0x3903b3c2,
                                   0xa7672661, 0xd06016f7, 0x4969474d,
                                   0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
                                   0x40df0b66, 0x37d83bf0, 0xa9bcae53,
                                   0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
                                   0xbdbdf21c, 0xcabac28a, 0x53b39330,
                                   0x24b4a3a6, 0xbad03605, 0xcdd70693,
                                   0x54de5729, 0x23d967bf, 0xb3667a2e,
                                   0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
                                   0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
                                   0x2d02ef8d };

static int check_desc(char *path)
{
  char *bfp, *image_name, *fp;
  char *cfp;
  int lstrlen, desc_fd, alen;
  struct stat file_stat;

  desc_fd = open(path, O_RDONLY);
  if (desc_fd < 0) {
    perror("No desc file\n");
    return -1;
  }

  fstat(desc_fd, &file_stat);
  fp = malloc(file_stat.st_size);
  if (!fp) {
    perror("No mem\n");
    close(desc_fd);
    return -1;
  }

  alen = file_stat.st_size;
  if (read(desc_fd, fp, alen) != alen) {
    perror("Read desc file error\n");
    free(fp);
    close(desc_fd);
    return -1;
  }

  cfp = fp;
  while (1) {
    bfp = strstr(fp, ";");
    if (!bfp)
      lstrlen = cfp - fp + alen;
    else
      lstrlen = bfp - fp + 1;
    image_name = malloc(lstrlen);
    if (!image_name) {
      perror("No mem\n");
      free(fp);
      close(desc_fd);
      return -1;
    }

    strncpy(image_name, fp, lstrlen - 1);
    *(image_name + lstrlen - 1) = '\0';

    if (strncmp(image_name, KERN_NAME, lstrlen) == 0)
      kernel_flag = NAND_UPGRADE_FILE_TYPE_PRI;
    if (strncmp(image_name, FS_NAME, lstrlen) == 0)
      fs_flag = NAND_UPGRADE_FILE_TYPE_LNX;

    if (strncmp(image_name, BLD_NAME, lstrlen) == 0)
      bld_flag = NAND_UPGRADE_FILE_TYPE_BLD;

    if (strncmp(image_name, BST_NAME, lstrlen) == 0)
      bst_flag = NAND_UPGRADE_FILE_TYPE_BST;

    if (strncmp(image_name, HAL_NAME, lstrlen) == 0)
      hal_flag = NAND_UPGRADE_FILE_TYPE_HAL;

    if (strncmp(image_name, PBA_NAME, lstrlen) == 0)
      pba_flag = NAND_UPGRADE_FILE_TYPE_PBA;

    if (bfp)
      fp = bfp + 1;
    else
      fp = fp + lstrlen;

    if (fp >= cfp + alen - 1)
      break;
  }

  free(cfp);
  free(image_name);
  close(desc_fd);

  return 0;
}

int calc_crc(char *fp, int payload)
{
  unsigned int crc = ~0U;
  char *p = fp;

  while (payload --) {
    crc = crc32_tab[(crc ^ *p ++) & 0xff] ^ (crc >> 8);
  }

  crc ^= ~0U;

  return crc;

}

int check_file(char *path, int flag)
{
  int fd;
  char *map;
  struct stat file_stat;
  nand_update_file_header_t *file_head;
  int crc, ret = 0;

  fd = open(path, O_RDWR);
  if (fd < 0) {
    perror("Open check file error\n");
    return -1;
  }

  fstat(fd, &file_stat);

  map = (char *) mmap(0, file_stat.st_size,
  PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
  if (map < 0) {
    perror("MMAP ERROR\n");
    close(fd);
    return -1;
  }

#ifndef TEST
  file_head = (nand_update_file_header_t *) map;
  if ((strcmp((char *) file_head->magic_number, MAGIC_NUMBER) != 0)
      || (flag != file_head->payload_type)) {
    munmap(map, file_stat.st_size);
    close(fd);
    return -1;
  }

  crc = calc_crc(map + file_head->header_size, file_head->payload_size);
#else
  crc=calc_crc(map, file_stat.st_size);
#endif

#ifndef TEST
  if (crc == file_head->payload_crc32) {
    ret = 0;
  } else {
    ret = -1;
  }
#endif

  munmap(map, file_stat.st_size);
  close(fd);
  return ret;

}

#ifndef TEST
void do_reboot_for_update()
{
  int ret;
  int retry = 0;

  if (kernel_flag || bld_flag || bst_flag || hal_flag || fs_flag) {
    while (retry ++ < 3) {
      ret = system("/usr/sbin/nandwrite -C "
                   "'console=ttyS0 rootfs=ramfs "
                   "root=/dev/ram rw initrd=/linuxrc'");
      if (ret < 0)
        printf("Failed to config cmdline, retry...");
      else {
        printf("Everythin is ready for update, machine will reboot...");
        system("/sbin/reboot");
        while (1) {
        }
      }
    }
    printf("Failed to config cmdline, give up this update!\n");
  }
}

int do_pba_update(char *path)
{
  char command[256] = "nandwrite -p -Q -F 4  -t -L 0xC0208000 /dev/";
  char *mtd = NULL;
  char erase[32] = "flash_erase /dev/";
  FILE *fp;
  char buf[1024];
  char *p, *cp;
  int len, ret = 0;

  fp = fopen("/proc/mtd", "r");
  if (!fp) {
    perror("Faile to open /proc/mtd\n");
    return -1;
  }

  while (fgets(buf, 1024, fp) != NULL) {
    p = strstr(buf, "pba");
    if (p) {
      cp = strstr(buf, ":");
      len = cp - buf + 1;
      mtd = malloc(len);
      if (!mtd) {
        perror("No mem\n");
        fclose(fp);
        return -1;
      }
      strncpy(mtd, buf, len);
      *(mtd + len - 1) = '\0';
      break;
    }
  }

  if (!mtd) {
    printf("Error!! No pba partation\n");
    fclose(fp);
    return -1;
  }
  strcat(erase, mtd);
  strcat(command, mtd);
  strcat(command, " ");
  strcat(command, path);
  dbg_printf("update command is %s\n", command);

#ifndef TEST
  ret = system(erase);
  if (ret < 0) {
    free(mtd);
    fclose(fp);
    return -1;
  }

  ret = system(command);
#endif
  free(mtd);
  fclose(fp);

  return ret < 0 ? -1 : 0;

}
#endif

int main()
{
  mqd_t msg_queue;
  struct mq_attr attr;
  int ret, pathlen;
  char tmpbuf[MAX_SIZE];
  char *desc_path, *kern_path, *fs_path;
  char *pba_path, *hal_path, *bst_path, *bld_path;

  attr = (struct mq_attr ) { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = 128,
                             .mq_curmsgs = 0 };

  mq_unlink(QUEUE_NAME);
  msg_queue = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);
  if (msg_queue < 0) {
    printf("Failed to create msgQueue %d\n", msg_queue);
    return -1;
  }

  while (1) {
    kernel_flag = fs_flag = bld_flag = hal_flag = pba_flag = bst_flag = 0;
    ret = mq_receive(msg_queue, tmpbuf, MAX_SIZE, NULL);
    if (ret < 0) {
      perror("Receive msg error\n");
      continue;
    }
    pathlen = strlen(tmpbuf) + 1;
    desc_path = malloc(pathlen + 1 + strlen(DESC_NAME));
    if (!desc_path) {
      perror("No mem\n");
      continue;
    }

    strcpy(desc_path, tmpbuf);
    strcat(desc_path, "/");
    strcat(desc_path, DESC_NAME);
    dbg_printf("path is %s\n", desc_path);

    ret = check_desc(desc_path);
    free(desc_path);
    if (ret < 0) {
      printf("Check desc file failed, give up this upgrade\n");
      continue;
    }

    if (kernel_flag) {
      printf("Check kernel\n");
      kern_path = malloc(pathlen + 1 + strlen(KERN_NAME));
      if (!kern_path) {
        perror("No mem\n");
        continue;
      }

      strcpy(kern_path, tmpbuf);
      strcat(kern_path, "/");
      strcat(kern_path, KERN_NAME);
      ret = check_file(kern_path, kernel_flag);
      free(kern_path);
      if (ret < 0) {
        printf("Warning! check kernel failed, give up this update!\n");
        continue;
      } else
        printf("Check kernel OK.\n");
    }

    if (fs_flag) {
      printf("Check filesystem\n");
      fs_path = malloc(pathlen + 1 + strlen(FS_NAME));
      if (!fs_path) {
        perror("No mem\n");
        continue;
      }

      strcpy(fs_path, tmpbuf);
      strcat(fs_path, "/");
      strcat(fs_path, FS_NAME);
      ret = check_file(fs_path, fs_flag);
      free(fs_path);
      if (ret < 0) {
        printf("Warning! check fs failed, give up this upgrade!\n");
        continue;
      } else
        printf("Check fs OK.\n");

    }

    if (pba_flag) {
      printf("Check pba image\n");
      pba_path = malloc(pathlen + 1 + strlen(PBA_NAME));
      if (!pba_path) {
        perror("No mem\n");
        continue;
      }

      strcpy(pba_path, tmpbuf);
      strcat(pba_path, "/");
      strcat(pba_path, PBA_NAME);
      ret = check_file(pba_path, pba_flag);
      if (ret < 0) {
        printf("Warning! check pba kernel failed, give up this upgrade!\n");
        free(pba_path);
        continue;
      } else {
        printf("Check pba OK...do pba update first!!!\n");
#ifndef TEST
        ret = do_pba_update(pba_path);
#endif
        if (ret < 0) {
          printf("Warning! Pba update failed! give up this upgrade\n");
          free(pba_path);
          continue;
        }
      }
    }

    if (hal_flag) {
      printf("Check hal\n");
      hal_path = malloc(pathlen + 1 + strlen(HAL_NAME));
      if (!hal_path) {
        perror("No mem\n");
        continue;
      }

      strcpy(hal_path, tmpbuf);
      strcat(hal_path, "/");
      strcat(hal_path, HAL_NAME);
      ret = check_file(hal_path, hal_flag);
      free(hal_path);
      if (ret < 0) {
        printf("Warning! check hal failed, give up this upgrade!\n");
        continue;
      } else
        printf("Check hal OK.\n");
    }

    if (bst_flag) {
      printf("Check bst\n");
      bst_path = malloc(pathlen + 1 + strlen(BST_NAME));
      if (!bst_path) {
        perror("No mem\n");
        continue;
      }

      strcpy(bst_path, tmpbuf);
      strcat(bst_path, "/");
      strcat(bst_path, BST_NAME);
      ret = check_file(bst_path, bst_flag);
      free(bst_path);
      if (ret < 0) {
        printf("Warning! check bst failed, give up this upgrade!\n");
        continue;
      } else
        printf("Check bst OK.\n");
    }

    if (bld_flag) {
      printf("Check bst\n");
      bld_path = malloc(pathlen + 1 + strlen(BLD_NAME));
      if (!bld_path) {
        perror("No mem\n");
        continue;
      }

      strcpy(bld_path, tmpbuf);
      strcat(bld_path, "/");
      strcat(bld_path, BLD_NAME);
      ret = check_file(bld_path, bld_flag);
      free(bld_path);
      if (ret < 0) {
        printf("Warning! check bld failed, give up this upgrade!\n");
        continue;
      } else
        printf("Check bld OK.\n");
    }

#ifndef TEST
    do_reboot_for_update();
#endif

  }

  mq_close(msg_queue);
  return 0;
}
