// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "init.h"
#include "luts.h"
#include "touch.h"
#include "audio.h"
#include "flash.h"
#include "display.h"
#include "ir.h"
#include "user_data.h"
#include "idle.h"
#include "testmode.h"
#include "synth.h"
#include "misc.h"
#include "rec.h"

// UI:																					STATE_IDLE
// REC button long
//	Blink REC LED, select BOOP/LEFT/RIGHT/ALERT/VOICE with up/down, cancel with STOP	STATE_REC_SEL
//	PLAY button long -> choose which predef sample to use								STATE_REC_PREDEF
//		Select with up/down, autoplay on select, chose with REC, cancel with STOP
//	Keep REC pressed: record mic with progressbar										STATE_REC_ACTIVE
//		REC release: goto PITCH
//	Pitch
//		Play back recording or predef, show "PITCH"										STATE_REC_PITCH
//		Select pitch with up/down, chose with PLAY
//		Cancel with STOP
//		Record to user data
// PLAY button -> synth mode															STATE_SYNTH
// LEAR, REAR, BOOP: Play recorded / predef

// Audio recording:
// TIM2 triggers update interrupts at 8kHz (TIM2_IRQHandler)
// ADC automatically starts a conversion via TIM2's TRGO
// ADC triggers a conversion done interrupt (ADC1_COMP_IRQHandler)
// If audio_op is AUDIO_OP_REC, save sample at audio_buffer_put
// Increment audio_buffer_put, set flag_full if now 0 or 256

// Audio playback:
// TIM2 triggers update interrupts at 8kHz (TIM2_IRQHandler)
// A sample is read from audio_buffer_get and send to the DAC
// Increment audio_buffer_get, set flag_empty if now 0 or 256

// MCU flash memory map: 128kB, 2kB pages, 64 pages, 32 sectors
// Pages 16~62: App
// Page 63: User data (user_data_ptr, 0x0801F800)

// Ext flash memory map: 512kB, 256 byte pages, 2048 pages
// Sample format:
//	Page n: uint16_t length (in pages)
//			IMPLEMENTED IN USER DATA INSTEAD: uint16_t pitch_delta (8.8 fixed point)
//	Pages n+1...: uint8_t pcm_data[]
// Pages 0~1023: Recorded samples (8x 4.96s)
// 	Pages 0~127: Recorded sample BOOP
// 	Pages 128~255: Recorded sample LEFT
// 	Pages 256~383: Recorded sample RIGHT
// 	Pages 384~511: Recorded sample ALERT
// 	Pages 512~639: Recorded sample VOICE
// 	Pages 640~1023: Unused
// Pages 1024~2047: Pre-defined samples (8x 4.96s)

// General IR frame format, all bytes:
//	Command, number of parameters, parameters, sum of all previous bytes

// IR: In idle, transmit species and user ID every BEACON_PERIOD
// Beacon frame:
//	u8 IR_CMD_BEACON
//	u8 length (6)
//	u16 user_id
//	u16 species_a_code
//	u16 species_b_code
//	u8 sum
// Beacon frame received and species match A or B:
//	Transmit species match frame, play sample, display species
// Species match frame: IR_CMD_MATCH 0x02 species_a species_b sum
// Species frame match received and species match A or B:
//	Play sample, display species, start anti-spam timeout

// IR_CMD_TESTMODE:
// 0F 37 0n SS
// n = Test mode 0~3

// ADC for audio input and battery voltage monitoring
// DAC for audio output
// I2C1 for communication with XW12A (can't use because of wonky format !)
// SPI1 for communication with external flash
// SPI2 for LED scanning
// TIM2 for ADC/DAC timing
// TIM3 for IR_RX capture
// TIM14 for LED PWM
// TIM15 for display scan and global tick
// TIM16+TIM17 for IR transmit

uint8_t uid[12];						// MCU UID
uint8_t batt_timer = 0;					// Battery voltage check frame interval
uint8_t match_timer = 0;
uint8_t temp_timer = 0;					// Temp state timer
uint8_t blink_timer = 0;				// LED blinking timer
uint16_t touched_keys = 0;
uint8_t blink_mask = 0;
uint32_t xs_state = 0x1337;

TSC_HandleTypeDef h_tsc;

volatile uint8_t flag_tick;
volatile uint8_t disp_buf[9];

state_t prev_state = STATE_IDLE;
state_t state = STATE_IDLE;

const char str_charge[] = "CHARGE";

void common_enter() {
	memset(touched_rise, 0, sizeof(touched_rise));
	ClearDisp();
	set_brightness(255);
	audio_op = AUDIO_OP_NOP;
}

void enter_temp(const uint8_t time) {
	temp_timer = time;
	state = STATE_TEMP;
}

void enter_alert() {
	ClearDisp();
	PrintTxt(0, "O_O");
	temp_timer = 20;	// 1s
	state = STATE_ALERT;
}

