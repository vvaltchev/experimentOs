
#pragma once

#include <commonDefs.h>
#include <paging.h>

// A page table entry
typedef struct {

	uint32_t present : 1;
	uint32_t rw : 1;        // read only = 0, read/write = 1
	uint32_t us : 1;        // user/supervisor
	uint32_t wt : 1;        // write-through
	uint32_t cd : 1;        // cache-disabled
	uint32_t accessed : 1;
	uint32_t dirty : 1;
	uint32_t zero : 1;
	uint32_t global : 1;
	uint32_t avail : 3;
	uint32_t pageAddr : 20; // the first 20 bits of the physical addr.

} page_t;


// A page table
typedef struct {

	page_t pages[1024];

} page_table_t;


// A page directory entry
typedef struct {

	uint32_t present : 1;
	uint32_t rw : 1;             // read only = 0, read/write = 1
	uint32_t us : 1;             // us = 0 -> supervisor only, 1 -> user also
	uint32_t wt : 1;             // write-through
	uint32_t cd : 1;             // cache-disabled
	uint32_t accessed : 1;
	uint32_t zero : 1;
	uint32_t psize : 1;          // page size; 0 = 4 KB, 1 = 4 MB
	uint32_t ignored : 1;
	uint32_t avail : 3;
	uint32_t pageTableAddr : 20; // aka, 'page_table_t *'

} page_dir_entry_t;


// A page directory
struct page_directory_t {

	page_dir_entry_t entries[1024];  // actual entries used by the CPU
	page_table_t *page_tables[1024]; // pointers to the tables (virtual addresses)
   uintptr_t paddr;                 // physical address of this page directory
};

STATIC_ASSERT(sizeof(page_directory_t) == PAGE_DIR_SIZE);