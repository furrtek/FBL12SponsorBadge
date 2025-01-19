// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include <stdio.h>
#include "main.h"
#include "fat_data.h"
#include "user_data.h"
#include "update.h"

static const char readme_contents[] = "Drop a valid .UF2 update file in the root directory to update your badge.\nCopiez un fichier .UF2 valide dans le repertoire racine pour mettre a jour votre badge.";

/* Virtual FAT32 filesystem organization:

Largest UF2 file size needed is 512kB for ext flash data
With UF2 format overhead: 512kB * 2 = 1MB. 1MB / 512 bytes per sector = 2048 data sectors
2048 entries / (512 bytes per sector / 4 bytes per FAT entry) = 16 FAT sectors = 8kB FAT
There are 2 copies of the FAT, so 16 * 2 = 32 FAT sectors = 16kB FATs

((512kB data * 2 for UF2 format overhead) + 32kB) / sector size = 2112 sectors total
(32kB / sector size) - 32 FATs sectors = 32 reserved sectors (includes BPB, FSInfo and backups)

LBA         Byte address            Description
0x0         0x00000000              BPB
0x1         0x00000200              FSInfo 1
0x2         0x00000400              FSInfo 2
0x3~0x5     0x00000600~0x00000A00   Empty
0x6         0x00000C00              Backup BPB (Pointed by BPB_BkBootSec)
0x7         0x00000E00              Backup FSInfo 1
0x8         0x00001000              Backup FSInfo 2
0x9~0x1F	0x00001200~0x00003E00   Empty
0x20      	0x00004000              FAT first sector (Pointed by BPB_RsvdSecCnt)
0x2F      	0x00005E00              FAT last sector
0x30      	0x00006000              Backup FAT first sector
0x3F      	0x00007E00              Backup FAT last sector
0x40      	0x00008000              Root directory
0x43      	0x00008600              README.TXT contents
0x44      	0x00008800              Update file contents
*/

static void fat32_read_bpb(uint8_t *b) {
    fat32_bpb_t *bpb = (fat32_bpb_t*)b;

    memset(b, 0, FAT32_SECTOR_SIZE);    // Clear

    bpb->BS_jmpBoot[0] = 0xEB;
    bpb->BS_jmpBoot[1] = 0xFE;
    bpb->BS_jmpBoot[2] = 0x90;

    memcpy(bpb->BS_OEMName, "MSDOS5.0", 8);
    bpb->BPB_BytsPerSec = FAT32_SECTOR_SIZE;
    bpb->BPB_SecPerClus = 1;
    bpb->BPB_RsvdSecCnt = FAT32_RESERVED_SECTORS;
    bpb->BPB_NumFATs = 2;
    bpb->BPB_RootEntCnt = 0x0000;
    bpb->BPB_TotSec16 = 0x0000;
    bpb->BPB_Media = 0xF8;
    bpb->BPB_FATSz16 = 0x0000;
    bpb->BPB_SecPerTrk = 0x003F;
    bpb->BPB_NumHeads = 0x00FF;
    bpb->BPB_HiddSec = 0x0000003F;
    bpb->BPB_TotSec32 = FAT32_TOTAL_SECTORS;

    // FAT32 Structure
    bpb->BPB_FATSz32 = FAT32_FAT_SECTORS;
    bpb->BPB_ExtFlags = 0x0000;
    bpb->BPB_FSVer = 0x0000;
    bpb->BPB_RootClus = 0x00000002;     // Root dir cluster number + 2
    bpb->BPB_FSInfo = 0x0001;
    bpb->BPB_BkBootSec = 0x0006;        // Backup bootsector
    bpb->BS_DrvNum = 0x80;
    bpb->BS_BootSig = 0x29;
    bpb->BS_VolID = 0x00001234;
    memcpy(bpb->BS_VolLab, "BADGE      ", 11);
    memcpy(bpb->BS_FilSysType, "FAT32   ", 8);

    b[510] = 0x55;
    b[511] = 0xAA;
}

static void fat32_read_fsinfo(uint8_t *b) {
    fat32_fsinfo_t *fsinfo = (fat32_fsinfo_t*)b;

    memset(b, 0, FAT32_SECTOR_SIZE);    // Clear

    fsinfo->FSI_LeadSig = 0x41615252;
    fsinfo->FSI_StrucSig = 0x61417272;
    fsinfo->FSI_Free_Count = FAT32_DATA_SECTORS;
    fsinfo->FSI_Nxt_Free = 0x00000805;

    b[510] = 0x55;
    b[511] = 0xAA;
}

static void fat32_read_fsinfo2(uint8_t *b) {
    memset(b, 0, FAT32_SECTOR_SIZE);    // Clear

    b[510] = 0x55;
    b[511] = 0xAA;
}

