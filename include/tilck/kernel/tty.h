/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>
#include <tilck/kernel/kb.h>

struct tty;

static ALWAYS_INLINE struct tty *get_curr_tty(void)
{
   extern struct tty *__curr_tty;
   return __curr_tty;
}

void tty_send_keyevent(struct tty *t, struct key_event ke, bool block);
void tty_setup_for_panic(struct tty *t);
int tty_get_num(struct tty *t);
void tty_restore_kd_text_mode(struct tty *t);
struct tty *get_serial_tty(int n);
void tty_reset_termios(struct tty *t);
struct tty *get_curr_process_tty(void);
int get_curr_proc_tty_term_type(void);
ssize_t tty_curr_proc_write(const char *buf, size_t size);
void tty_write_on_all_ttys(const char *buf, size_t size);

static inline int get_curr_tty_num(void)
{
   return tty_get_num(get_curr_tty());
}

/* Used only by the debug panel */
int set_curr_tty(struct tty *t);
struct tty *create_tty_nodev(void);
void tty_set_raw_mode(struct tty *t);
void tty_set_medium_raw_mode(struct tty *t, bool enabled);
