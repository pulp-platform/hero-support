#ifndef _PULP_MBOX_H_
#define _PULP_MBOX_H_

#include <linux/spinlock.h>   /* locking in interrupt context*/
#include <asm/uaccess.h>      /* copy_to_user, copy_from_user, access_ok */
#include <asm/io.h>           /* ioremap, iounmap, iowrite32 */
#include <linux/fs.h>         /* file_operations struct, struct file */

#include "pulp_module.h"

#include "pulp_host.h"

void pulp_mbox_init(void *mbox);
void pulp_mbox_clear(void);
void pulp_mbox_intr(void *mbox);
ssize_t pulp_mbox_read(struct file *filp, char __user *buf, size_t count, loff_t *offp);

#endif // _PULP_MBOX_H_ 
