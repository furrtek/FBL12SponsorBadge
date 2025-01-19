// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include <stdio.h>
#include "main.h"
#include "update.h"
#include "flash.h"

uint8_t first_block = 1;

uint32_t total_blocks = 0;
uint32_t block_count = 0;

uint32_t checksum_computed;
uint32_t checksum_correct;

uint8_t page_map[48] = { 0 };	// Flash pages, really (128kB - 32kB - 2kB) / 2kB = 47 used for app

uint8_t update_extflash(uint32_t address, uint8_t * const buffer);
uint8_t update_mcu(uint32_t address, uint8_t * const buffer);

uint8_t process_uf2_data(uint8_t * buffer, uint32_t bufsize) {
	uint8_t ret;
    UF2_Block_t* uf2block = (UF2_Block_t*)buffer;

    uint8_t * data_src;

    // Check UF2 fields
    if ((uf2block->payloadSize != UF2_PAYLOAD_SIZE) ||
    (uf2block->magicStart0 != 0x0A324655) ||
    (uf2block->magicStart1 != 0x9E5D5157) ||
    (uf2block->magicEnd != 0x0AB16F30) ||
    !(uf2block->flags & 1) ||
    !uf2block->numBlocks)
        return 0; // Just ignore

    UF2_Custom_t* custom_fields = (UF2_Custom_t*)(uf2block->data + UF2_PAYLOAD_SIZE);

    // Update format version check
    if (custom_fields->id != 0x2A) {
    	uart_print("Invalid UF2 file version\n");
        return 1;
    }

    if (first_block) {
    	uart_print("UF2 first block\n");

        // Init
        checksum_computed = 0;
        block_count = 0;
        total_blocks = uf2block->numBlocks;

        memset(page_map, 0, sizeof(page_map));

        first_block = 0;
    }

    if (uf2block->targetAddr + UF2_PAYLOAD_SIZE > (custom_fields->dest_index ? SIZE_EXTFLASH_MAX : SIZE_MCUFLASH_MAX)) {
    	sprintf(str_buffer, "UF2 block target invalid 0x%08lx\n", uf2block->targetAddr);
    	uart_print(str_buffer);
        return 1;
    }

    data_src = uf2block->data;

	// Current block data checksum
	uint32_t block_sum = 0;
	for (uint16_t c = 0; c < UF2_PAYLOAD_SIZE; c++)
		block_sum += uf2block->data[c];

	checksum_computed += block_sum;
	checksum_correct = custom_fields->overall_sum;   // All blocks provide the correct overall checksum

	// Check against UF2 field
	if ((block_sum & 0xFFFF) != custom_fields->block_sum) {
    	sprintf(str_buffer, "UF2 block checksum invalid: Computed:%04x Expected:%04x\n", (uint16_t)block_sum, custom_fields->block_sum);
    	uart_print(str_buffer);
		return 1;
	}

    // Write data to memory
	if (custom_fields->dest_index) {
		// Write to external flash
		ret = update_extflash(uf2block->targetAddr, data_src);
	} else {
		// Write to app section
		ret = update_mcu(uf2block->targetAddr, data_src);
	}

	if (ret) {
    	uart_print("Memory write error\n");
		return 1;
	}

    block_count++;

    if (block_count == total_blocks) {
    	first_block = 1;
		if (checksum_computed != checksum_correct) {
	    	sprintf(str_buffer, "Final checksum invalid: Computed:%08lx Expected:%08lx\n", checksum_computed, checksum_correct);
	    	uart_print(str_buffer);
			return 1;
		}
		check_app();
		uart_print("Update done\n");
    }

    return 0;
}

uint8_t update_extflash(uint32_t address, uint8_t * const buffer) {
	sprintf(str_buffer, "update_extflash %08lx\n", address);
	uart_print(str_buffer);

	extflash_program(address, buffer);

    return 0;
}

uint8_t update_mcu(uint32_t address, uint8_t * const buffer) {
	uint32_t page_number = address >> 11;	// 2kB pages

	sprintf(str_buffer, "update_mcu %08lx\n", address);
	uart_print(str_buffer);

	if (page_number >= sizeof(page_map))
		return 1;

	if (!page_map[page_number]) {
		sprintf(str_buffer, "mcuflash_erase %08lx\n", address);
		uart_print(str_buffer);
		if (mcuflash_erase(address))
			return 1;
		page_map[page_number] = 1;	// Mark as erased
	}

	return mcuflash_program(address, buffer, 256);
}
