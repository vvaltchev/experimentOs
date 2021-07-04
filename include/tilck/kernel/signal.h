/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#include <tilck/common/basic_defs.h>

#include <signal.h>                   // system header
#include <asm-generic/signal-defs.h>  // system header

int send_signal_to_group(int pgid, int sig);
int send_signal_to_session(int sid, int sig);
int send_signal2(int pid, int tid, int signum, bool whole_process);
void process_signals(void);

static inline int send_signal(int tid, int signum, bool whole_process)
{
   return send_signal2(tid, tid, signum, whole_process);
}

#define K_SIGACTION_MASK_WORDS                                             2
