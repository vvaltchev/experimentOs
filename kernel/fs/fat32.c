
#include <common_defs.h>
#include <string_util.h>

typedef struct __attribute__(( packed )) {

   u8 BS_jmpBoot[3];
   s8 BS_OEMName[8];
   u16 BPB_BytsPerSec;
   u8 BPB_SecPerClus;
   u16 BPB_RsvdSecCnt;
   u8 BPB_NumFATs;
   u16 BPB_RootEntCnt;
   u16 BPB_TotSec16;
   u8 BPB_Media;
   u16 BPB_FATSz16;
   u16 BPB_SecPerTrk;
   u16 BPB_NumHeads;
   u32 BPB_HiddSec;
   u32 BPB_TotSec32;

} fat_bpb;

static void dump_fixed_str(const char *what, char *str, u32 len)
{
   char buf[len+1];
   buf[len]=0;
   memcpy(buf, str, len);
   printk("%s: '%s'\n", what, buf);
}

void fat32_dump_common_header(void *data)
{
   fat_bpb *bpb = data;

   dump_fixed_str("EOM name", bpb->BS_OEMName, sizeof(bpb->BS_OEMName));
   printk("Bytes per sec: %u\n", bpb->BPB_BytsPerSec);
   printk("Sectors per cluster: %u\n", bpb->BPB_SecPerClus);
   printk("Reserved sectors count: %u\n", bpb->BPB_RsvdSecCnt);
   printk("Num FATs: %u\n", bpb->BPB_NumFATs);
   printk("Root ent count: %u\n", bpb->BPB_RootEntCnt);
   printk("Tot Sec 16: %u\n", bpb->BPB_TotSec16);
   printk("Media: %u\n", bpb->BPB_Media);
   printk("FATz16: %u\n", bpb->BPB_FATSz16);
   printk("Sectors per track: %u\n", bpb->BPB_SecPerTrk);
   printk("Num heads: %u\n", bpb->BPB_NumHeads);
   printk("Hidden sectors: %u\n", bpb->BPB_HiddSec);
   printk("Total Sec 32: %u\n", bpb->BPB_TotSec32);
}

typedef struct __attribute__(( packed )) {

   u8 BS_DrvNum;
   u8 BS_Reserved1;
   u8 BS_BootSig;
   u32 BS_VolID;
   s8 BS_VolLab[11];
   s8 BS_FilSysType[8];

} fat16_bpb;

static void dump_fat16_headers(void *data)
{
   fat16_bpb *hdr = (void*) ( (u8*)data + sizeof(fat_bpb) );

   printk("Drive num: %u\n", hdr->BS_DrvNum);
   printk("BootSig: %u\n", hdr->BS_BootSig);
   printk("VolID: %p\n", hdr->BS_VolID);
   dump_fixed_str("VolLab", hdr->BS_VolLab, sizeof(hdr->BS_VolLab));
   dump_fixed_str("FilSysType", hdr->BS_FilSysType, sizeof(hdr->BS_FilSysType));
}

typedef struct __attribute__(( packed )) {

   u8 short_filename[11];

   u8 unused1 : 1;
   u8 unused2 : 1;
   u8 archive : 1;
   u8 directory : 1;
   u8 volume_id : 1;
   u8 system : 1;
   u8 hidden : 1;
   u8 readonly : 1;

   u16 first_clust_hi;
   u16 first_clust_lo;
   u32 filesize;

} fat_file;


void fat32_dump_info(void *fatpart_begin)
{
   fat_bpb *hdr = fatpart_begin;
   fat32_dump_common_header(fatpart_begin);

   printk("\n");

   void *root_dir = NULL;

   if (hdr->BPB_TotSec16 != 0) {
      dump_fat16_headers(fatpart_begin);
   } else {
      printk("FAT32\n");
   }

   printk("\n");

   int clusters_first_sec = hdr->BPB_RsvdSecCnt + hdr->BPB_NumFATs * hdr->BPB_FATSz16;

   printk("clusters first sec: %i\n", clusters_first_sec);

   root_dir = (void*) ((u8*)hdr + hdr->BPB_BytsPerSec * clusters_first_sec);

   printk("root_dir offset: %i\n", (u8*)root_dir - (u8*)fatpart_begin);

   fat_file *file = root_dir;

   if (file->short_filename[0] != 0xE5)
      dump_fixed_str("short fname", (char*)file->short_filename, sizeof(file->short_filename));

   printk("file size: %u\n", file->filesize);
   printk("first clust: %u\n", file->first_clust_hi << 16 | file->first_clust_lo);
}


