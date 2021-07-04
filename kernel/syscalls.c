/* SPDX-License-Identifier: BSD-2-Clause */

#define __SYSCALLS_C__

#include <tilck/common/basic_defs.h>
#include <tilck/common/build_info.h>

#include <tilck/kernel/syscalls.h>
#include <tilck/kernel/debug_utils.h>
#include <tilck/kernel/user.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/signal.h>
#include <tilck/kernel/timer.h>
#include <tilck/kernel/fs/vfs.h>

#define LINUX_REBOOT_MAGIC1         0xfee1dead
#define LINUX_REBOOT_MAGIC2          672274793
#define LINUX_REBOOT_MAGIC2A          85072278
#define LINUX_REBOOT_MAGIC2B         369367448
#define LINUX_REBOOT_MAGIC2C         537993216

#define LINUX_REBOOT_CMD_RESTART     0x1234567
#define LINUX_REBOOT_CMD_RESTART2   0xa1b2c3d4
#define LINUX_REBOOT_CMD_HALT       0xcdef0123
#define LINUX_REBOOT_CMD_POWER_OFF  0x4321fedc

int sys_madvise(void *addr, size_t len, int advice)
{
   // TODO (future): consider implementing at least part of sys_madvice().
   return 0;
}

int
do_nanosleep(const struct k_timespec64 *req)
{
   u64 ticks_to_sleep = 0;

   ticks_to_sleep += (ulong) TIMER_HZ * (ulong) req->tv_sec;
   ticks_to_sleep += (ulong) req->tv_nsec / (1000000000 / TIMER_HZ);
   kernel_sleep(ticks_to_sleep);

   if (pending_signals())
      return -EINTR;

   // TODO (future): use HPET in order to improve the sleep precision
   // TODO (nanosleep): set rem if the call has been interrupted by a signal
   return 0;
}

int
sys_nanosleep_time32(const struct k_timespec32 *user_req,
                     struct k_timespec32 *rem)
{
   struct k_timespec32 req32;
   struct k_timespec64 req;
   int rc;

   if (copy_from_user(&req32, user_req, sizeof(req)))
      return -EFAULT;

   req = (struct k_timespec64) {
      .tv_sec = req32.tv_sec,
      .tv_nsec = req32.tv_nsec,
   };

   if ((rc = do_nanosleep(&req)))
      return rc;

   bzero(&req32, sizeof(req32));

   if (copy_to_user(rem, &req32, sizeof(req32)))
      return -EFAULT;

   return 0;
}

int sys_newuname(struct utsname *user_buf)
{
   struct commit_hash_and_date comm;
   struct utsname buf = {0};

   extract_commit_hash_and_date(&tilck_build_info, &comm);

   strcpy(buf.sysname, "Tilck");
   strcpy(buf.nodename, "tilck");
   strcpy(buf.version, comm.hash);
   strcpy(buf.release, tilck_build_info.ver);
   strcpy(buf.machine, tilck_build_info.arch);

   if (copy_to_user(user_buf, &buf, sizeof(struct utsname)) < 0)
      return -EFAULT;

   return 0;
}

NORETURN int sys_exit(int exit_status)
{
   terminate_process(exit_status, 0 /* term_sig */);

   /* Necessary to guarantee to the compiler that we won't return. */
   NOT_REACHED();
}

NORETURN int sys_exit_group(int status)
{
   // TODO: update when user threads are supported
   sys_exit(status);
}


/* NOTE: deprecated syscall */
int sys_tkill(int tid, int sig)
{
   if (!IN_RANGE(sig, 0, _NSIG) || tid <= 0)
      return -EINVAL;

   return send_signal(tid, sig, false);
}

int sys_tgkill(int pid /* linux: tgid */, int tid, int sig)
{
   if (pid != tid) {
      printk("sys_tgkill: pid != tid NOT SUPPORTED yet.\n");
      return -EINVAL;
   }

   if (!IN_RANGE(sig, 0, _NSIG) || pid <= 0 || tid <= 0)
      return -EINVAL;

   return send_signal2(pid, tid, sig, false);
}


int sys_kill(int pid, int sig)
{
   if (!IN_RANGE(sig, 0, _NSIG))
      return -EINVAL;

   if (pid == 0)
      return send_signal_to_group(get_curr_proc()->pgid, sig);

   if (pid == -1) {

      /*
       * In theory, pid == -1, means:
       *    "sig is sent to every process for which the calling process has
       *     permission to send signals, except for process 1 (init)".
       *
       * But Tilck does not have a permission model nor multi-user support.
       * So, what to do here? Kill everything except init? Mmh, not sure.
       * It looks acceptable for the moment to just kill all the processes in
       * the current session.
       */
      return send_signal_to_session(get_curr_proc()->sid, sig);
   }

   if (pid < -1)
      return send_signal_to_group(-pid, sig);

   /* pid > 0 */
   return send_signal(pid, sig, true);
}

