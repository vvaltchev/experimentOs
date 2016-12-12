
#pragma once

#include <process.h>
#include <arch/i386/arch_utils.h>



struct process_info {

   process_info *next;
   process_info *prev;

   int pid;
   int state;

   regs state_regs;
   page_directory_t *pdir;
};
