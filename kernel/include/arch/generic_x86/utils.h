
#pragma once

#include <common_defs.h>

#if !defined(__i386__) && !defined(__x86_64__)
#error This header can be used only for x86 and x86-64 architectures.
#endif

#define TIMER_HZ 100

static ALWAYS_INLINE u64 RDTSC()
{
#ifdef BITS64
   uptr lo, hi;
   asm("rdtsc" : "=a" (lo), "=d" (hi));
   return lo | (hi << 32);
#else
   u64 val;
   asm("rdtsc" : "=A" (val));
   return val;
#endif
}

static ALWAYS_INLINE void outb(u16 port, u8 val)
{
   /*
    * There's an outb %al, $imm8  encoding, for compile-time constant port
    * numbers that fit in 8b.  (N constraint). Wider immediate constants
    * would be truncated at assemble-time (e.g. "i" constraint).
    * The  outb  %al, %dx  encoding is the only option for all other cases.
    * %1 expands to %dx because  port  is a u16.  %w1 could be used if we had
    * the port number a wider C type.
    */
   asmVolatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static ALWAYS_INLINE u8 inb(u16 port)
{
   u8 ret_val;
   asmVolatile("inb %[port], %[result]"
      : [result] "=a"(ret_val)   // using symbolic operand names
      : [port] "Nd"(port));
   return ret_val;
}

static ALWAYS_INLINE void halt()
{
   asmVolatile("hlt");
}

static ALWAYS_INLINE void cli()
{
   asmVolatile("cli");
}

static ALWAYS_INLINE void sti()
{
   asmVolatile("sti");
}

static ALWAYS_INLINE void wrmsr(u32 msr_id, u64 msr_value)
{
    asmVolatile( "wrmsr" : : "c" (msr_id), "A" (msr_value) );
}

static ALWAYS_INLINE u64 rdmsr(u32 msr_id)
{
    u64 msr_value;
    asmVolatile( "rdmsr" : "=A" (msr_value) : "c" (msr_id) );
    return msr_value;
}

static ALWAYS_INLINE bool are_interrupts_enabled()
{
    uptr flags;
    asmVolatile( "pushf\n\t"
                 "pop %0"
                 : "=g"(flags) );
    return flags & (1 << 9);
}

static ALWAYS_INLINE void cpuid(int code, u32 *a, u32 *d)
{
    asmVolatile( "cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "ebx", "ecx" );
}
