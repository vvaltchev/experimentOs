
/* Usermode application */

#include <common_defs.h>
#include "usermode_syscall_wrappers.h"
#include "string.h"

int bss_variable[32];

void main()
{
   int stackVar;

   printf("Hello from init!\n");
   printf("&stackVar = %p\n", &stackVar);

   for (int i = 0; i < 4; i++) {
      printf("BssVar[%i] = %i\n", i, bss_variable[i]);
   }

   int ret = open("/myfile.txt", 0xAABB, 0x112233);

   printf("ret = %i\n", ret);

   for (int i = 0; i < 5; i++) {
      printf("i = %i\n", i);
   }

   printf("Running infinite loop..\n");

   unsigned n = 1;
   int billions = 0;
   bool inchild = false;

   while (true) {

      if (!(n % (1024 * 1024 * 1024))) {

         printf("[PID: %i] 1 billion iters\n", getpid());
         billions++;

         if (billions == 1) {

            printf("forking..\n");

            int pid = fork();

            printf("Fork returned %i\n", pid);

            if (pid == 0) {
               printf("############## I'm the child!\n");
               inchild = true;
            } else {
               printf("############## I'm the parent, child's pid = %i\n", pid);
            }

         }

         if (billions == 2 && inchild) {
            printf("child: 2 billion, exit!\n");
            _exit(123);
         }
      }

      n++;
   }
}

