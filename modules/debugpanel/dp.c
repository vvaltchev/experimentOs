/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck_gen_headers/mod_tracing.h>

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>
#include <tilck/common/syscalls.h>

#include <tilck/kernel/modules.h>
#include <tilck/kernel/debug_utils.h>
#include <tilck/kernel/irq.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/timer.h>
#include <tilck/kernel/kb.h>
#include <tilck/kernel/system_mmap.h>
#include <tilck/kernel/term.h>
#include <tilck/kernel/elf_utils.h>
#include <tilck/kernel/tty.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/cmdline.h>
#include <tilck/kernel/tty_struct.h>
#include <tilck/kernel/fs/devfs.h>

#include <tilck/mods/fb_console.h>
#include <tilck/mods/tracing.h>

#include "termutil.h"
#include "dp_int.h"

int dp_rows;
int dp_cols;
int dp_start_row;
int dp_end_row;
int dp_start_col;
int dp_screen_start_row;
int dp_screen_rows;
bool ui_need_update;
bool dp_in_tracing_screen;
const char *modal_msg;
struct dp_screen *dp_ctx;
fs_handle dp_input_handle;

static bool skip_next_keypress;
static ATOMIC(bool) dp_running;
static struct list dp_screens_list = STATIC_LIST_INIT(dp_screens_list);

static inline void
dp_write_header(int i, const char *s, bool selected)
{
   if (selected) {

      dp_write_raw(
         E_COLOR_BR_WHITE "%d" REVERSE_VIDEO "[%s]" RESET_ATTRS " ",
         i, s
      );

   } else {
      dp_write_raw("%d[%s]" RESET_ATTRS " ", i, s);
   }
}

static void dp_enter(void)
{
   struct term_params tparams;
   struct dp_screen *pos;

   process_term_read_info(&tparams);
   dp_set_cursor_enabled(false);

   if (tparams.type == term_type_video)
      dp_switch_to_alt_buffer();

   dp_rows = tparams.rows;
   dp_cols = tparams.cols;
   dp_start_row = (dp_rows - DP_H) / 2 + 1;
   dp_start_col = (dp_cols - DP_W) / 2 + 1;
   dp_end_row = dp_start_row + DP_H;
   dp_screen_start_row = dp_start_row + 3;
   dp_screen_rows = (DP_H - 2 - (dp_screen_start_row - dp_start_row));

   list_for_each_ro(pos, &dp_screens_list, node) {

      pos->row_off = 0;
      pos->row_max = 0;

      if (pos->on_dp_enter)
         pos->on_dp_enter();
   }
}

static void dp_exit(void)
{
   struct term_params tparams;
   struct dp_screen *pos;

   process_term_read_info(&tparams);

   list_for_each_ro(pos, &dp_screens_list, node) {

      if (pos->on_dp_exit)
         pos->on_dp_exit();
   }

   if (tparams.type == term_type_video) {
      dp_switch_to_default_buffer();
   } else {
      dp_clear();
      dp_write_raw("\n");
   }

   dp_set_cursor_enabled(true);
}

void dp_register_screen(struct dp_screen *screen)
{
   struct dp_screen *pos;
   struct list_node *pred = (struct list_node *)&dp_screens_list;

   list_for_each_ro(pos, &dp_screens_list, node) {

      if (pos->index < screen->index)
         pred = &pos->node;

      if (pos->index == screen->index)
         panic("[debugpanel] Index conflict while registering %s at %d",
               screen->label, screen->index);
   }

   list_add_after(pred, &screen->node);
}

static void redraw_screen(void)
{
   struct dp_screen *pos;
   char buf[64];
   int rc;

   dp_clear();
   dp_move_cursor(dp_start_row + 1, dp_start_col + 2);

   list_for_each_ro(pos, &dp_screens_list, node) {
      dp_write_header(pos->index+1, pos->label, pos == dp_ctx);
   }

   dp_write_raw("q[Quit]" RESET_ATTRS " ");
   dp_ctx->draw_func();

   dp_draw_rect_raw(dp_start_row, dp_start_col, DP_H, DP_W);
   dp_move_cursor(dp_start_row, dp_start_col + 2);
   dp_write_raw(E_COLOR_YELLOW "[ TilckDebugPanel ]" RESET_ATTRS);

   rc = snprintk(buf, sizeof(buf),
                 "[rows %02d - %02d of %02d]",
                 dp_ctx->row_off + 1,
                 MIN(dp_ctx->row_off + dp_screen_rows, dp_ctx->row_max) + 1,
                 dp_ctx->row_max + 1);

   dp_move_cursor(dp_end_row - 1, dp_start_col + DP_W - rc - 2);
   dp_write_raw(E_COLOR_BR_RED "%s" RESET_ATTRS, buf);
   dp_move_cursor(dp_rows, 1);
   ui_need_update = false;
}

