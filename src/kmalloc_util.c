
#include <paging.h>
#include <stringUtil.h>

bool kbasic_virtual_alloc(uptr vaddr, int pageCount)
{
   ASSERT(!(vaddr & (PAGE_SIZE - 1))); // the vaddr must be page-aligned

   page_directory_t *pdir = get_kernel_page_dir();

   // Ensure that we have enough physical memory.
   if (get_free_physical_pages_count() < pageCount) {
      return false;
   }

   for (int i = 0; i < pageCount; i++) {

      void *paddr = alloc_phys_page();
      ASSERT(paddr != NULL);

      ASSERT(!is_mapped(pdir, vaddr + (i << PAGE_SHIFT)));
      map_page(pdir, vaddr + (i << PAGE_SHIFT), (uptr)paddr, false, true);
   }

   return true;
}

bool kbasic_virtual_free(uptr vaddr, int pageCount)
{
   ASSERT(!(vaddr & (PAGE_SIZE - 1))); // the vaddr must be page-aligned

   page_directory_t *pdir = get_kernel_page_dir();

   for (int i = 0; i < pageCount; i++) {

      uptr va = vaddr + (i << PAGE_SHIFT);

      // get_mapping ASSERTs that 'va' is mapped.
      void *paddr = get_mapping(pdir, va);

      // un-map the virtual address.
      unmap_page(pdir, va);

      // free the physical page as well.
      free_phys_page(paddr);
   }

   return true;
}

