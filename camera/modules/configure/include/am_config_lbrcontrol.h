/**
 * am_config_lbrcontrol.h
 *
 *  History:
 *		Mar 13, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AM_CONFIG_LBRCONTROL_H_
#define AM_CONFIG_LBRCONTROL_H_

class AmConfigLBR: public AmConfigBase
{
public:
   AmConfigLBR(const char *configFileName) :
      AmConfigBase(configFileName),
      mLBRParameters(NULL) {}
   virtual ~AmConfigLBR()
   {
      delete mLBRParameters;
   }

public:
   LBRParameters *get_lbr_config();
   void set_lbr_config(LBRParameters *config);

private:
   LBRParameters *mLBRParameters;
};

#endif /* AM_CONFIG_LBRCONTROL_H_ */
