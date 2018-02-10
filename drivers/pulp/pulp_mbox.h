/*
 * This file is part of the PULP device driver.
 *
 * Copyright (C) 2018 ETH Zurich, University of Bologna
 *
 * Author: Pirmin Vogel <vogelpi@iis.ee.ethz.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
