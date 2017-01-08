
#include <list.h>
#include <kmalloc.h>
#include <string_util.h>

#include <arch_utils.h>

#define TIME_SLOT_JIFFIES 500

task_info *current_process = NULL;
int current_max_pid = 0;

// Our linked list for all the tasks (processes, threads, etc.)
LIST_HEAD(tasks_list);

void save_current_process_state(regs *r)
{
   memmove(&current_process->state_regs, r, sizeof(*r));
}

void add_process(task_info *p)
{
   p->state = TASK_STATE_RUNNABLE;
   list_add_tail(&tasks_list, &p->list);
}

void remove_process(task_info *p)
{
   list_remove(&p->list);
   printk("[remove_process] pid = %i\n", p->pid);
}

NORETURN void switch_to_process(task_info *pi)
{
   ASSERT(pi->state == TASK_STATE_RUNNABLE);

   pi->state = TASK_STATE_RUNNING;

   //printk("[sched] Switching to pid: %i\n", current_process->pid);

   if (get_curr_page_dir() != pi->pdir) {
      set_page_directory(pi->pdir);
   }

   end_current_interrupt_handling();
   current_process = pi;
   context_switch(&current_process->state_regs);
}


NORETURN void schedule()
{
   task_info *const curr = current_process;
   task_info *selected = curr;
   const u64 jiffies_used = jiffies - curr->jiffies_when_switch;

   if (jiffies_used < TIME_SLOT_JIFFIES && curr->state == TASK_STATE_RUNNING) {
      curr->state = TASK_STATE_RUNNABLE;
      goto end;
   }

   printk("[sched] Current pid: %i\n", current_process->pid);
   printk("[sched] Used %llu jiffies\n", jiffies_used);

   // If we preempted the process, it is still runnable.
   if (curr->state == TASK_STATE_RUNNING) {
      curr->state = TASK_STATE_RUNNABLE;
   }

   // Actual scheduling logic.

   task_info *pos;
   list_for_each_entry(pos, &tasks_list, list) {
      if (pos != curr && pos->state == TASK_STATE_RUNNABLE) {
         selected = pos;
         break;
      }
   }

   selected->jiffies_when_switch = jiffies;

   // Finalizing code.

end:

   if (selected->state != TASK_STATE_RUNNABLE) {

      printk("[sched] No runnable process found. Halt.\n");

      end_current_interrupt_handling();
      // We did not found any runnable task. Halt.
      halt();
   }

   if (selected != curr) {
      printk("[sched] Switching to pid: %i\n", selected->pid);
   }

   switch_to_process(selected);
}



/*
 * ***************************************************************
 *
 * SYSCALLS
 *
 * ***************************************************************
 */

int sys_getpid()
{
   ASSERT(current_process != NULL);
   return current_process->pid;
}

NORETURN void sys_exit(int exit_code)
{
   printk("[kernel] Exit process %i with code = %i\n",
          current_process->pid,
          exit_code);

   current_process->state = TASK_STATE_ZOMBIE;
   current_process->exit_code = exit_code;
   pdir_destroy(current_process->pdir);
   schedule();
}

// Returns child's pid
int sys_fork()
{
   page_directory_t *pdir = pdir_clone(current_process->pdir);

   task_info *child = kmalloc(sizeof(task_info));
   INIT_LIST_HEAD(&child->list);
   child->pdir = pdir;
   child->pid = ++current_max_pid;
   memmove(&child->state_regs,
           &current_process->state_regs,
           sizeof(child->state_regs));

   set_return_register(&child->state_regs, 0);


   //printk("forking current proccess with eip = %p\n", child->state_regs.eip);

   add_process(child);

   // Make the parent to get child's pid as return value.
   set_return_register(&current_process->state_regs, child->pid);

   /*
    * Force the CR3 reflush using the current (parent's) pdir.
    * Without doing that, COW on parent's pages doesn't work immediately.
    * That is better (in this case) than invalidating all the pages affected,
    * one by one.
    */

   set_page_directory(current_process->pdir);
   return child->pid;
}
