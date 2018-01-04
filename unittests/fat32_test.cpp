
#include <cstdio>
#include <iostream>
#include <memory>
#include <map>

using namespace std;

#include <gtest/gtest.h>

#include "kernel_init_funcs.h"


extern "C" {
   #include <fs/fat32.h>
   #include <fs/exvfs.h>
   #include <utils.h>
}

const char *load_once_file(const char *filepath, size_t *fsize = nullptr)
{
   static map<const char *,
              pair<unique_ptr<const char[]>, size_t>> files_loaded;

   auto it = files_loaded.find(filepath);

   if (it == files_loaded.end()) {

      long file_size;
      char *buf;
      FILE *fp;

      fp = fopen(filepath, "rb");
      assert(fp != nullptr);

      fseek(fp, 0, SEEK_END);
      file_size = ftell(fp);

      buf = new char [file_size];
      assert(buf != nullptr);

      fseek(fp, 0, SEEK_SET);
      size_t bytes_read = fread(buf, 1, file_size, fp);
      assert(bytes_read == file_size);

      fclose(fp);

      auto &e = files_loaded[filepath];
      e.first.reset(buf);
      e.second = file_size;
   }

   auto &e = files_loaded[filepath];

   if (fsize)
      *fsize = e.second;

   return e.first.get();
}

TEST(fat32, dumpinfo)
{
   const char *buf = load_once_file("build/fatpart");
   fat_dump_info((void *) buf);

   fat_header *hdr = (fat_header*)buf;
   fat_entry *e = fat_search_entry(hdr, fat_unknown, "/nonesistentfile");
   ASSERT_TRUE(e == NULL);
}

TEST(fat32, read_content_of_shortname_file)
{
   const char *buf = load_once_file("build/fatpart");
   char data[128] = {0};
   fat_header *hdr;
   fat_entry *e;
   size_t res;
   FILE *fp;

   hdr = (fat_header*)buf;
   e = fat_search_entry(hdr, fat_unknown, "/testdir/dir1/f1");
   ASSERT_TRUE(e != NULL);

   fat_read_whole_file(hdr, e, data, sizeof(data));

   ASSERT_STREQ("hello world!\n", data);
}

TEST(fat32, read_content_of_longname_file)
{
   const char *buf = load_once_file("build/fatpart");
   char data[128] = {0};
   fat_header *hdr;
   fat_entry *e;
   size_t res;
   FILE *fp;

   hdr = (fat_header*)buf;
   e = fat_search_entry(hdr,
                        fat_unknown,
                        "/testdir/This_is_a_file_with_a_veeeery_long_name.txt");
   ASSERT_TRUE(e != NULL);
   fat_read_whole_file(hdr, e, data, sizeof(data));

   ASSERT_STREQ("Content of file with a long name\n", data);
}


TEST(fat32, read_whole_file)
{
   fat_header *hdr = (fat_header*)load_once_file("build/fatpart");
   fat_entry *e = fat_search_entry(hdr, fat_unknown, "/sbin/init");

   char *content = (char*)calloc(1, e->DIR_FileSize);
   fat_read_whole_file(hdr, e, content, e->DIR_FileSize);
   uint32_t fat_crc = crc32(0, content, e->DIR_FileSize);
   free(content);

   size_t fsize;
   const char *buf = load_once_file("build/init", &fsize);
   uint32_t actual_file_crc = crc32(0, buf, fsize);
   ASSERT_EQ(fat_crc, actual_file_crc);
}

TEST(fat32, fread)
{
   initialize_kmalloc_for_tests();

   filesystem *fs = fat_mount_ramdisk((void *) load_once_file("build/fatpart"));

   fs_int_handle_t h = fs->fopen(fs, "/sbin/init");
   ASSERT_TRUE(h != NULL);

   ssize_t buf2_size = 100*1000;
   char *buf2 = (char *) calloc(1, buf2_size);
   ssize_t read_offset = 0;

   FILE *fp = fopen("build/init", "rb");
   char tmpbuf[1024];

   while (true) {

      ssize_t read_block_size = 1023; /* 1023 is a prime number */
      ssize_t bytes_read = fs->fread(fs, h, buf2+read_offset, read_block_size);
      ssize_t bytes_read_real_file = fread(tmpbuf, 1, read_block_size, fp);

      ASSERT_EQ(bytes_read_real_file, bytes_read);

      for (int j = 0; j < bytes_read; j++) {
         if (buf2[read_offset+j] != tmpbuf[j]) {

            printf("Byte #%li differs:\n", read_offset+j);

            printf("buf2: ");
            for (int k = 0; k < 16; k++)
               printf("%x ", buf2[read_offset+j+k]);
            printf("\n");
            printf("tmpbuf: ");
            for (int k = 0; k < 16; k++)
               printf("%x ", tmpbuf[j+k]);
            printf("\n");

            FAIL();
         }
      }

      read_offset += bytes_read;

      if (bytes_read < read_block_size)
         break;
   }

   fclose(fp);
   fs->fclose(fs, h);

   fat_umount_ramdisk(fs);
   free(buf2);
}
