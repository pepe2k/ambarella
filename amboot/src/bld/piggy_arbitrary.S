/**
 * system/src/bld/piggy_arbitrary.S
 *
 * History:
 *    2005/09/09 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <fio/firmfl.h>

piggy_arbitrary:

#ifndef INCBIN
#error "no data to embed!"
#endif

.text

.incbin xstr(INCBIN)

.end
