/*
 * version.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 30/11/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_include.h"
#include "am_utility.h"
#include "am_qrcode.h"

#ifndef SIMPLE_CAMERA_VIN_CONFIG
#define SIMPLE_CAMERA_VIN_CONFIG \
   BUILD_AMBARELLA_CAMERA_CONF_DIR"/video_vin.conf"
#endif

int
main (int argc, char **argv)
{
   if (argc != 2) {
      NOTICE ("Usage: test_qrcode <file_name>");
      return -1;
   }

   AmQrcodeReader *pQrcode = NULL;
   if ((pQrcode = new AmQrcodeReader ()) == NULL) {
      ERROR ("Failed to create an instance of qrcode!");
      return -1;
   }

   pQrcode->set_vin_config_path (SIMPLE_CAMERA_VIN_CONFIG);
   if (pQrcode->qrcode_read (argv[1])) {
      NOTICE ("Please see reading result from: %s", argv[1]);
   } else {
      ERROR ("Failed to read qr code!");
   }

   delete pQrcode;
   return 0;
}
