#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <random>

extern "C" {
#include <kmalloc.h>
#include <paging.h>
void kmalloc_trivial_perf_test();
void kmalloc_perf_test();
}

using namespace std;

extern "C" {


void *kernel_heap_base = nullptr;


unordered_map<uintptr_t, uintptr_t> mappings;

bool __wrap_is_mapped(void *pdir, uintptr_t vaddr)
{
   return mappings[vaddr & ~(PAGE_SIZE - 1)] != 0;
}

bool __wrap_kbasic_virtual_alloc(uintptr_t vaddr, int pageCount)
{
   assert((vaddr & (PAGE_SIZE - 1)) == 0);

   for (int i = 0; i < pageCount; i++) {
      void *p = alloc_phys_page();
      mappings[vaddr + i * PAGE_SIZE] = (uintptr_t)p;
   }

   return true;
}

bool __wrap_kbasic_virtual_free(uintptr_t vaddr, int pageCount)
{
   assert((vaddr & (PAGE_SIZE - 1)) == 0);

   for (int i = 0; i < pageCount; i++) {
      
      uintptr_t phys_addr = mappings[vaddr + i * PAGE_SIZE];
      free_phys_page((void *)phys_addr);
      mappings[vaddr + i * PAGE_SIZE] = 0;
   }

   return true;
}

}

void init_test_kmalloc()
{
   uintptr_t align_size = 16 * PAGE_SIZE;

   uintptr_t addr = (uintptr_t)malloc(HEAP_DATA_SIZE + align_size);
   addr += align_size;
   addr &= ~(align_size - 1);

   kernel_heap_base = (void *)addr;
}


void kmalloc_chaos_test_sub(default_random_engine &e, lognormal_distribution<> &dist)
{
   size_t mem_allocated = 0;
   vector<pair<void *, size_t>> allocations;

   for (int i = 0; i < 1000; i++) {

      size_t orig_s = round(dist(e));
      size_t s = roundup_next_power_of_2(MAX(orig_s, MIN_BLOCK_SIZE));

      //printf("[Test] Allocating %u bytes..\n", s);

      void *r = kmalloc(s);

      if (!r) {
         //printf("**** Unable to allocate %u bytes (allocated by now: %u)\n", s, mem_allocated);
         continue;
      }

      mem_allocated += s;
      allocations.push_back(make_pair(r, s));
   }

   for (const auto& e : allocations) {
      //printf("[Test] Free ptr at %p (size: %u)\n", e.first, e.second);
      kfree(e.first, e.second);
      mem_allocated -= e.second;
   }

   // Now, re-allocate all the chunks and expect to get the same results.

   for (const auto& e : allocations) {

      //printf("Re-allocating block of size %u\n", e.second);
      void *ptr = kmalloc(e.second);

      if (ptr != e.first) {
         printf("TEST FAILED\n");
         printf("ptr expected: %p\n", e.first);
         printf("ptr got:      %p\n", ptr);
         break;
      }
   }

   for (const auto& e : allocations) {
      kfree(e.first, e.second);
      mem_allocated -= e.second;
   }
}

void kmalloc_chaos_test()
{
   random_device rdev;
   default_random_engine e(rdev());

   lognormal_distribution<> dist(5.0, 3);

   for (int i = 0; i < 100; i++) {
      kmalloc_chaos_test_sub(e, dist);
   }
}



int main(int argc, char **argv) {

   init_test_kmalloc();
   init_physical_page_allocator();
   initialize_kmalloc();

   kmalloc_chaos_test();

   //kmalloc_trivial_perf_test();
   kmalloc_perf_test();

   return 0;
}
