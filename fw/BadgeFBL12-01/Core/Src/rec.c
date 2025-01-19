// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include "main.h"
#include "rec.h"
#include "touch.h"
#include "luts.h"
#include "display.h"
#include "audio.h"
#include "idle.h"
#include "misc.h"
#include "user_data.h"
#include "flash.h"

uint8_t sel_react = 0;
uint8_t sel_predef = 0;
uint8_t sel_pitch = 0;
uint16_t timeout = 0;
uint8_t react_is_predef = 0;

void save_react(uint8_t sample_slot, uint8_t pitch);
void play_pitch();

const char str_boop[] = "BOOP";
const char str_left[] = "LEFT";
const char str_right[] = "RIGHT";
const char str_alert[] = "ALERT";
const char str_voice[] = "VOICE";

const char * list_rec_str[] = {
	str_boop,
	str_left,
	str_right,
	str_alert,
	str_voice
};

// This must match contents of ext flash !
const char str_cat[] = "CAT";
const char str_wolf[] = "WOLF";
const char str_dragon[] = "DRAGON";
const char str_horse[] = "HORSE";
const char str_parrot[] = "PARROT";
const char str_cow[] = "COW";
const char str_sheep[] = "SHEEP";
const char str_donkey[] = "DONKEY";

const char * list_predef_str[] = {
	str_cat,
	str_wolf,
	str_dragon,
	str_horse,
	str_parrot,
	str_cow,
	str_sheep,
	str_donkey
};

const char str_pitch_mm[] ="<< ";
const char str_pitch_m[] = " < ";
const char str_pitch_n[] = " ' ";
const char str_pitch_p[] = " > ";
const char str_pitch_pp[] =" >>";

const char * list_pitch_str[] = {
	str_pitch_mm,
	str_pitch_m,
	str_pitch_n,
	str_pitch_p,
	str_pitch_pp
};

void enter_rec_sel() {
	common_enter();
	PrintTxt(0, list_rec_str[0]);
	sel_react = 0;
	timeout = 0;
	state = STATE_REC_SEL;
	led_blink(LED_MASK_REC);
	led_on(LED_MASK_STOP);
}

// TODO: Use these everywhere suitable
void dec_warp(uint8_t * ptr, uint8_t max) {
	if (*ptr == 0)
		*ptr = max;
	else
		*ptr = *ptr - 1;
}

void inc_warp(uint8_t * ptr, uint8_t max) {
	if (*ptr == max)
		*ptr = 0;
	else
		*ptr = *ptr + 1;
}

void handle_rec_sel() {
	// Display: Item name

	if (timeout >= TIMEOUT) {
		enter_idle();	// Auto exit
		return;
	} else
		timeout++;

	if (touched_rise[TOUCH_A_REC]) {
		enter_rec_active();		// Start recording mic
	} else if (touched_rise[TOUCH_OK_PLAY]) {
		if (sel_react < 4)		// Only allow predef selection for reactions, not VOICE
			enter_rec_predef();
	} else if (touched_rise[TOUCH_B_STOP]) {
		enter_idle();	// Exit
	} else if (touched_rise[TOUCH_LEAR]) {
		timeout = 0;
		dec_warp(&sel_react, 4);
		ClearDisp();
		PrintTxt(0, list_rec_str[sel_react]);
	} else if (touched_rise[TOUCH_REAR]) {
		timeout = 0;
		inc_warp(&sel_react, 4);
		ClearDisp();
		PrintTxt(0, list_rec_str[sel_react]);
	}

	if (sel_react < 4)
		led_on(LED_MASK_PLAY);	// Show LED play available for reactions only
}

void enter_rec_predef() {
	common_enter();
	PrintTxt(0, list_predef_str[0]);
	sel_predef = 0;
	timeout = 0;
	state = STATE_REC_PREDEF;
	led_blink(LED_MASK_REC);
	led_on(LED_MASK_PLAY);
	led_on(LED_MASK_STOP);
	react_is_predef = 1;
}

void handle_rec_predef() {
	// Display: Predef name

	if (timeout >= TIMEOUT) {
		enter_idle();	// Auto exit
		return;
	} else
		timeout++;

	if (touched_rise[TOUCH_A_REC]) {
		// Mark react as predef sample with default pitch in case we exit during pitch select
		save_react(sel_predef + 8, 2);
		enter_rec_pitch();
	} else if (touched_rise[TOUCH_OK_PLAY]) {
		timeout = 0;
		play_sample(sel_predef + 8, 2);
	} else if (touched_rise[TOUCH_B_STOP]) {
		enter_idle();	// Exit
	} else if (touched_rise[TOUCH_LEAR]) {
		timeout = 0;
		dec_warp(&sel_predef, 7);
		ClearDisp();
		PrintTxt(0, list_predef_str[sel_predef]);
		play_sample(sel_predef + 8, 2);
	} else if (touched_rise[TOUCH_REAR]) {
		timeout = 0;
		inc_warp(&sel_predef, 7);
		ClearDisp();
		PrintTxt(0, list_predef_str[sel_predef]);
		play_sample(sel_predef + 8, 2);
	}
}