void uart_print(char * buffer) {
	char c;

	while ((c = *buffer++)) {
	    while (!LL_USART_IsActiveFlag_TXE(USART2)) {};
	    LL_USART_TransmitData8(USART2, c);
	}
}

int main(void) {
	uint32_t c = 0;
	uint32_t tick_counter = 0;
	uint8_t batt_percent = 0;

	main_init();

	set_brightness(255);
	ClearDisp();
	disp_buf[8] = 0;

	if (is_charging()) {
		PrintTxt(0, str_charge);
	} else {
		batt_percent = read_batt();
		batt_percent = read_batt();

		if (batt_percent > 80)
			PrintTxt(0, "3/3");
		else if (batt_percent > 50)
			PrintTxt(0, "2/3");
		else if (batt_percent > 20)
			PrintTxt(0, "1/3");
		else
			PrintTxt(0, "0/3");
	}
	PrintNum(1);	// Version
	enter_temp(20);		// 1s
	beep(melody_start);

	char str_buffer[64];
	sprintf(str_buffer, "FBL BADGE APP V1 (%08lx)\n", (*(uint32_t*)ADDR_APPSUM));
	uart_print(str_buffer);

	while (1) {
		// Stuff that must be done ASAP
		if (irrx_flag) {
			// Complete IR frame received
			irrx_flag = 0;
			handle_ir_cmd(irrx_frame.cmd);
		}

		if (state == STATE_TESTMODE) {
			// Nothing ?
		} else if (audio_op == AUDIO_OP_REC) {
			if (flag_full) {
				flag_full = 0;
				if (audio_write_count < audio_write_max) {
					// Write audio_buffer[0...255] or audio_buffer[256...511] to flash
					uint16_t src = (audio_buffer_put >= 256) ? 0 : 256;
					flash_program(audio_write_addr, (uint8_t*)&audio_buffer[src]);

					audio_write_addr += 256;
					audio_write_count++;
				} else {
					rec_stop();
				}
			}
		} else if ((audio_op == AUDIO_OP_PLAY) ||
			((audio_op == AUDIO_OP_SYNTH) && (synth_type == 3))) {
			if (flag_empty) {
				flag_empty = 0;

				// Read to audio_buffer[0...255] or audio_buffer[256...511] from flash
				preload_audio((audio_buffer_get >= 256) ? 0 : 256);
			}
		}

		if (flag_tick) {
			// 600Hz
			flag_tick = 0;

			if (tick_counter >= TICK_PERIOD) {
				// 20Hz
				tick_counter = 0;

				// Keyboard touch
				touched_keys = ReadExtTouch();

				// MCU touch - TODO: Only run this when a complete MCU touch measurement cycle ends
				for (c = 0; c < 12; c++) {
					touched[c] = (touch_val[c] < touch_thr[c]) ? 1 : 0;
					touched_rise[c] = touched[c] & ~touched_prev[c];
				}
				memcpy(touched_prev, touched, sizeof(touched));

				// Battery LED state
				if (is_charging()) {
					led_on(LED_MASK_BATT);	// Keep D7 on
				} else {
					if (batt_percent < 10)
						led_blink(LED_MASK_BATT);	// Blink D7
					else
						led_off(LED_MASK_BATT);		// Keep D7 off
				}

				// LED blinking
				if (!blink_timer) {
					blink_timer = BLINK_PERIOD;
					disp_buf[8] ^= blink_mask;
				} else
					blink_timer--;

				if (state != STATE_REC_ACTIVE) {
					// We can use the ADC whenever we're not using the mic
					if (!batt_timer) {
						// Measure battery voltage
						batt_timer = BATT_PERIOD;
						batt_percent = read_batt();
					} else
						batt_timer--;
				}

				if (state == STATE_TESTMODE) {
					handle_testmode();
				} else if (state == STATE_TEMP) {
					// Do nothing for some time, then go to idle state
					// Used to display stuff temporarily
					if (!temp_timer) {
						enter_idle();
					} else
						temp_timer--;
				} else if (state == STATE_ALERT) {
					// Do nothing for some time, then go to temp state
					if (!temp_timer) {
						ClearDisp();
						//PrintTxt(0, user_data_ptr->species_a);
						PrintTxt(0, "WOLF");	// TODO
						enter_temp(20 * 2);	// 2s
					} else
						temp_timer--;
				} else if (state == STATE_IDLE) {
					handle_idle();
				} else if (state == STATE_SYNTH) {
					handle_synth();
				} else if (state == STATE_REC_SEL) {
					handle_rec_sel();
				} else if (state == STATE_REC_PREDEF) {
					handle_rec_predef();
				} else if (state == STATE_REC_ACTIVE) {
					handle_rec_active();
				} else if (state == STATE_REC_PITCH) {
					handle_rec_pitch();
				}
			} else
				tick_counter++;
		}
	}
}

void Error_Handler(void) {
	__disable_irq();
	while (1) {
	}
}
