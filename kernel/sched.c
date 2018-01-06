#include <list.h>
#include <kmalloc.h>
#include <string_util.h>
#include <process.h>
#include <hal.h>

//#define DEBUG_printk printk
#define DEBUG_printk(...)


// low debug value
//#define TIME_SLOT_JIFFIES (TIMER_HZ * 1)

// correct value (20 ms)
#define TIME_SLOT_JIFFIES (TIMER_HZ / 50)

// high debug value
//#define TIME_SLOT_JIFFIES (1)

task_info *volatile current = NULL;
int current_max_pid = 0;

// Our linked list for all the tasks (processes, threads, etc.)
list_node tasks_list = make_list_node(tasks_list);
list_node runnable_tasks_list = make_list_node(runnable_tasks_list);
list_node sleeping_tasks_list = make_list_node(sleeping_tasks_list);
volatile int runnable_tasks_count = 0;


task_info *idle_task = NULL;
volatile u64 idle_ticks = 0;

void idle_task_kthread()
{
   while (true) {

      idle_ticks++;
      halt();

      if (!(idle_ticks % TIMER_HZ)) {
         DEBUG_printk("[idle task] ticks: %llu\n", idle_ticks);
      }

      if (runnable_tasks_count > 0) {
         DEBUG_printk("[idle task] runnable > 0, yield!\n");
         kernel_yield();
      }
   }
}

void initialize_scheduler(void)
{
   idle_task = kthread_create(&idle_task_kthread, NULL);
}

bool is_kernel_thread(task_info *ti)
{
   return ti->owning_process_pid == 0;
}

void set_current_task_in_kernel()
{
   ASSERT(!is_preemption_enabled());
   current->running_in_kernel = 1;
}


void task_add_to_state_list(task_info *ti)
{
   switch (ti->state) {

   case TASK_STATE_RUNNABLE:
      list_add_tail(&runnable_tasks_list, &ti->runnable_list);
      runnable_tasks_count++;
      break;

   case TASK_STATE_SLEEPING:
      list_add_tail(&sleeping_tasks_list, &ti->sleeping_list);
      break;

   case TASK_STATE_RUNNING:
   case TASK_STATE_ZOMBIE:
      break;

   default:
      NOT_REACHED();
   }
}

void task_remove_from_state_list(task_info *ti)
{
   switch (ti->state) {

   case TASK_STATE_RUNNABLE:
      list_remove(&ti->runnable_list);
      runnable_tasks_count--;
      ASSERT(runnable_tasks_count >= 0);
      break;

   case TASK_STATE_SLEEPING:
      list_remove(&ti->sleeping_list);
      break;

   case TASK_STATE_RUNNING:
   case TASK_STATE_ZOMBIE:
      break;

   default:
      NOT_REACHED();
   }
}

void task_change_state(task_info *ti, task_state_enum new_state)
{
   disable_preemption();
   {
      ASSERT(ti->state != new_state);
      task_remove_from_state_list(ti);

      ti->state = new_state;

      task_add_to_state_list(ti);
   }
   enable_preemption();
}

void add_task(task_info *ti)
{
   ASSERT(!is_preemption_enabled());
   disable_preemption();
   {
      list_add_tail(&tasks_list, &ti->list);
      task_add_to_state_list(ti);
   }
   enable_preemption();
}

void remove_task(task_info *ti)
{
   disable_preemption();
   {
      ASSERT(ti->state == TASK_STATE_ZOMBIE);
      DEBUG_printk("[remove_task] pid = %i\n", ti->pid);

      task_remove_from_state_list(ti);
      list_remove(&ti->list);

      kfree(ti->kernel_stack, KTHREAD_STACK_SIZE);
      kfree(ti, sizeof(task_info));
   }
   enable_preemption();
}



void account_ticks()
{
   if (!current) {
      return;
   }

   current->ticks++;
   current->total_ticks++;

   if (current->running_in_kernel) {
      current->kernel_ticks++;
   }
}

bool need_reschedule()
{
   if (!current) {
      // The kernel is still initializing and we cannot call schedule() yet.
      return false;
   }

   if (current->ticks < TIME_SLOT_JIFFIES &&
       current->state == TASK_STATE_RUNNING) {
      return false;
   }

   DEBUG_printk("\n\n[sched] Current pid: %i, "
                "used %llu ticks (user: %llu, kernel: %llu) [runnable: %i]\n",
                current->pid, current->ticks,
                current->total_ticks - current->kernel_ticks,
                current->kernel_ticks, runnable_tasks_count);

   return true;
}

void schedule_outside_interrupt_context()
{
   // HACK: push a fake interrupt to compensate the call to
   // pop_nested_interrupt() in switch_to_process().

   push_nested_interrupt(-1);
   schedule();
}

NORETURN void switch_to_idle_task_outside_interrupt_context()
{
   // HACK: push a fake interrupt to compensate the call to
   // pop_nested_interrupt() in switch_to_process().

   push_nested_interrupt(-1);
   switch_to_task(idle_task);
}


void schedule()
{
   task_info *selected = NULL;
   task_info *pos;
   u64 least_ticks_for_task = (u64)-1;

   ASSERT(!is_preemption_enabled());

   // If we preempted the process, it is still runnable.
   if (current->state == TASK_STATE_RUNNING) {
      task_change_state(current, TASK_STATE_RUNNABLE);
   }

   list_for_each(pos, &runnable_tasks_list, runnable_list) {

      DEBUG_printk("   [sched] checking pid %i (ticks = %llu): ",
                   pos->pid, pos->total_ticks);

      ASSERT(pos->state == TASK_STATE_RUNNABLE);

      if (pos == idle_task) {
         DEBUG_printk("SKIP\n");
         continue;
      }

      if (pos->total_ticks < least_ticks_for_task) {
         DEBUG_printk("GOOD\n");
         selected = pos;
         least_ticks_for_task = pos->total_ticks;
      } else {
         DEBUG_printk("BAD\n");
      }
   }

   if (!selected) {
      selected = idle_task;
      DEBUG_printk("[sched] Selected 'idle'\n");
   }

   if (selected == current) {
      task_change_state(selected, TASK_STATE_RUNNING);
      selected->ticks = 0;
      return;
   }

   switch_to_task(selected);
}

// TODO: make this function much faster (e.g. indexing by pid)
task_info *get_task(int pid)
{
   task_info *pos;
   task_info *res = NULL;

   disable_preemption();

   list_for_each(pos, &tasks_list, list) {
      if (pos->pid == pid) {
         res = pos;
         break;
      }
   }

   enable_preemption();
   return res;
}
