/*
 * am_qrcode.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 23/11/2012 [Created]
 *          12/03/2013 [Modified]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __QRCODE_READER_H__
#define __QRCODE_READER_H__

class AmQrcodeReader
{
public:
   AmQrcodeReader ();
   virtual ~AmQrcodeReader ();

public:
   bool set_vin_config_path (const char *);
   bool qrcode_read (const char *);

private:
   char *vin_config_path;
};

#endif
