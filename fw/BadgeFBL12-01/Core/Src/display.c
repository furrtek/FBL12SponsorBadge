// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "display.h"
#include "luts.h"

void PrintChar(uint8_t x, const char c) {
	disp_buf[x] = ((c >= 32) && (c < 32 + 64)) ? char_lut[c - 32] : 0;
}

void PrintTxt(uint8_t x, const char * src) {
	char c;

	while ((c = *src++)) {
		if (x >= 8) break;
		disp_buf[x++] = ((c >= 32) && (c < 32 + 64)) ? char_lut[c - 32] : 0;
	}
}

// Right-aligned
void PrintNum(uint16_t v) {
	uint8_t i = 7;

	do {
		disp_buf[i--] = char_lut[(v % 10) + 16];
		v /= 10;
	} while(v);
}

static uint8_t HexShift(const uint8_t v) {
	return v > 9 ? v + 33 - 10 : v + 16;
}

void PrintHex(const uint8_t x, uint8_t v) {
	disp_buf[x] = char_lut[HexShift(v >> 4)];
	disp_buf[x + 1] = char_lut[HexShift(v & 15)];
}

void ClearDisp() {
	for (uint8_t c = 0; c < 8; c++)
		disp_buf[c] = 0;
}

void PrintSeg(const uint8_t v) {
	disp_buf[(v >> 3) & 7] = 1 << (v & 7);
}
