
#include <common_defs.h>

#include <arch/i386/arch_utils.h>
#include <string_util.h>
#include <term.h>

/* Defines an IDT entry */
struct idt_entry
{
    u16 base_lo;
    u16 sel;
    u8 always0;
    u8 flags;
    u16 base_hi;
} __attribute__((packed));

struct idt_ptr
{
    u16 limit;
    void *base;
} __attribute__((packed));

/*
 * Declare an IDT of 256 entries. Although we will only use the
 * first 32 entries in this tutorial, the rest exists as a bit
 * of a trap. If any undefined IDT entry is hit, it normally
 * will cause an "Unhandled Interrupt" exception. Any descriptor
 * for which the 'presence' bit is cleared (0) will generate an
 * "Unhandled Interrupt" exception
 */
struct idt_entry idt[256] = {0};
struct idt_ptr idtp = {0};

/* This exists in 'start.asm', and is used to load our IDT */
void idt_load();

/*
 * Use this function to set an entry in the IDT.
 */
void idt_set_gate(u8 num, void *handler, u16 sel, u8 flags)
{
	const u32 base = (u32)handler;

    /* The interrupt routine's base address */
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;

    /* The segment or 'selector' that this IDT entry will use
    *  is set here, along with any access flags */
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/*
 * These are function prototypes for all of the exception
 * handlers: The first 32 entries in the IDT are reserved
 * by Intel, and are designed to service exceptions!
 */

void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

// This is used for int 0x80 (syscallls)
void isr128();

/*
 * We set the access flags to 0x8E. This means that the entry is
 * present, is running in ring 0 (kernel level), and has the lower 5 bits
 * set to the required '14', which is represented by 'E' in hex.
 */

void isrs_install()
{
   idt_set_gate(0, isr0, 0x08, 0x8E);
   idt_set_gate(1, isr1, 0x08, 0x8E);
   idt_set_gate(2, isr2, 0x08, 0x8E);
   idt_set_gate(3, isr3, 0x08, 0x8E);
   idt_set_gate(4, isr4, 0x08, 0x8E);
   idt_set_gate(5, isr5, 0x08, 0x8E);
   idt_set_gate(6, isr6, 0x08, 0x8E);
   idt_set_gate(7, isr7, 0x08, 0x8E);

   idt_set_gate(8, isr8, 0x08, 0x8E);
   idt_set_gate(9, isr9, 0x08, 0x8E);
   idt_set_gate(10, isr10, 0x08, 0x8E);
   idt_set_gate(11, isr11, 0x08, 0x8E);
   idt_set_gate(12, isr12, 0x08, 0x8E);
   idt_set_gate(13, isr13, 0x08, 0x8E);
   idt_set_gate(14, isr14, 0x08, 0x8E);
   idt_set_gate(15, isr15, 0x08, 0x8E);

   idt_set_gate(16, isr16, 0x08, 0x8E);
   idt_set_gate(17, isr17, 0x08, 0x8E);
   idt_set_gate(18, isr18, 0x08, 0x8E);
   idt_set_gate(19, isr19, 0x08, 0x8E);
   idt_set_gate(20, isr20, 0x08, 0x8E);
   idt_set_gate(21, isr21, 0x08, 0x8E);
   idt_set_gate(22, isr22, 0x08, 0x8E);
   idt_set_gate(23, isr23, 0x08, 0x8E);

   idt_set_gate(24, isr24, 0x08, 0x8E);
   idt_set_gate(25, isr25, 0x08, 0x8E);
   idt_set_gate(26, isr26, 0x08, 0x8E);
   idt_set_gate(27, isr27, 0x08, 0x8E);
   idt_set_gate(28, isr28, 0x08, 0x8E);
   idt_set_gate(29, isr29, 0x08, 0x8E);
   idt_set_gate(30, isr30, 0x08, 0x8E);
   idt_set_gate(31, isr31, 0x08, 0x8E);

   // Syscall with int 0x80.

   // Note: flags is 0xEE, in order to allow this interrupt
   // to be used from ring 3.

   // Flags:
   // P | DPL | Always 01110 (14)
   // P = Segment is present, 1 = Yes
   // DPL = Ring
   //
   idt_set_gate(0x80, isr128, 0x08, 0xEE);
}

/* This is a simple string array. It contains the message that
*  corresponds to each and every exception. We get the correct
*  message by accessing like:
*  exception_message[interrupt_number] */
char *exception_messages[] =
{
   "Division By Zero",
   "Debug",
   "Non Maskable Interrupt",
   "Breakpoint",
   "Into Detected Overflow",
   "Out of Bounds",
   "Invalid Opcode",
   "No Coprocessor",

   "Double Fault",
   "Coprocessor Segment Overrun",
   "Bad TSS",
   "Segment Not Present",
   "Stack Fault",
   "General Protection Fault",
   "Page Fault",
   "Unknown Interrupt",

   "Coprocessor Fault",
   "Alignment Check",
   "Machine Check",
   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved",

   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved",
   "Reserved"
};


interrupt_handler fault_handlers[32] = { NULL };

volatile int nested_interrupts_count = 0;
volatile int nested_interrupts[32] = { [0 ... 31] = -1 };

extern interrupt_handler irq_routines[16];
void handle_syscall(regs *);


void set_fault_handler(int exceptionNum, void *ptr)
{
   fault_handlers[exceptionNum] = (interrupt_handler) ptr;
}

extern task_info *current_process;

void end_current_interrupt_handling()
{
   int curr_int = get_curr_interrupt();

   if (is_irq(curr_int)) {
      PIC_sendEOI(curr_int - 32);
   }

   if (LIKELY(current_process != NULL)) {

      nested_interrupts_count--;
      ASSERT(nested_interrupts_count >= 0);

   } else if (nested_interrupts_count > 0) {

      nested_interrupts_count--;
   }
}

static void handle_irq(regs *r)
{
   const u8 irq = r->int_no - 32;

   if (irq_routines[irq] != NULL) {

      irq_routines[irq](r);

   } else {

      if (irq == 7) return;

      printk("Unhandled IRQ #%i\n", irq);
   }
}

static void handle_fault(regs *r)
{
   if (fault_handlers[r->int_no] != NULL) {

      fault_handlers[r->int_no](r);

   } else {

      cli();

      printk("Fault #%i: %s [errCode: %i]\n",
             r->int_no,
             exception_messages[r->int_no],
             r->err_code);

      NOT_REACHED();
   }
}


void generic_interrupt_handler(regs *r)
{
   if (nested_interrupts_count >= (int)ARRAY_SIZE(nested_interrupts)) {
      NOT_REACHED();
   }

   nested_interrupts[nested_interrupts_count++] = r->int_no;

   if (LIKELY(r->int_no == SYSCALL_SOFT_INTERRUPT)) {

      handle_syscall(r);

   } else if (LIKELY(r->int_no >= 32)) {

      handle_irq(r);

   } else {

      handle_fault(r);
   }

   end_current_interrupt_handling();
}



/* Installs the IDT */
void idt_install()
{
    /* Sets the special IDT pointer up, just like in 'gdt.c' */
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
    idtp.base = &idt;

    /* Clear out the entire IDT, initializing it to zeros */
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    /* Add any new ISRs to the IDT here using idt_set_gate */

    isrs_install();

    /* Points the processor's internal register to the new IDT */
    idt_load();
}
