
#include <elf.h>
#include <paging.h>

#include <string_util.h>
#include <arch/generic_x86/utils.h>

#ifdef DEBUG

//
// Debug functions
//

void dump_elf32_header(Elf32_Ehdr *h)
{
   printk("Magic: ");
   for (int i = 0; i < EI_NIDENT; i++) {
      printk("%x ", h->e_ident[i]);
   }

   printk("\n");
   printk("Type: %p\n", h->e_type);
   printk("Machine: %p\n", h->e_machine);
   printk("Entry point: %p\n", h->e_entry);
   printk("ELF header size: %d\n", h->e_ehsize);
   printk("Program header entry size: %d\n", h->e_phentsize);
   printk("Program header num entries: %d\n", h->e_phnum);
   printk("Section header entry size: %d\n", h->e_shentsize);
   printk("Section header num entires: %d\n", h->e_shnum);
   printk("Section header string table index: %d\n\n", h->e_shstrndx);
}

void dump_elf32_program_segment_header(Elf32_Phdr *ph)
{
   printk("Segment type: %d\n", ph->p_type);
   printk("Segment offset in file: %d\n", ph->p_offset);
   printk("Segment vaddr: %p\n", ph->p_vaddr);
   printk("Segment paddr: %p\n", ph->p_paddr);
   printk("Segment size in file: %d\n", ph->p_filesz);
   printk("Segment size in memory: %d\n", ph->p_memsz);
   printk("Segment flags: %u\n", ph->p_flags);
   printk("Segment alignment: %d\n", ph->p_align);
}

void dump_elf32_phdrs(Elf32_Ehdr *h)
{
   Elf32_Phdr *phdr = (Elf32_Phdr *) ((char *)h + sizeof(*h));

   for (int i = 0; i < h->e_phnum; i++, phdr++) {
      printk("*** SEGMENT %i ***\n", i);
      dump_elf32_program_segment_header(phdr);
      printk("\n\n");
   }
}

#endif

//////////////////////////////////////////////////////////////////////////////

uptr alloc_pageframe();

void load_elf_program(void *elf,
                      page_directory_t *pdir,
                      void **entry,
                      void **stack_addr)
{

   Elf32_Ehdr *header = (Elf32_Ehdr *)elf;
   ASSERT(header->e_ehsize == sizeof(*header));

   //dump_elf32_header(header);
   //dump_elf32_phdrs(header);

   Elf32_Phdr *phdr = (Elf32_Phdr *) ((char *)header + sizeof(*header));

   for (int i = 0; i < header->e_phnum; i++, phdr++) {

      // Ignore non-load segments.
      if (phdr->p_type != PT_LOAD) {
         continue;
      }

      // Support only page-aligned segments.
      ASSERT(phdr->p_align == PAGE_SIZE);

      ASSERT(phdr->p_memsz >= phdr->p_filesz);

      int pages_count = ((phdr->p_memsz + PAGE_SIZE) & ~OFFSET_IN_PAGE_MASK) >> PAGE_SHIFT;

      printk("[ELF LOADER] Segment %i\n", i);
      printk("[ELF LOADER] Size: %i\n", phdr->p_memsz);
      printk("[ELF LOADER] Vaddr: %p\n", phdr->p_vaddr);
      printk("[ELF LOADER] Pages count: %i\n", pages_count);

      char *vaddr = (char *) phdr->p_vaddr;

      for (int j = 0; j < pages_count; j++, vaddr += PAGE_SIZE) {
         map_page(pdir, vaddr, alloc_pageframe(), true, true);
         memset(vaddr, 0, PAGE_SIZE);
         memmove(vaddr,
                 (char *)elf + phdr->p_offset + j * PAGE_SIZE,
                 PAGE_SIZE);
      }
   }

   // Allocating memory for the user stack.

   const int pages_for_stack = 16;
   void *stack_top = (void *) (0xC0000000UL - pages_for_stack * PAGE_SIZE);

   for (int i = 0; i < pages_for_stack; i++) {
      map_page(pdir, stack_top + i * PAGE_SIZE, alloc_pageframe(), true, true);
   }

   // Finally setting the output-params.

   *stack_addr = (void *) ((0xC0000000UL - 1) & ~15);
   *entry = (void *) header->e_entry;
}
