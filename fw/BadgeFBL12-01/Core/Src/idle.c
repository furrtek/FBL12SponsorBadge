// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "idle.h"
#include "luts.h"
#include "misc.h"
#include "display.h"
#include "user_data.h"
#include "audio.h"
#include "ir.h"
#include "rec.h"
#include "touch.h"
#include "synth.h"

uint8_t name_anim_state = 0;
uint16_t name_anim_timer = NAME_ANIM_PERIOD;
uint8_t name_anim_state_idx;
uint8_t name_anim_a = 0;
uint8_t name_anim_b = 0;
uint16_t name_page_timer;
uint8_t name_page;
uint8_t beacon_timer = BEACON_PERIOD;	// IR beacon frame interval

// Anim index, display time
const uint8_t name_anim_state_lut[8][2] = {
	{0, UI_RATE * 5},
	{1, UI_RATE * 6},
	{0, UI_RATE * 5},
	{1, UI_RATE * 6},
	{0, UI_RATE * 5},
	{2, UI_RATE * 1},
	{0, UI_RATE * 5},
	{2, UI_RATE * 1},
};

void enter_idle() {
	common_enter();
	PrintTxt(0, user_data_ptr->name);
	Disable_SpkAmp();
	irrx_enabled = 1;
	name_page_timer = NAME_PAGE_PERIOD;
	name_page = 0;
	audio_op = AUDIO_OP_NOP;
	state = STATE_IDLE;
	name_anim_state_idx = 0;
	name_anim_state = name_anim_state_lut[0][0];
	name_anim_timer = name_anim_state_lut[0][1];
	led_on(LED_MASK_REC);
	led_on(LED_MASK_PLAY);
	led_off(LED_MASK_STOP);
}

uint8_t short_name() {
	// Check name length
	for (uint8_t c = 0; c < 16; c++) {
		if (!(user_data_ptr->name[c])) {
			// Found null
			if (c < 8) {
				return 1;	// Short name, single page
			} else {
				return 0;	// Toggle page
			}
			break;
		}
	}
	return 0;	// Toggle page
}

void play_react(uint8_t v, uint8_t react_idx) {
	uint8_t pitch_idx = 2;

	if (react_idx == 0)
		pitch_idx = user_data_ptr->react_boop >> 4;
	else if (react_idx == 1)
		pitch_idx = user_data_ptr->react_lear >> 4;
	else if (react_idx == 2)
		pitch_idx = user_data_ptr->react_rear >> 4;
	else if (react_idx == 3)
		pitch_idx = user_data_ptr->react_alert >> 4;

	play_sample(v & 15, pitch_idx);
}

void handle_idle() {
	if (name_anim_state == 0) {
		// Just show name with breathing brightness
		set_brightness(128 + (sin_lut[(name_anim_timer << 4) & 0xFF] >> 1));
		ClearDisp();

		if (!name_page_timer) {
			name_page_timer = NAME_PAGE_PERIOD;

			if (short_name()) {
				name_page = 0;	// Short name, single page
			} else {
				name_page ^= 1;	// Toggle page
			}
		} else
			name_page_timer--;

		if (name_page == 0)
			PrintTxt(0, user_data_ptr->name);
		else
			PrintTxt(0, &(user_data_ptr->name[8]));

		/*
		 * Disabled for demo purposes
		// Name scrolling
		// Scan name from null
		//	If null is found at < 8: don't scroll (small name)
		//  If null is found at scroll_index + 8 or before: reset scrolling
		//	Otherwise: scroll
		if (!scroll_timer) {
			scroll_timer = SCROLL_PERIOD;
			for (c = 0; c < 16; c++) {
				if (!(user_data_ptr->name[c])) {
					// Found null
					if (c < 8) {
						scroll_index = 0;	// Short name, no scrolling
					} else {
						if (c <= scroll_index + 8)
							scroll_index = 0;	// Reset scrolling
						else
							scroll_index++;
					}
					break;
				}
			}
			PrintTxt(0, &user_data_ptr->name[scroll_index]);	// scroll_index
		} else
			scroll_timer--;
		*/
	} else if (name_anim_state == 1) {
		// Fade each letter
		ClearDisp();
		PrintChar(name_anim_a & 7, user_data_ptr->name[name_anim_a]);
		set_brightness(255 - name_anim_b);

		if (name_anim_b < 225) {
			name_anim_b += 45;
		} else {
			name_anim_b = 0;
			name_anim_a++;	// Next letter
		}

		if (!(user_data_ptr->name[name_anim_a]) || (name_anim_a > 16))
			name_anim_timer = 0;	// Force next state
	} else if (name_anim_state == 2) {
		// Random shit
		for (uint8_t c = 0; c < 8; c++)
			disp_buf[c] = XORShift();
		set_brightness(128 + ((XORShift() & 127) >> 1));
	}

	if (!name_anim_timer) {
		name_page_timer = NAME_PAGE_PERIOD;
		name_anim_a = 0;
		name_anim_b = 0;
		name_page = 0;
		name_anim_state_idx = (name_anim_state_idx + 1) & 7;
		name_anim_state = name_anim_state_lut[name_anim_state_idx][0];
		name_anim_timer = name_anim_state_lut[name_anim_state_idx][1];
	} else
		name_anim_timer--;

	if (touched_rise[TOUCH_NOSE]) {
		ClearDisp();
		PrintTxt(2, "BOOP");
		enter_temp(20);

		play_react(user_data_ptr->react_boop, 0);	// TODO: Have an array for reacts
	} else if (touched_rise[TOUCH_LEAR]) {
		play_react(user_data_ptr->react_lear, 1);
	} else if (touched_rise[TOUCH_REAR]) {
		play_react(user_data_ptr->react_rear, 2);
	} else if (touched_rise[TOUCH_OK_PLAY]) {
		enter_synth();
	} else if (touched_rise[TOUCH_A_REC]) {
		enter_rec_sel();
	}

	// Decrement beacon_timer, try sending a beacon frame if IR isn't busy
	// Disable while nose is pressed (IR programming unlock)
	// If busy: try again next cycle
	// If frame sent: reload beacon timer
	if (!beacon_timer) {
		if ((!irtx_busy) && (!touched[TOUCH_NOSE])) {
			irtx_beacon(user_data_ptr);	// Beacon frame
			beacon_timer = BEACON_PERIOD;
		}
	} else
		beacon_timer--;
}
