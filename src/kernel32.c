

#include <commonDefs.h>
#include <stringUtil.h>
#include <term.h>
#include <irq.h>
#include <kmalloc.h>
#include <paging.h>
#include <debug_utils.h>

void gdt_install();
void idt_install();

void timer_phase(int hz)
{
   int divisor = 1193180 / hz;   /* Calculate our divisor */
   outb(0x43, 0x36);             /* Set our command byte 0x36 */
   outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
   outb(0x40, divisor >> 8);     /* Set high byte of divisor */
}


/* This will keep track of how many ticks that the system
*  has been running for */
volatile unsigned timer_ticks = 0;

#define CLOCK_HZ 10

/* Handles the timer. In this case, it's very simple: We
*  increment the 'timer_ticks' variable every time the
*  timer fires. By default, the timer fires 18.222 times
*  per second. Why 18.222Hz? Some engineer at IBM must've
*  been smoking something funky */


void timer_handler()
{
   unsigned val = ++timer_ticks;

   if ((val % CLOCK_HZ) == 0) {
      printk("Ticks: %u\n", timer_ticks);
   }
}

void init_kb();
void keyboard_handler(struct regs *r);
void set_kernel_stack(uint32_t stack);
void switch_to_usermode_asm(void *entryPoint, void *stackAddr);

void switch_to_user_mode()
{
   void * const user_program_vaddr = (void *) 0x08000000U;
   void * const user_program_stack_vaddr = (void *) 0x0800FFF0U;
   void * const user_program_paddr = (void *) 0x120000;

   // Set up our kernel stack.
   set_kernel_stack(0xC01FFFF0);

   // maps 16 pages (64 KB) for the user program

   map_pages(get_curr_page_dir(),
             (uint32_t)user_program_vaddr,
             (uint32_t)user_program_paddr, 16, true, true);

   printk("pdir entries used = %i\n", debug_count_used_pdir_entries(get_curr_page_dir()));
   debug_dump_used_pdir_entries(get_curr_page_dir());

   switch_to_usermode_asm(user_program_vaddr, user_program_stack_vaddr);
}


void show_hello_message()
{
   printk("Hello from my kernel!\n");
}


void kmain() {

   term_init();
   show_hello_message();

   gdt_install();
   idt_install();
   irq_install();

   init_physical_page_allocator();
   init_paging();


   timer_phase(CLOCK_HZ);

   irq_install_handler(0, timer_handler);
   irq_install_handler(1, keyboard_handler);

   IRQ_set_mask(0); // mask the timer interrupt.

   sti();
   init_kb();

   switch_to_user_mode();

   while (1) {
      halt();
   }
}