static void fat32_read_fat_table(uint8_t *b, uint32_t addr) {
    memset(b, 0, FAT32_SECTOR_SIZE);    // Clear

    if (addr == FAT32_FAT_START) {
        uint32_t *b32 = (uint32_t*)b;
        b32[0] = 0x0FFFFFF8;    // Should be 0xFFFFFFF8 ?
        b32[1] = 0xFFFFFFFF;

        b32[2] = 0x0FFFFFFF;    // EOC for root dir
        b32[3] = 0x0FFFFFFF;    // EOC for README.TXT
        b32[4] = 0x0FFFFFFF;    // Why ?
        b32[5] = 0x0FFFFFFF;    // Why ?
        b32[6] = 0x0FFFFFFF;    // Why ?
        b32[7] = 0x0FFFFFFF;    // Why ?
    }
}

static void fat32_read_dir_entry(uint8_t *b) {
    fat32_dir_entry_t *dir = (fat32_dir_entry_t*)b;

    memset(b, 0, FAT32_SECTOR_SIZE);    // Clear
    
    memcpy(dir->DIR_Name, "BADGE      ", 11);	// Volume name
    dir->DIR_Attr = FAT32_ATTR_VOLUME_ID;
    dir->DIR_NTRes = 0x00;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = FAT32_MAKE_TIME(0, 0);
    dir->DIR_CrtDate = FAT32_MAKE_DATE(1, 1, 2024);
    dir->DIR_LstAccDate = FAT32_MAKE_DATE(1, 1, 2024);
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0, 0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(1, 1, 2024);
    dir->DIR_FstClusLO = 0x0000;
    dir->DIR_FileSize = 0x00000000;

    dir++;

    memcpy(dir->DIR_Name, "README  TXT", 11);
    dir->DIR_Attr = FAT32_ATTR_ARCHIVE | FAT32_ATTR_READ_ONLY;
    dir->DIR_NTRes = 0x18;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = FAT32_MAKE_TIME(0, 0);
    dir->DIR_CrtDate = FAT32_MAKE_DATE(1, 1, 2024);
    dir->DIR_LstAccDate = FAT32_MAKE_DATE(1, 1, 2024);
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0, 0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(1, 1, 2024);
    dir->DIR_FstClusLO = FAT32_README_CLUSTER;
    dir->DIR_FileSize = 512;
}

static void fat32_read_readme(char * buffer, uint32_t addr) {
    memset(buffer, 0, 512);
    memcpy(buffer, readme_contents, sizeof(readme_contents));

    uint32_t str_length = sizeof(readme_contents);

    str_length += sprintf(buffer + str_length,
		"\n\nID: %u\nName: %s\nFav ID: %u\nSpecies A: %s\nSpecies B: %s",
		user_data_ptr->user_id,
		user_data_ptr->name[16] ? "-" : user_data_ptr->name,
		user_data_ptr->fav_id,
		user_data_ptr->species_a[16] ? "-" : user_data_ptr->species_a,
		user_data_ptr->species_b[16] ? "-" : user_data_ptr->species_b
	);

    str_length += sprintf(buffer + str_length, "\n\nUID: %08lX %08lX %08lX\nBL: 1\nAPP: ", uid[0], uid[1], uid[2]);
    str_length += sprintf(buffer + str_length, "%08lX", app_checksum);
    if (!app_valid)
    	str_length += sprintf(buffer + str_length, " (INVALID)");
}

uint8_t fat32_read(uint8_t * buffer, uint32_t addr) {
	if (addr & (FAT32_SECTOR_SIZE - 1))
		return 1;   // Wrong alignment

	if (addr == 0x0000 || addr == 0x0C00) {
		fat32_read_bpb(buffer);      // Main BPB or backup
	} else if (addr == 0x0200 || addr == 0x0E00) {
		fat32_read_fsinfo(buffer);   // Main FSInfo or backup
	} else if (addr == 0x0400 || addr == 0x1000) {
		fat32_read_fsinfo2(buffer);  // Why is this needed ?
	} else if (addr >= FAT32_FAT_START && addr < FAT32_FATS_END) {
		fat32_read_fat_table(buffer, addr);  // Main FAT or backup
	} else if (addr == FAT32_ROOT_DIR_START) {
		fat32_read_dir_entry(buffer);        // Root dir
	} else if (addr >= FAT32_README_TXT_START && addr < (FAT32_README_TXT_START + FAT32_SECTOR_SIZE)) {
		fat32_read_readme((char*)buffer, addr); 	// README.TXT contents
	} else {
		memset(buffer, 0, FAT32_SECTOR_SIZE);
	}

	return 0;
}

uint8_t fat32_write(uint8_t * buffer, uint32_t addr) {
	if (addr & (FAT32_SECTOR_SIZE - 1))
		return 1;   // Wrong alignment

    if (addr >= FAT32_ROOT_DIR_START) {
		if (process_uf2_data(buffer, addr))
			return 1;
    }

	return 0;
}
