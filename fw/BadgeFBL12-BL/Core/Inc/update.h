// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef __UPDATE_H__
#define __UPDATE_H__

#include "main.h"

#define UF2_PAYLOAD_SIZE 256

  // 32 byte UF2 header
typedef struct {
  uint32_t magicStart0;
  uint32_t magicStart1;
  uint32_t flags;
  uint32_t targetAddr;
  uint32_t payloadSize;
  uint32_t blockNo;
  uint32_t numBlocks;
  uint32_t fileSize; // or familyID;
  uint8_t data[476];
  uint32_t magicEnd;
} UF2_Block_t;

static_assert(sizeof(UF2_Block_t) == 512, "UF2_Block_t size not 512 bytes !");

// Custom fields after data[]
typedef struct {
  uint8_t id;
  uint8_t dest_index;
  uint16_t block_sum;
  uint32_t overall_sum;
} UF2_Custom_t;

static_assert(sizeof(UF2_Custom_t) == 8, "UF2_Custom_t size not 8 bytes !");

uint8_t process_uf2_data(uint8_t *buffer, uint32_t bufsize);

#endif /* __UPDATE_H__ */
