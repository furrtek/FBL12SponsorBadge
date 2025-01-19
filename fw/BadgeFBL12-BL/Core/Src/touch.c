// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "touch.h"
#include "luts.h"

// PCB B MCU touch					touch_val	testmode	Normal	Touched	Mid
// TOUCH12 G1 IO1 Left btn top A	[0]			0			316     170		243
// TOUCH13 G1 IO2 Left ear			[4]			4			171     100		135	key4
// TOUCH14 G1 IO3 Right ear			[8]			8			191		102		146 key8
// TOUCH19 G5 IO1 Right button OK	[1]			1			621		310		465
// TOUCH20 G5 IO2 Nose				[5]			5			400		220 	310	key5
// TOUCH21 G5 IO3 Left btn bot B	[9]			9			370		210		290	key9

volatile uint16_t touch_val[12] = { 0 };	// (io << 2) + group
uint8_t touched[12] = { 0 };
uint8_t touched_prev[12] = { 0 };
uint8_t touched_rise[12] = { 0 };

// These values depend on sense caps and touch pad ares, they are found by testing (test mode 1)
const uint16_t touch_thr[12] = {	// (io << 2) + group
	243, 465, 0, 0,
	135, 310, 0, 0,
	146, 290, 0, 0
};