static void
dp_main_handle_keypress(struct key_event ke)
{
   int rc;

   if ('0' <= ke.print_char && ke.print_char <= '9') {

      struct dp_screen *pos;
      rc = ke.print_char - '0';

      list_for_each_ro(pos, &dp_screens_list, node) {

         if (pos->index == rc - 1 && pos != dp_ctx) {
            dp_ctx = pos;
            ui_need_update = true;
            break;
         }
      }

   } else if (ke.key == KEY_PAGE_DOWN) {

      if (dp_ctx->row_off + dp_screen_rows < dp_ctx->row_max) {
         dp_ctx->row_off++;
         ui_need_update = true;
      }

   } else if (ke.key == KEY_PAGE_UP) {

      if (dp_ctx->row_off > 0) {
         dp_ctx->row_off--;
         ui_need_update = true;
      }
   }
}

static int
dp_main_body(enum term_type tt, struct key_event ke)
{
   int rc;
   bool dp_screen_key_handled = false;

   if (!skip_next_keypress) {

      dp_main_handle_keypress(ke);

      if (!ui_need_update && dp_ctx->on_keypress_func) {

         rc = dp_ctx->on_keypress_func(ke);

         if (rc == kb_handler_ok_and_stop)
            return 1; /* skip redraw_screen() */

         if (rc != kb_handler_nak)
            dp_screen_key_handled = true;
      }

   } else {
      skip_next_keypress = false;
      ui_need_update = true;
   }

   if (ui_need_update || modal_msg) {

      if (tt == term_type_video)
         term_pause_output();

      redraw_screen();

      if (modal_msg) {
         dp_show_modal_msg(modal_msg);
         modal_msg = NULL;
         skip_next_keypress = true;
      }

      if (tt == term_type_video)
         term_restart_output();
   }

   return dp_screen_key_handled;
}

void dp_set_input_blocking(bool blocking)
{
   struct fs_handle_base *h = dp_input_handle;

   disable_preemption();
   {
      if (blocking)
         h->fl_flags &= ~O_NONBLOCK;
      else
         h->fl_flags |= O_NONBLOCK;
   }
   enable_preemption();
}

static void
dp_common_entry(bool only_tracing)
{
   enum term_type tt;
   struct key_event ke;
   struct tty *t = get_curr_process_tty();
   int rc;

   if (dp_running)
      return;

   if (!t) {
      printk("ERROR: debugpanel: the current process has no attached TTY\n");
      return;
   }

   disable_preemption();
   {
      devfs_kernel_create_handle_for(t->devfile, &dp_input_handle);
   }
   enable_preemption();

   if (!dp_input_handle) {
      printk("ERROR: debugpanel: cannot open process's TTY\n");
      return;
   }

   dp_running = true;
   tt = t->tparams.type;
   dp_ctx = list_first_obj(&dp_screens_list, struct dp_screen, node);
   dp_enter();

   disable_preemption();
   {
      tty_set_raw_mode(t);
      dp_set_input_blocking(false);
   }
   enable_preemption();

   bzero(&ke, sizeof(ke));
   ui_need_update = true;

   if (only_tracing) {

      if (MOD_tracing)
         dp_tracing_screen();

   } else {

      while (true) {

         rc = dp_main_body(tt, ke);

         if (!rc && ke.print_char == 'q')
            break;

         if (dp_read_ke_from_tty(&ke) < 0)
            break;

         if (ke.print_char == DP_KEY_CTRL_C)
            break;
      }
   }

   disable_preemption();
   {
      dp_set_input_blocking(true);
      tty_reset_termios(get_curr_process_tty());
   }
   enable_preemption();
   dp_exit();
   devfs_kernel_destory_handle(dp_input_handle);
   dp_running = false;
}

static void
dp_default_entry()
{
   dp_common_entry(false);
}

static void
dp_direct_tracing_mode_entry()
{
   dp_common_entry(true);
}

static void dp_init(void)
{
   struct dp_screen *pos;
   register_tilck_cmd(TILCK_CMD_DEBUG_PANEL, dp_default_entry);

   if (MOD_tracing)
      register_tilck_cmd(TILCK_CMD_TRACING_TOOL, dp_direct_tracing_mode_entry);

   list_for_each_ro(pos, &dp_screens_list, node) {

      if (pos->first_setup)
         pos->first_setup();
   }
}

static struct module dp_module = {

   .name = "debugpanel",
   .priority = MOD_dp_prio,
   .init = &dp_init,
};

REGISTER_MODULE(&dp_module);
