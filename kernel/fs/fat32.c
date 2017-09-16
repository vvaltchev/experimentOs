
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

void fat_dump_common_header(void *data)
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

   s8 sfname[11];

   u8 readonly : 1; // lower-bit
   u8 hidden : 1;
   u8 system : 1;
   u8 volume_id : 1;
   u8 directory : 1;
   u8 archive : 1;
   u8 unused1 : 2;  // higher 2 bits

   u8 DIR_NTRes;
   u8 DIR_CrtTimeTenth;

   u8 unused2[6];

   u16 DIR_FstClusHI;

   u16 DIR_WrtTime;
   u16 DIR_WrtDate;

   u16 DIR_FstClusLO;
   u32 DIR_FileSize;

} fat_dir_entry;

int dump_directory(fat_bpb *hdr, fat_dir_entry *entry, int level);

int get_sector_for_cluster(fat_bpb *hdr, int clusterNum)
{
   int RootDirSectors = ((hdr->BPB_RootEntCnt * 32) + (hdr->BPB_BytsPerSec - 1)) / hdr->BPB_BytsPerSec;
   int FirstDataSector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz16) + RootDirSectors;

   int FirstSectorofCluster = ((clusterNum - 2) * hdr->BPB_SecPerClus) + FirstDataSector;

   return FirstSectorofCluster;
}

int dump_dir_entry(fat_bpb *hdr, fat_dir_entry *entry, int level)
{
   if (entry->volume_id) {
      return 0; // the first "file" is the volume ID. Skip it.
   }

   char *fname = entry->sfname;

   if (fname[0] == 0) {
      //printk("dir end\n");
      return -1;
   }

   if (fname[0] == (char)0xE5) {
      //printk("invalid fname\n");
      return -1;
   }

   // TODO: hack!! fix it
   if (fname[0] == '.')
      return 0;

   dump_fixed_str("short fname", (char*)entry->sfname, sizeof(entry->sfname));

   // printk("readonly:  %u\n", entry->readonly);
   // printk("hidden:    %u\n", entry->hidden);
   // printk("system:    %u\n", entry->system);
   // printk("vol id:    %u\n", entry->volume_id);
   //printk("directory: %u\n", entry->directory);

   int first_cluster = entry->DIR_FstClusHI << 16 | entry->DIR_FstClusLO;

   if (entry->directory) {
      int sec = get_sector_for_cluster(hdr, first_cluster);
      fat_dir_entry *e = (fat_dir_entry*)((u8*)hdr + sec * hdr->BPB_BytsPerSec);
      dump_directory(hdr, e, level);
      return 0;
   }

   printk("file size: %u\n", entry->DIR_FileSize);
   //printk("first cluster: %u\n", first_cluster);
   printk("\n");
   return 0;
}

int dump_directory(fat_bpb *hdr, fat_dir_entry *entry, int level)
{
   int ret;
   do {
      ret = dump_dir_entry(hdr, entry, level+1);
      entry++;
   } while (ret == 0);
   return 0;
}

void fat32_dump_info(void *fatpart_begin)
{
   fat_bpb *hdr = fatpart_begin;
   fat_dump_common_header(fatpart_begin);

   printk("\n");

   void *root_dir = NULL;

   if (hdr->BPB_TotSec16 != 0) {
      dump_fat16_headers(fatpart_begin);
   } else {
      printk("FAT32 not supported yet.\n");
   }
   printk("\n");

   int clusters_first_sec = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz16);

   printk("clusters first sec: %i\n", clusters_first_sec);
   root_dir = (void*) ((u8*)hdr + (hdr->BPB_BytsPerSec * clusters_first_sec));
   dump_directory(hdr, root_dir, 0);
}


