/**
 * \file
 *         Header file for the RF2500-specific rtimer code
 * \author
 *         Wincent Balin <wincent.balin@gmail.com>
 */

#ifndef __RTIMER_ARCH_H__
#define __RTIMER_ARCH_H__

#include <io.h>
#include "sys/rtimer.h"

#define RTIMER_ARCH_SECOND 12000

#define rtimer_arch_now() (TAR)

#endif /* __RTIMER_ARCH_H__ */
