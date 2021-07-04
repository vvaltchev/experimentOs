/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>
#include <tilck/kernel/signal.h>

#include <features.h>   // system header

#if defined(__GLIBC__) && !defined(__USE_LARGEFILE64)
   #define __USE_LARGEFILE64
#endif

#include <sys/types.h>  // system header
#include <sys/time.h>   // system header
#include <sys/times.h>  // system header
#include <sys/uio.h>    // system header
#include <sys/select.h> // system header
#include <time.h>       // system header
#include <poll.h>       // system header
#include <utime.h>      // system header

#ifndef __GLIBC__
   #define stat stat64
#endif

#include <sys/stat.h>   // system header

#ifndef __GLIBC__
   #undef stat
#endif

#include <unistd.h>       // system header
#include <sys/utsname.h>  // system header
#include <sys/stat.h>     // system header
#include <fcntl.h>        // system header

#define MAX_SYSCALLS 500

typedef u64 tilck_ino_t;

/* From the man page of getdents64() */
struct linux_dirent64 {
   u64            d_ino;    /* 64-bit inode number */
   u64            d_off;    /* 64-bit offset to next structure */
   unsigned short d_reclen; /* Size of this dirent */
   unsigned char  d_type;   /* File type */
   char           d_name[]; /* Filename (null-terminated) */
};

struct k_sigaction {

   union {
      void (*handler)(int);
      void (*sigact_handler)(int, void * /* siginfo */, void *);
   };

   ulong sa_flags;
   void (*restorer)(void);
   ulong sa_mask[K_SIGACTION_MASK_WORDS];
};

struct k_rusage {

   struct timeval ru_utime;
   struct timeval ru_stime;

/* linux extentions */
   long ru_maxrss;
   long ru_ixrss;
   long ru_idrss;
   long ru_isrss;
   long ru_minflt;
   long ru_majflt;
   long ru_nswap;
   long ru_inblock;
   long ru_oublock;
   long ru_msgsnd;
   long ru_msgrcv;
   long ru_nsignals;
   long ru_nvcsw;
   long ru_nivcsw;

   long reserved[16];
};

#ifdef BITS32
   STATIC_ASSERT(sizeof(struct k_rusage) == 136);
#endif

struct k_timespec32 {

   s32 tv_sec;
   long tv_nsec;
};

struct k_timespec64 {

   s64 tv_sec;
   long tv_nsec;
};

#ifndef O_DIRECTORY
   #define O_DIRECTORY __O_DIRECTORY
#endif

#ifndef O_TMPFILE
   #define O_TMPFILE (__O_TMPFILE | O_DIRECTORY)
#endif

#ifndef O_DIRECT
   #define O_DIRECT __O_DIRECT
#endif

#ifndef O_NOATIME
   #define O_NOATIME __O_NOATIME
#endif

#ifndef O_PATH
   #define O_PATH __O_PATH
#endif

#define FCNTL_CHANGEABLE_FL (         \
   O_APPEND      |                    \
   O_ASYNC       |                    \
   O_DIRECT      |                    \
   O_NOATIME     |                    \
   O_NONBLOCK                         \
)

#define EXITCODE(ret, sig)    ((ret) << 8 | (sig))
#define STOPCODE(sig)          ((sig) << 8 | 0x7f)
#define CONTINUED                           0xffff
#define COREFLAG                              0x80
