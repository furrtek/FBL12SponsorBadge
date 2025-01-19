// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "synth.h"
#include "touch.h"
#include "luts.h"
#include "display.h"
#include "audio.h"
#include "flash.h"
#include "idle.h"

uint8_t octave = 0;	// 0~2
uint8_t audio_amp_timer = 0;
uint8_t touched_key_a, touched_key_b;
uint8_t prev_key_a = 0, prev_key_b = 0;

void enter_synth() {
	common_enter();
	Enable_SpkAmp();
	led_blink(LED_MASK_REC);
	led_blink(LED_MASK_PLAY);
	led_on(LED_MASK_STOP);
	state = STATE_SYNTH;
	audio_op = AUDIO_OP_SYNTH;
}

void handle_synth() {
	// Display: "AA BB OS"

	if (touched_rise[TOUCH_B_STOP]) {
		enter_idle();
		return;
	}

	if (touched_rise[TOUCH_A_REC]) {
		octave = (octave == 0) ? 0 : octave - 1;
		audio_amp_timer = TIMEOUT;
	}
	if (touched_rise[TOUCH_OK_PLAY]) {
		octave = (octave == 2) ? 2 : octave + 1;
		audio_amp_timer = TIMEOUT;
	}
	disp_buf[6] = (octave == 0) ? char_lut[SYMBOL_DOWN] : (octave == 1) ? 0 : char_lut[SYMBOL_UP];

	if (touched_rise[TOUCH_LEAR]) {
		synth_type = (synth_type - 1) & 3;
		audio_amp_timer = TIMEOUT;
	}
	if (touched_rise[TOUCH_REAR]) {
		synth_type = (synth_type + 1) & 3;
		audio_amp_timer = TIMEOUT;
	}
	PrintNum(synth_type);

	if (touched_keys) {
		// Generate sound, reset timeout
		audio_op = AUDIO_OP_SYNTH;
		audio_amp_timer = TIMEOUT;
	} else if (!touched_keys) {
		// Immediate silence, let timeout run
		audio_op = AUDIO_OP_NOP;
	}

	// Get first and second touched keys for polyphony starting from C upwards
	touched_key_a = 0xFF;
	touched_key_b = 0xFF;
	for (uint8_t key = 0; key < 12; key++) {
		if (touched_keys & (1 << key)) {
			if (touched_key_a == 0xFF) {
				touched_key_a = key;
			} else
				if (touched_key_b == 0xFF)
					touched_key_b = key;
		}
	}

	if (touched_key_a != prev_key_a) {
		decay_timer = 0;	// For lpf synth

		// For voice synth
		memset(audio_buffer, 0, 256);	// Start with a buffer of silence
		audio_buffer_get = 0;	// Start from beginning
		audio_read_addr = (4 << 15) + 256;	// Eight 32kB slots
		audio_read_count = 0;
		audio_read_max = flash_readword(4 << 15);
		if (audio_read_max > 128)	// Sanitize
			audio_read_max = 128;

		preload_audio(256);	// Preload second buffer

		if (touched_key_a == 0xFF) {
			PrintTxt(0, "- ");
			tone_a_delta = 0;
		} else {
			PrintTxt(0, key_name_lut[touched_key_a]);
			tone_a_delta = tone_lut[touched_key_a + (octave * 12)];
		}
	}

	if (touched_key_b != prev_key_b) {
		if (touched_key_b == 0xFF) {
			PrintTxt(3, "- ");
			tone_b_delta = 0;
		} else {
			PrintTxt(3, key_name_lut[touched_key_b]);
			tone_b_delta = tone_lut[touched_key_b + (octave * 12)];
		}
	}

	// Return to idle state after some time with no touches
	if (audio_amp_timer) {
		audio_amp_timer--;
		if (!audio_amp_timer)
			enter_idle();
	}

	prev_key_a = touched_key_a;
	prev_key_b = touched_key_b;
}
