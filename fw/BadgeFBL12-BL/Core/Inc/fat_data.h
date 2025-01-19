// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef __FAT_DATA_H__
#define __FAT_DATA_H__

#define FAT32_SECTOR_SIZE           512     // In bytes

#define FAT32_ATTR_READ_ONLY        0x01
#define FAT32_ATTR_HIDDEN           0x02
#define FAT32_ATTR_SYSTEM           0x04
#define FAT32_ATTR_VOLUME_ID        0x08
#define FAT32_ATTR_DIRECTORY        0x10
#define FAT32_ATTR_ARCHIVE          0x20
#define FAT32_ATTR_LONG_NAME        (FAT32_ATTR_READ_ONLY | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM | FAT32_ATTR_VOLUME_ID)

#define FAT32_MAKE_DATE(dd, mm, yyyy)   (uint16_t)( ( (((yyyy)-1980) & 0x7F)  << 9) | (((mm) & 0x0F) << 5) | ((dd) & 0x1F) )
#define FAT32_MAKE_TIME(hh,mm)          (uint16_t)(( ((hh) & 0x1F)<< 11) | (((mm) & 0x3F) << 5))

#define FAT32_DATA_SECTORS          ((1 * 1024 * 1024) / FAT32_SECTOR_SIZE)   	// 1MB
#define FAT32_HEADER_SECTORS        ((32 * 1024) / FAT32_SECTOR_SIZE)    		// 32kB

#define FAT32_TOTAL_SECTORS         (FAT32_DATA_SECTORS + FAT32_HEADER_SECTORS)     	// 2112
#define FAT32_FAT_SECTORS           (FAT32_DATA_SECTORS / (FAT32_SECTOR_SIZE / 4))  	// 16
#define FAT32_RESERVED_SECTORS      (FAT32_HEADER_SECTORS - (FAT32_FAT_SECTORS * 2))	// 32
#define FAT32_FAT_START             (FAT32_RESERVED_SECTORS * FAT32_SECTOR_SIZE)    	// 0x4000
#define FAT32_FATS_END              (FAT32_FAT_START + ((FAT32_FAT_SECTORS * 2) * FAT32_SECTOR_SIZE))

#define FAT32_README_CLUSTER        5

#define FAT32_ROOT_DIR_START        FAT32_FATS_END // LBA 0x40
#define FAT32_README_TXT_START      (FAT32_FATS_END + ((FAT32_README_CLUSTER - 2) * FAT32_SECTOR_SIZE)) // LBA 0x43

typedef struct {
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    
    // FAT32 Structure
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BS_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
} __attribute__((packed)) fat32_bpb_t;

typedef struct {
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
} __attribute__((packed)) fat32_fsinfo_t;

typedef struct {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} __attribute__((packed)) fat32_dir_entry_t;

uint8_t fat32_read(uint8_t * buffer, uint32_t addr);
uint8_t fat32_write(uint8_t * buffer, uint32_t addr);

#endif /* __FAT_DATA_H__ */
