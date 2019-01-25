/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/string_util.h>

#include <tilck/kernel/fs/vfs.h>
#include <tilck/kernel/fs/devfs.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/process.h>

#include "tty_int.h"

static inline tty *get_curr_process_tty(void)
{
   return get_curr_task()->pi->proc_tty;
}

static ssize_t ttydev_read(fs_handle h, char *buf, size_t size)
{
   return tty_read_int(get_curr_process_tty(), h, buf, size);
}

static ssize_t ttydev_write(fs_handle h, char *buf, size_t size)
{
   return tty_write_int(get_curr_process_tty(), h, buf, size);
}

static int ttydev_ioctl(fs_handle h, uptr request, void *argp)
{
   return tty_ioctl_int(get_curr_process_tty(), h, request, argp);
}

static int ttydev_fcntl(fs_handle h, int cmd, uptr arg)
{
   return tty_fcntl_int(get_curr_process_tty(), h, cmd, arg);
}

static int
ttydev_create_device_file(int minor, file_ops *ops, devfs_entry_type *t)
{
   *t = DEVFS_CHAR_DEVICE;

   bzero(ops, sizeof(file_ops));

   ops->read = ttydev_read;
   ops->write = ttydev_write;
   ops->ioctl = ttydev_ioctl;
   ops->fcntl = ttydev_fcntl;

   /* the tty device-file requires NO locking */
   ops->exlock = &vfs_file_nolock;
   ops->exunlock = &vfs_file_nolock;
   ops->shlock = &vfs_file_nolock;
   ops->shunlock = &vfs_file_nolock;
   return 0;
}

/*
 * Creates the special /dev/tty file which redirects the tty_* funcs to the
 * tty that was current when the process was created.
 */
void init_tty_dev(void)
{
   int major;
   driver_info *di = kzmalloc(sizeof(driver_info));

   if (!di)
      panic("TTY: no enough memory for driver_info");

   di->name = "ttydev";
   di->create_dev_file = ttydev_create_device_file;
   major = register_driver(di, 5);

   internal_tty_create_devfile("tty", major, 0);
   internal_tty_create_devfile("console", major, 1);
}