ulong sys_times(struct tms *user_buf)
{
   struct task *curr = get_curr_task();
   struct tms buf;

   // TODO (threads): when threads are supported, update sys_times()
   // TODO: consider supporting tms_cutime and tms_cstime in sys_times()

   disable_preemption();
   {

      buf = (struct tms) {
         .tms_utime = (clock_t) curr->ticks.total,
         .tms_stime = (clock_t) curr->ticks.total_kernel,
         .tms_cutime = 0,
         .tms_cstime = 0,
      };

   }
   enable_preemption();

   if (copy_to_user(user_buf, &buf, sizeof(buf)) != 0)
      return (ulong) -EBADF;

   return (ulong) get_ticks();
}

int sys_fork(void)
{
   return do_fork(false);
}

int sys_vfork(void)
{
   return do_fork(true);
}

static void
kernel_shutdown(void)
{
   /* This is just a stub */
}

int sys_reboot(u32 magic, u32 magic2, u32 cmd, void *arg)
{
   if (magic != LINUX_REBOOT_MAGIC1)
      return -EINVAL;

   if (magic2 != LINUX_REBOOT_MAGIC2  &&
       magic2 != LINUX_REBOOT_MAGIC2A &&
       magic2 != LINUX_REBOOT_MAGIC2B &&
       magic2 != LINUX_REBOOT_MAGIC2C)
   {
      return -EINVAL;
   }

   switch (cmd) {

      case LINUX_REBOOT_CMD_RESTART:
      case LINUX_REBOOT_CMD_RESTART2:
         kernel_shutdown();
         reboot();
         break;

      case LINUX_REBOOT_CMD_HALT:
         kernel_shutdown();
         disable_interrupts_forced();
         while (true) { halt(); }
         break;

      case LINUX_REBOOT_CMD_POWER_OFF:
         kernel_shutdown();
         poweroff();
         break;

      default:
         return -EINVAL;
   }

   return 0;
}

int sys_sched_yield(void)
{
   kernel_yield();
   return 0;
}

int sys_utimes(const char *u_path, const struct timeval u_times[2])
{
   struct timeval ts[2];
   struct k_timespec64 new_ts[2];
   char *path = get_curr_task()->args_copybuf;

   if (copy_str_from_user(path, u_path, MAX_PATH, NULL))
      return -EFAULT;

   if (u_times) {

      if (copy_from_user(ts, u_times, sizeof(ts)))
         return -EFAULT;

      new_ts[0] = (struct k_timespec64) {
         .tv_sec = ts[0].tv_sec,
         .tv_nsec = ((long)ts[0].tv_usec) * 1000,
      };

      new_ts[1] = (struct k_timespec64) {
         .tv_sec = ts[1].tv_sec,
         .tv_nsec = ((long)ts[1].tv_usec) * 1000,
      };

   } else {

      /*
       * If `u_times` is NULL, the access and modification times of the file
       * are set to the current time.
       */

      real_time_get_timespec(&new_ts[0]);
      new_ts[1] = new_ts[0];
   }

   return vfs_utimens(path, new_ts);
}

int sys_utime(const char *u_path, const struct utimbuf *u_times)
{
   struct utimbuf ts;
   struct k_timespec64 new_ts[2];
   char *path = get_curr_task()->args_copybuf;

   if (copy_from_user(&ts, u_times, sizeof(ts)))
      return -EFAULT;

   if (copy_str_from_user(path, u_path, MAX_PATH, NULL))
      return -EFAULT;

   new_ts[0] = (struct k_timespec64) {
      .tv_sec = ts.actime,
      .tv_nsec = 0,
   };

   new_ts[1] = (struct k_timespec64) {
      .tv_sec = ts.modtime,
      .tv_nsec = 0,
   };

   return vfs_utimens(path, new_ts);
}

int sys_utimensat_time32(int dirfd, const char *u_path,
                         const struct k_timespec32 times[2], int flags)
{
   // TODO (future): consider implementing sys_utimensat() [modern]
   return -ENOSYS;
}

int sys_futimesat(int dirfd, const char *u_path,
                  const struct timeval times[2])
{
   // TODO (future): consider implementing sys_futimesat() [obsolete]
   return -ENOSYS;
}

int sys_socketcall(int call, ulong *args)
{
   return -ENOSYS;
}
