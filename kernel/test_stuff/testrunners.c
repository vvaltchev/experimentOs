
#include <common_defs.h>
#include <string_util.h>
#include <term.h>
#include <irq.h>
#include <kmalloc.h>
#include <paging.h>
#include <debug_utils.h>
#include <process.h>

#include <hal.h>
#include <utils.h>
#include <tasklet.h>
#include <sync.h>

#define INIT_PROGRAM_MEM_DISK_OFFSET 0x00023600


void simple_test_kthread(void)
{
   printk("[kernel thread] This is a kernel thread..\n");

   for (int i = 0; i < 1024*(int)MB; i++) {
      if (!(i % (256*MB))) {
         printk("[kernel thread] i = %i\n", i/MB);
      }
   }
}


void test_memdisk()
{
   char *ptr;

   printk("Data at %p:\n", 0x0);
   ptr = (char *)RAM_DISK_VADDR;
   for (int i = 0; i < 16; i++) {
      printk("%x ", (u8)ptr[i]);
   }
   printk("\n");

   printk("Data at %p:\n", INIT_PROGRAM_MEM_DISK_OFFSET);
   ptr = (char *)(RAM_DISK_VADDR + INIT_PROGRAM_MEM_DISK_OFFSET);
   for (int i = 0; i < 16; i++) {
      printk("%x ", (u8)ptr[i]);
   }
   printk("\n");



   printk("\n\n");
   printk("Calculating CRC32...\n");
   u32 crc = crc32(0, (void *)RAM_DISK_VADDR, RAM_DISK_SIZE);
   printk("Crc32 of the data: %p\n", crc);
}


static kmutex test_mutex = { 0 };

void test_kmutex_thread1(void)
{
   printk("1) before lock\n");

   kmutex_lock(&test_mutex);

   printk("1) under lock..\n");
   for (int i=0; i < 1024*1024*1024; i++) { }

   kmutex_unlock(&test_mutex);

   printk("1) after lock\n");
}

void test_kmutex_thread2(void)
{
   printk("2) before lock\n");

   kmutex_lock(&test_mutex);

   printk("2) under lock..\n");
   for (int i=0; i < 1024*1024*1024; i++) { }

   kmutex_unlock(&test_mutex);

   printk("2) after lock\n");
}

void test_kmutex_thread3(void)
{
   printk("3) before trylock\n");

   bool locked = kmutex_trylock(&test_mutex);

   if (locked) {

      printk("3) trylock SUCCEEDED: under lock..\n");

      if (locked) {
         kmutex_unlock(&test_mutex);
      }

      printk("3) after lock\n");

   } else {
      printk("trylock returned FALSE\n");
   }
}


void kmutex_test(void)
{
   kmutex_init(&test_mutex);
   current_task = kthread_create(test_kmutex_thread1);
   current_task = kthread_create(test_kmutex_thread2);
   current_task = kthread_create(test_kmutex_thread3);
}


static kcond cond = { 0 };
static kmutex cond_mutex = { 0 };

void kcond_thread_test1()
{
   kmutex_lock(&cond_mutex);

   printk("[thread1]: under lock, waiting for signal..\n");
   kcond_wait(&cond, &cond_mutex);

   printk("[thread1]: under lock, signal received..\n");

   kmutex_unlock(&cond_mutex);

   printk("[thread1]: exit\n");
}

void kcond_thread_test2()
{
   kmutex_lock(&cond_mutex);

   printk("[thread2]: under lock, waiting for signal..\n");
   kcond_wait(&cond, &cond_mutex);

   printk("[thread2]: under lock, signal received..\n");

   kmutex_unlock(&cond_mutex);

   printk("[thread2]: exit\n");
}


void kcond_thread_signal_generator()
{
   kmutex_lock(&cond_mutex);

   printk("[thread signal]: under lock, waiting some time..\n");
   for (int i=0; i < 1024*1024*1024; i++) { }

   printk("[thread signal]: under lock, signal_all!\n");

   kcond_signal_all(&cond);

   kmutex_unlock(&cond_mutex);

   printk("[thread signal]: exit\n");
}


void kcond_test(void)
{
   kmutex_init(&cond_mutex);
   kcond_init(&cond);

   current_task = kthread_create(&kcond_thread_test1);
   current_task = kthread_create(&kcond_thread_test2);
   current_task = kthread_create(&kcond_thread_signal_generator);
}
