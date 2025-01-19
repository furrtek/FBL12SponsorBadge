// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include <stdio.h>
#include "main.h"
#include "audio.h"
#include "luts.h"
#include "flash.h"

volatile audio_op_t audio_op = AUDIO_OP_NOP;
uint8_t synth_type = 0;
volatile uint16_t tone_a_delta = 0, tone_a_acc = 0;
volatile uint16_t tone_b_delta = 0, tone_b_acc = 0;

volatile uint8_t audio_buffer[512];	// Two 256 byte buffers - 2*32ms @ 8000Hz (125us/byte)
volatile uint8_t flag_empty = 0, flag_full = 0;
volatile uint16_t audio_buffer_get = 0;
volatile uint16_t audio_buffer_put = 0;
volatile uint16_t audio_pitch_acc = 0;
volatile uint16_t audio_pitch_delta = 0;
uint32_t audio_read_addr = 0;
uint16_t audio_read_count = 0;
uint16_t audio_read_max = 0;
uint32_t audio_write_addr = 0;
uint16_t audio_write_count = 0;
uint16_t audio_write_max = 0;

volatile const beep_t * audio_melody_ptr;
volatile uint8_t audio_melody_index;
volatile uint16_t audio_melody_timer;

const beep_t melody_start[] = {
	{36, 20},	// Silence, needed while amp is waking up
	{21, 3},
	{22, 3},
	{23, 3},
	{36, 6},	// Silence
	{33, 3},
	{34, 3},
	{35, 3},
	{0, 0}
};

const beep_t melody_prog[] = {
	{36, 20},	// Silence, needed while amp is waking up
	{22, 2},
	{23, 2},
	{35, 6},
	{36, 6},	// Silence
	{22, 2},
	{23, 2},
	{35, 6},
	{0, 0}
};

const beep_t melody_alert[] = {
	{36, 20},	// Silence, needed while amp is waking up
	{29, 8},
	{35, 8},
	{0, 0}
};

const beep_t melody_boop[] = {
	{36, 20},	// Silence, needed while amp is waking up
	{18, 4},
	{36, 6},	// Silence
	{16, 4},
	{0, 0}
};

const uint16_t lut_pitch_delta[5] = {
	0x2000,
	0x3000,
	0x4000,	// Normal pitch
	0x5000,
	0x6000
};

void preload_audio(uint16_t dest) {
	if (audio_read_count < audio_read_max) {
		// Read to audio_buffer[0...255] or audio_buffer[256...511] from flash
		flash_PREAD(audio_read_addr, (uint8_t*)&audio_buffer[dest]);

		audio_read_addr += 256;
		audio_read_count++;
	} else {
		audio_op = AUDIO_OP_NOP;
	}
}

void rec_stop() {
	Disable_OpAmp();
	audio_op = AUDIO_OP_NOP;
	LL_ADC_REG_StopConversion(ADC1);
}

void play_sample(uint8_t index, uint8_t pitch) {
	uint32_t addr = 0x00000 + ((index & 15) << 15);	// 2x Eight 32kB slots

	Enable_SpkAmp();

	if (pitch > 4)
		pitch = 2;	// Fail safe

	audio_pitch_acc = 0;
	audio_pitch_delta = lut_pitch_delta[pitch];
	audio_buffer_get = 0;	// Start from beginning
	audio_read_addr = addr + 256;	// Eight 32kB slots
	audio_read_count = 0;
	audio_read_max = flash_readword(addr);
	if (audio_read_max > 128)	// Sanitize
		audio_read_max = 128;

	preload_audio(0);

	audio_op = AUDIO_OP_PLAY;
}

void beep(const beep_t * melody) {
	Enable_SpkAmp();
	audio_op = AUDIO_OP_BEEP;
	audio_melody_ptr = melody;
	audio_melody_index = 0;
	audio_melody_timer = 0;
}

// Dual sawtooth
uint8_t synth_saw() {
	uint32_t mixed = 0;

	// Tone B only shouldn't be possible as it can only be set if tone A is already used
	if (tone_a_delta && !tone_b_delta)
		mixed = tone_a_acc >> 8;				// Tone A only
	else if (tone_a_delta && tone_b_delta)
		mixed = (tone_a_acc + tone_b_acc) >> 9;	// Tone A and B

	return (uint8_t)mixed;
}

// Square with LFO PWM
uint8_t synth_pwm() {
	static uint16_t lfo_acc;
	static uint8_t square;

	// tone_a_acc: 8.8 ramp up
	if ((tone_a_acc >> 8) < (16 + (sin_lut[lfo_acc >> 8] >> 1)))
		square = 255;	// Set on wrap
	else
		square = 0;		// Reset on compare match

	lfo_acc += 6;	// 256 / ((4 * 8000) >> 8) = ~1.4s period

	return square;
}

// Square with decaying resonant LPF - TODO: Dual
int32_t prev_sample = 0;
int32_t mom = 0;
uint16_t decay_timer;

uint8_t synth_lpf() {
	int32_t square;
	int32_t dtg;

	square = (tone_a_acc & 0x8000) ? (127 << 8) : (-128 << 8);

	int16_t a = (int16_t)lpf_lut[decay_timer >> 8];
	dtg = square - prev_sample;	// 8.8 - 8.8
	mom += ((dtg * a) >> 8);	// 8.8 * 0.8 -> 8.16 -> 8.8
	prev_sample += (mom + ((dtg * 40) >> 8));	// (8.8 + 8.8) -> 9.8

	if (decay_timer <= (65535 - 20))
		decay_timer += 20;	// 256 / ((8 * 8000) >> 8) = ~0.4s period

	return (uint8_t)(prev_sample >> 10) ^ 0x80;
}

// Galois LFSR noise
/*uint8_t synth_lfsr() {
	static uint16_t prev_tone_a_acc;
	static uint8_t lfsr = 1;

	// tone_a_acc: 8.8 ramp up
	if ((tone_a_acc & 0xFFF) < (prev_tone_a_acc & 0xFFF)) {
		// Update LFSR on wrap
		if (lfsr & 1)
			lfsr = (lfsr >> 1) ^ 0xC0;
		else
			lfsr >>= 1;
	}

	prev_tone_a_acc = tone_a_acc;

	return lfsr;
}*/

uint8_t synth_voice() {
	while (tone_a_acc >= 0x0600) {
		audio_buffer_get = (audio_buffer_get + 1) & 511;
		tone_a_acc -= 0x0600;

		if ((audio_buffer_get == 0) || (audio_buffer_get == 256)) {
			flag_empty = 1;	// Signal buffer empty
		}
	}

	return audio_buffer[audio_buffer_get];
}

uint8_t (*synth_gen[4])() = {
	synth_saw,
	synth_pwm,
	synth_lpf,
	synth_voice
};

void Disable_OpAmp() {
	LL_GPIO_ResetOutputPin(OPAMP_PWR_GPIO_Port, OPAMP_PWR_Pin);
}

void Enable_OpAmp() {
	LL_GPIO_SetOutputPin(OPAMP_PWR_GPIO_Port, OPAMP_PWR_Pin);
}

void Disable_SpkAmp() {
#if AMPSD_POLARITY_POS
	LL_GPIO_SetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#else
	LL_GPIO_ResetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#endif
}

void Enable_SpkAmp() {
#if AMPSD_POLARITY_POS
	LL_GPIO_ResetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#else
	LL_GPIO_SetOutputPin(AMPSD_GPIO_Port, AMPSD_Pin);
#endif
}
