// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_AUDIO_H_
#define INC_AUDIO_H_

typedef enum {
	AUDIO_OP_NOP = 0,
	AUDIO_OP_BEEP,
	AUDIO_OP_PLAY,
	AUDIO_OP_SYNTH,
	AUDIO_OP_REC,
} audio_op_t;

typedef struct {
	uint8_t note;
	uint8_t duration;
} beep_t;

extern const beep_t melody_start[];
extern const beep_t melody_prog[];
extern const beep_t melody_alert[];
extern const beep_t melody_boop[];

extern volatile uint8_t flag_empty, flag_full;
extern volatile uint16_t audio_buffer_get, audio_buffer_put;
extern volatile uint16_t audio_pitch_acc;
extern volatile uint16_t audio_pitch_delta;
extern volatile uint8_t audio_buffer[512];
extern volatile uint16_t tone_a_delta, tone_a_acc;
extern volatile uint16_t tone_b_delta, tone_b_acc;
extern uint8_t (*synth_gen[4])();
extern uint16_t decay_timer;
extern uint8_t synth_type;
extern volatile audio_op_t audio_op;
extern uint32_t audio_read_addr;
extern uint16_t audio_read_count;
extern uint16_t audio_read_max;
extern uint32_t audio_write_addr;
extern uint16_t audio_write_count;
extern uint16_t audio_write_max;

extern volatile const beep_t * audio_melody_ptr;
extern volatile uint8_t audio_melody_index;
extern volatile uint16_t audio_melody_timer;

void beep(const beep_t * melody);

void rec_stop();
void preload_audio(uint16_t dest);
void play_sample(uint8_t index, uint8_t pitch);
void Disable_OpAmp();
void Enable_OpAmp();
void Disable_SpkAmp();
void Enable_SpkAmp();

#endif /* INC_AUDIO_H_ */