void enter_rec_active() {
	common_enter();
	state = STATE_REC_ACTIVE;
	led_blink(LED_MASK_REC);
	led_off(LED_MASK_PLAY);
	led_off(LED_MASK_STOP);

	react_is_predef = 0;

	// Mark react as recorded sample with default pitch in case we exit during pitch select
	// If we're seeting VOICE, this is just ignored in save_react()
	save_react(sel_react, 2);

	Enable_OpAmp();
	LL_ADC_REG_StopConversion(ADC1);	// Just in case, the rest fails if a conversion is ongoing
	LL_ADC_Disable(ADC1);
	LL_ADC_SetResolution(ADC1, LL_ADC_RESOLUTION_8B);
	LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_EXT_TIM2_TRGO);
	LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_8);
	audio_buffer_put = 0;	// Start from beginning
	audio_write_addr = (sel_react << 15) + 256;	// Eight 32kB slots - 1 page
	audio_write_count = 0;
	audio_write_max = 127;	// 128 pages - 1
	LL_ADC_EnableIT_EOC(ADC1);
	LL_ADC_Enable(ADC1);
	LL_ADC_REG_StartConversion(ADC1);

	audio_op = AUDIO_OP_REC;
}

void handle_rec_active() {
	// Display: progressbar

	for (uint32_t c = 0; c < (audio_write_count >> 1); c++)
		disp_buf[c >> 3] |= (1 << (c & 7));

	if ((!touched[TOUCH_A_REC]) || (audio_write_count >= audio_write_max)) {
		rec_stop();

		// Write recording length in first page
		uint8_t temp_buf[256];
		memcpy(temp_buf, &audio_write_count, sizeof(audio_write_count));

		flash_program(sel_react << 15, (uint8_t*)temp_buf);

		if (sel_react < 4)
			enter_rec_pitch();
		else
			enter_idle();	// Exit after VOICE recording
	}
}

void enter_rec_pitch() {
	common_enter();
	PrintTxt(0, "PITCH");
	PrintTxt(5, list_pitch_str[2]);
	sel_pitch = 2;	// Normal
	timeout = 0;
	state = STATE_REC_PITCH;
	led_blink(LED_MASK_REC);
	led_on(LED_MASK_PLAY);
	led_on(LED_MASK_STOP);
	play_pitch();
}

// Select item	BOOP, LEAR, REAR, ALERT, VOICE		Sets sel_react 0~4
// BOOP, LEAR, REAR, ALERT: Reacts
//	Predef											Sets react_is_predef = 1
//		Pitch, save user data, exit					Sets sel_pitch 0~4
//	Record											Sets react_is_predef = 0
//		Pitch, save user data, exit					Sets sel_pitch 0~4
// VOICE
//	Record, exit

void save_react(uint8_t sample_slot, uint8_t pitch) {
	if (sel_react == 0)
		set_react_boop(sample_slot, pitch);
	else if (sel_react == 1)
		set_react_lear(sample_slot, pitch);
	else if (sel_react == 2)
		set_react_rear(sample_slot, pitch);
	else if (sel_react == 3)
		set_react_alert(sample_slot, pitch);
}

void play_pitch() {
	play_sample(react_is_predef ? sel_predef + 8 : sel_react, sel_pitch);
}

void handle_rec_pitch() {
	// Display: Pitch

	if (timeout >= TIMEOUT) {
		enter_idle();	// Auto exit
		return;
	} else
		timeout++;

	if (touched_rise[TOUCH_A_REC]) {
		// Update pitch
		save_react(react_is_predef ? sel_predef + 8 : sel_react, sel_pitch);
		enter_idle();	// Exit
	} else if (touched_rise[TOUCH_OK_PLAY]) {
		timeout = 0;
		play_pitch();
	} else if (touched_rise[TOUCH_B_STOP]) {
		enter_idle();	// Exit
	} else if (touched_rise[TOUCH_LEAR]) {
		timeout = 0;
		dec_warp(&sel_pitch, 4);
		PrintTxt(5, list_pitch_str[sel_pitch]);
		play_pitch();
	} else if (touched_rise[TOUCH_REAR]) {
		timeout = 0;
		inc_warp(&sel_pitch, 4);
		PrintTxt(5, list_pitch_str[sel_pitch]);
		play_pitch();
	}
}

