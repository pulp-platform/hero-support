/* Copyright (C) 2017 ETH Zurich, University of Bologna
 * All rights reserved.
 *
 * This code is under development and not yet released to the public.
 * Until it is released, the code is under the copyright of ETH Zurich and
 * the University of Bologna, and may contain confidential and/or unpublished 
 * work. Any reuse/redistribution is strictly forbidden without written
 * permission from ETH Zurich.
 *
 * Bug fixes and contributions will eventually be released under the
 * SolderPad open hardware license in the context of the PULP platform
 * (http://www.pulp-platform.org), under the copyright of ETH Zurich and the
 * University of Bologna.
 */
#ifndef _PULP_MBOX_H_
#define _PULP_MBOX_H_

#include <linux/spinlock.h>   /* locking in interrupt context*/
#include <asm/uaccess.h>      /* copy_to_user, copy_from_user, access_ok */
#include <asm/io.h>           /* ioremap, iounmap, iowrite32 */
#include <linux/fs.h>         /* file_operations struct, struct file */
#include <linux/sched.h>      /* wake_up_interruptible(), TASK_INTERRUPTIBLE */

#include "pulp_module.h"

#include "pulp_host.h"

void pulp_mbox_init(void *mbox);
void pulp_mbox_clear(void);
void pulp_mbox_intr(void *mbox);
ssize_t pulp_mbox_read(struct file *filp, char __user *buf, size_t count, loff_t *offp);

#endif // _PULP_MBOX_H_ 
