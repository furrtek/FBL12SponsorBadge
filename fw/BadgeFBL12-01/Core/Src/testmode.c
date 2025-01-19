// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include "main.h"
#include "testmode.h"
#include "display.h"
#include "ir.h"
#include "luts.h"
#include "touch.h"
#include "audio.h"
#include "misc.h"
#include "flash.h"

// 0:
// -All LEDs on, two brightness levels
// -Vertical segment test
// -Digit test
// -Segment test (uses individual LEDs)
// 1:
// -Touch all touch pads, starts with none, shows OK when all have been touched
// 2:
// -First digit should flash, second digit should stay on
// -Plugging in powered USB should show "CHRG", removing it should hide
// 3:
// -Reports ext flash predef section 32bit checksum
// -Sawtooth sweep

uint8_t test_mode = 0;
uint8_t test_mode_step = 0;
uint8_t test_mode_timer = 0;
uint32_t test_mode_touched = 0;

void enter_testmode(const uint8_t mode) {
	if (mode > 4)
		return;

	state = STATE_TESTMODE;
	audio_op = AUDIO_OP_NOP;
	test_mode = mode;
	test_mode_step = 0;
	test_mode_timer = 0;
	test_mode_touched = 0;
}

void handle_testmode() {
	if (test_mode == 0) {
		// LED test
		if (!test_mode_timer) {
			if (test_mode_step == 0) {
				// All LEDs on, max brightness
				set_brightness(255);
				memset((void*)disp_buf, 0xFF, 9);
				test_mode_timer = 20;	// 1s
				test_mode_step++;
			} else if (test_mode_step == 1) {
				// All LEDs on, low brightness
				set_brightness(50);
				test_mode_timer = 20;	// 1s
				test_mode_step++;
			} else if ((test_mode_step >= 2) && (test_mode_step < 6)) {
				// Vertical segment test
				set_brightness(255);
				ClearDisp();
				memset((void*)disp_buf, (test_mode_step & 1) ? 0x80 : 0x20, 8);
				disp_buf[8] = 0;
				test_mode_timer = 10;	// 0.5s
				test_mode_step++;
			} else if ((test_mode_step >= 6) && (test_mode_step < 15)) {
				// Digit test
				set_brightness(255);
				ClearDisp();
				disp_buf[8] = 0;
				disp_buf[test_mode_step - 6] = 0xFF;
				test_mode_timer = 5;	// 0.25s
				test_mode_step++;
			} else if (test_mode_step >= 15) {
				// Segment test
				ClearDisp();
				memset((void*)disp_buf, (1 << (test_mode_step - 15)), 9);
				test_mode_timer = 5;	// 0.25s
				if (test_mode_step == 22)
					test_mode_step = 0;	// Loop
				else
					test_mode_step++;
			}
		} else
			test_mode_timer--;
	} else if (test_mode == 1) {
		// Turn on segments (12 + 6) for each keyboard key and MCU pad touched
		ClearDisp();
		for (uint8_t c = 0; c < 12; c++) {
			if (touched_keys & (1 << c))
				test_mode_touched |= (1 << c);
		}
		if (touched[TOUCH_A_REC]) test_mode_touched |= (1 << 12);
		if (touched[TOUCH_B_STOP]) test_mode_touched |= (1 << 13);
		if (touched[TOUCH_OK_PLAY]) test_mode_touched |= (1 << 14);
		if (touched[TOUCH_NOSE]) test_mode_touched |= (1 << 15);
		if (touched[TOUCH_LEAR]) test_mode_touched |= (1 << 16);
		if (touched[TOUCH_REAR]) test_mode_touched |= (1 << 17);
		disp_buf[0] = test_mode_touched;
		disp_buf[1] = test_mode_touched >> 8;
		disp_buf[2] = test_mode_touched >> 16;

		if (test_mode_touched == ((1 << 18) - 1))
			PrintTxt(3, "OK");

		// Display MCU touch sense value, selection by lowest pressed keyboard key
		for (uint8_t c = 0; c < 12; c++) {
			if (touched_keys & (1 << c)) {
				PrintNum(touch_val[c]);
				break;
			}
		}
	} else if (test_mode == 2) {
		ClearDisp();
		PrintNum(2);
		if (!test_mode_timer) {
			ir_test_passed = 0;
			irtx_test();
			disp_buf[0] = 0xFF;		// Flash second digit to indicate IR transmit
			test_mode_timer = 40;	// 2s
		} else
			test_mode_timer--;

		if (ir_test_passed)
			disp_buf[1] = 0xFF;		// Light up 2nd digit to indicate IR received ok

		// Charge status display
		if (is_charging())
			PrintTxt(2, "CHRG");
	} else if (test_mode == 3) {
		ClearDisp();
		PrintNum(3);

		// Dual sawtooth sweep
		Enable_SpkAmp();
		audio_op = AUDIO_OP_SYNTH;
		synth_type = 0;
		if (!test_mode_timer) {
			tone_a_delta = tone_lut[24];
			tone_b_delta = tone_a_delta;
			test_mode_timer = 40;	// 2s
		} else {
			tone_a_delta += 50;
			tone_b_delta += 100;
			test_mode_timer--;
		}

		uart_print("Computing ext flash checksum...\n");
		uint32_t checksum = 0;
		uint8_t buf[256];
		for (uint32_t p = 0; p < 1024; p++) {
			flash_PREAD(0x40000 + (p << 8), buf);
			for (uint16_t b = 0; b < 256; b++)
				checksum += (uint32_t)buf[b];
		}
		sprintf(buf, "Ext flash checksum: %08lx\n", checksum);
		uart_print(buf);
	}
}
