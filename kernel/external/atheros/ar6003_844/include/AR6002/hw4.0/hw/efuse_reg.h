// ------------------------------------------------------------------
// Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6003
// </summary>
//
// ------------------------------------------------------------------
//===================================================================
// Author(s): ="Atheros"
//===================================================================


#ifndef _EFUSE_REG_REG_H_
#define _EFUSE_REG_REG_H_

#define EFUSE_WR_ENABLE_REG_ADDRESS              0x00000000
#define EFUSE_WR_ENABLE_REG_OFFSET               0x00000000
#define EFUSE_WR_ENABLE_REG_V_MSB                0
#define EFUSE_WR_ENABLE_REG_V_LSB                0
#define EFUSE_WR_ENABLE_REG_V_MASK               0x00000001
#define EFUSE_WR_ENABLE_REG_V_GET(x)             (((x) & EFUSE_WR_ENABLE_REG_V_MASK) >> EFUSE_WR_ENABLE_REG_V_LSB)
#define EFUSE_WR_ENABLE_REG_V_SET(x)             (((x) << EFUSE_WR_ENABLE_REG_V_LSB) & EFUSE_WR_ENABLE_REG_V_MASK)

#define EFUSE_INT_ENABLE_REG_ADDRESS             0x00000004
#define EFUSE_INT_ENABLE_REG_OFFSET              0x00000004
#define EFUSE_INT_ENABLE_REG_V_MSB               0
#define EFUSE_INT_ENABLE_REG_V_LSB               0
#define EFUSE_INT_ENABLE_REG_V_MASK              0x00000001
#define EFUSE_INT_ENABLE_REG_V_GET(x)            (((x) & EFUSE_INT_ENABLE_REG_V_MASK) >> EFUSE_INT_ENABLE_REG_V_LSB)
#define EFUSE_INT_ENABLE_REG_V_SET(x)            (((x) << EFUSE_INT_ENABLE_REG_V_LSB) & EFUSE_INT_ENABLE_REG_V_MASK)

#define EFUSE_INT_STATUS_REG_ADDRESS             0x00000008
#define EFUSE_INT_STATUS_REG_OFFSET              0x00000008
#define EFUSE_INT_STATUS_REG_V_MSB               0
#define EFUSE_INT_STATUS_REG_V_LSB               0
#define EFUSE_INT_STATUS_REG_V_MASK              0x00000001
#define EFUSE_INT_STATUS_REG_V_GET(x)            (((x) & EFUSE_INT_STATUS_REG_V_MASK) >> EFUSE_INT_STATUS_REG_V_LSB)
#define EFUSE_INT_STATUS_REG_V_SET(x)            (((x) << EFUSE_INT_STATUS_REG_V_LSB) & EFUSE_INT_STATUS_REG_V_MASK)

#define BITMASK_WR_REG_ADDRESS                   0x0000000c
#define BITMASK_WR_REG_OFFSET                    0x0000000c
#define BITMASK_WR_REG_V_MSB                     31
#define BITMASK_WR_REG_V_LSB                     0
#define BITMASK_WR_REG_V_MASK                    0xffffffff
#define BITMASK_WR_REG_V_GET(x)                  (((x) & BITMASK_WR_REG_V_MASK) >> BITMASK_WR_REG_V_LSB)
#define BITMASK_WR_REG_V_SET(x)                  (((x) << BITMASK_WR_REG_V_LSB) & BITMASK_WR_REG_V_MASK)

#define VDDQ_SETTLE_TIME_REG_ADDRESS             0x00000010
#define VDDQ_SETTLE_TIME_REG_OFFSET              0x00000010
#define VDDQ_SETTLE_TIME_REG_V_MSB               31
#define VDDQ_SETTLE_TIME_REG_V_LSB               0
#define VDDQ_SETTLE_TIME_REG_V_MASK              0xffffffff
#define VDDQ_SETTLE_TIME_REG_V_GET(x)            (((x) & VDDQ_SETTLE_TIME_REG_V_MASK) >> VDDQ_SETTLE_TIME_REG_V_LSB)
#define VDDQ_SETTLE_TIME_REG_V_SET(x)            (((x) << VDDQ_SETTLE_TIME_REG_V_LSB) & VDDQ_SETTLE_TIME_REG_V_MASK)

#define RD_STROBE_PW_REG_ADDRESS                 0x00000014
#define RD_STROBE_PW_REG_OFFSET                  0x00000014
#define RD_STROBE_PW_REG_V_MSB                   31
#define RD_STROBE_PW_REG_V_LSB                   0
#define RD_STROBE_PW_REG_V_MASK                  0xffffffff
#define RD_STROBE_PW_REG_V_GET(x)                (((x) & RD_STROBE_PW_REG_V_MASK) >> RD_STROBE_PW_REG_V_LSB)
#define RD_STROBE_PW_REG_V_SET(x)                (((x) << RD_STROBE_PW_REG_V_LSB) & RD_STROBE_PW_REG_V_MASK)

#define PG_STROBE_PW_REG_ADDRESS                 0x00000018
#define PG_STROBE_PW_REG_OFFSET                  0x00000018
#define PG_STROBE_PW_REG_V_MSB                   31
#define PG_STROBE_PW_REG_V_LSB                   0
#define PG_STROBE_PW_REG_V_MASK                  0xffffffff
#define PG_STROBE_PW_REG_V_GET(x)                (((x) & PG_STROBE_PW_REG_V_MASK) >> PG_STROBE_PW_REG_V_LSB)
#define PG_STROBE_PW_REG_V_SET(x)                (((x) << PG_STROBE_PW_REG_V_LSB) & PG_STROBE_PW_REG_V_MASK)

#define EFUSE_INTF_ADDRESS                       0x00000800
#define EFUSE_INTF_OFFSET                        0x00000800
#define EFUSE_INTF_R_MSB                         31
#define EFUSE_INTF_R_LSB                         0
#define EFUSE_INTF_R_MASK                        0xffffffff
#define EFUSE_INTF_R_GET(x)                      (((x) & EFUSE_INTF_R_MASK) >> EFUSE_INTF_R_LSB)
#define EFUSE_INTF_R_SET(x)                      (((x) << EFUSE_INTF_R_LSB) & EFUSE_INTF_R_MASK)


#ifndef __ASSEMBLER__

typedef struct efuse_reg_reg_s {
  volatile unsigned int efuse_wr_enable_reg;
  volatile unsigned int efuse_int_enable_reg;
  volatile unsigned int efuse_int_status_reg;
  volatile unsigned int bitmask_wr_reg;
  volatile unsigned int vddq_settle_time_reg;
  volatile unsigned int rd_strobe_pw_reg;
  volatile unsigned int pg_strobe_pw_reg;
  unsigned char pad0[2020]; /* pad to 0x800 */
  volatile unsigned int efuse_intf[512];
} efuse_reg_reg_t;

#endif /* __ASSEMBLER__ */

#endif /* _EFUSE_REG_H_ */
