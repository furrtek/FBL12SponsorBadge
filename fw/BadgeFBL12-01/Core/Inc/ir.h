// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_IR_H_
#define INC_IR_H_

#define IR_CMD_TEST 0x00
#define IR_CMD_TESTMODE 0x0F
#define IR_CMD_SETID 0x1E
#define IR_CMD_SETNAME 0x2D
#define IR_CMD_SETSPECIES 0x3C
#define IR_CMD_PLAYSOUND 0x87
#define IR_CMD_SILENCE 0x96
#define IR_CMD_BEACON 0xA5
#define IR_CMD_MATCH 0xB4

#define IRRX_TIMER_TICKS 1583	// One IR timeslot in TIM3 ticks
#define IRRX_TIMER_SLACK 500
#define IRRX_SYM0_TICKS (IRRX_TIMER_TICKS * 1)
#define IRRX_SYM1_TICKS (IRRX_TIMER_TICKS * 2)
#define IRRX_SYM2_TICKS (IRRX_TIMER_TICKS * 4)	// Short header
#define IRRX_SYM3_TICKS (IRRX_TIMER_TICKS * 5)	// Long header

#define IR_MAX_LENGTH 40

typedef enum {
	IRRX_STATE_IDLE = 0,
	IRRX_STATE_STA0,
	IRRX_STATE_STA1,
	IRRX_STATE_MID0,
	IRRX_STATE_MID1
} irrx_state_t;

typedef struct __attribute__((__packed__)) {
	uint8_t cmd;
	uint8_t size;
	uint8_t data[IR_MAX_LENGTH - 2];
} irrx_frame_t;

extern volatile uint8_t irrx_flag;
extern volatile irrx_frame_t irrx_frame;
extern volatile uint8_t irrx_bitidx;
extern volatile uint8_t irrx_byteidx;
extern volatile irrx_state_t irrx_state;

extern volatile uint8_t irtx_busy;
extern volatile uint8_t irtx_symbolsent;
extern volatile uint32_t irtx_buffer[IR_MAX_LENGTH];	// Manchester coded bytes with header
extern volatile uint8_t irtx_get;
extern volatile uint8_t irtx_totalbytes;

extern uint8_t ir_test_passed;
extern uint8_t irrx_enabled;

void irrx_irq_handler();
void irtx_irq_handler();
void irtx_tx(uint8_t * buffer, uint8_t size);
void handle_ir_cmd(const uint8_t ir_cmd);
void irtx_beacon();
void irtx_match();
void irtx_test();

#endif /* INC_IR_H_ */
