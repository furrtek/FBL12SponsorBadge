// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include <string.h>
#include "main.h"
#include "ir.h"
#include "audio.h"
#include "user_data.h"
#include "touch.h"
#include "display.h"
#include "luts.h"
#include "testmode.h"
#include "misc.h"

volatile uint8_t irrx_flag = 0;
static uint8_t irrx_buffer[IR_MAX_LENGTH] = { 0 };
volatile irrx_frame_t irrx_frame = { 0 };
volatile uint8_t irrx_bitidx;
volatile uint8_t irrx_byteidx;
volatile irrx_state_t irrx_state = IRRX_STATE_IDLE;
uint8_t irrx_byte;
uint8_t ir_test_passed = 0;
uint8_t irrx_enabled = 1;

volatile uint8_t irtx_busy = 0;
volatile uint8_t irtx_symbolsent = 0;
volatile uint32_t irtx_buffer[IR_MAX_LENGTH];	// Manchester coded bytes with header
volatile uint8_t irtx_get = 0;
volatile uint8_t irtx_totalbytes = 0;

void irrx_irq_handler() {
	uint16_t period;
	uint8_t symbol;
	uint8_t byte;

	// IR RX state change interrupt
	// Proto A: Pin change interrupt, use TIM3 manually to time periods
	// Proto B: Input capture TIM3 interrupt ?
	// Use TIM3 overflow interrupt to reset IR RX state on timeout (IRRX_STATE_IDLE, variables...)

	// Decode:
	// Timer capture on both edges
	// First edge: ignore
	// Second edge: if 4: state=sta0
	//				if 5: state=write 1, mid1
	//     0123 45 67 8
	// ''''____ '_ '_ ' = Start, 0, 0 (sta0, 0 mid0, sta0, 0 mid0, sta0)
	// ''''____ '_ _' ' = Start, 0, 1 (sta0, 0 mid0, 1 mid1)
	// ''''____ _' '_ ' = Start, 1, 0 (1 mid1, 0 mid0, sta0)
	// ''''____ _' _' ' = Start, 1, 1 (1 mid1, sta1, 1 mid1)

	// Other edges:
	// mid0: middle of 10
	// mid1: middle of 01
	// State Prev New 0=small 1=large
	// sta0    x    0   mid0, output 0
	// sta0    x    1   ignore
	// sta1    x    0   mid1, output 1
	// sta1    x    1   ignore
	// mid0    x    0   sta0
	// mid0    x    1   mid1, output 1
	// mid1    x    0   sta1
	// mid1    x    1   mid0, output 0
	// See manchester decoding state machine

	if (!irrx_enabled || irtx_busy)
		return;

	period = LL_TIM_GetCounter(TIM3);
	LL_TIM_SetCounter(TIM3, 0);			// Reset ASAP

	if ((period > IRRX_SYM0_TICKS - IRRX_TIMER_SLACK) && (period < IRRX_SYM0_TICKS + IRRX_TIMER_SLACK))
		symbol = 0;	// Small
	else if ((period > IRRX_SYM1_TICKS - IRRX_TIMER_SLACK) && (period < IRRX_SYM1_TICKS + IRRX_TIMER_SLACK))
		symbol = 1;	// Large
	else if ((period > IRRX_SYM2_TICKS - IRRX_TIMER_SLACK) && (period < IRRX_SYM2_TICKS + IRRX_TIMER_SLACK))
		symbol = 2;	// Short header
	else if ((period > IRRX_SYM3_TICKS - IRRX_TIMER_SLACK) && (period < IRRX_SYM3_TICKS + IRRX_TIMER_SLACK))
		symbol = 3;	// Long header
	else
		symbol = 4;	// Invalid - TODO: Abort rx

	//if (irrx_state == IRRX_STATE_IDLE) {
		if (symbol == 2) {
			// Short header
			irrx_state = IRRX_STATE_STA0;
			irrx_bitidx = 0;
		} else if (symbol == 3) {
			// Long header
			irrx_state = IRRX_STATE_MID1;
			irrx_byte <<= 1;	// Output 0
			irrx_byte |= 1;
			irrx_bitidx = 1;
		}
	//} else {
		// Didn't time out
		if (irrx_state == IRRX_STATE_STA0) {
			if (symbol == 0) {
				irrx_state = IRRX_STATE_MID0;
				irrx_byte <<= 1;	// Output 0
				irrx_bitidx++;
			} else if (symbol == 1) {
				// Shouldn't happen
			}
		} else if (irrx_state == IRRX_STATE_STA1) {
			if (symbol == 0) {
				irrx_state = IRRX_STATE_MID1;
				irrx_byte <<= 1;	// Output 1
				irrx_byte |= 1;
				irrx_bitidx++;
			} else if (symbol == 1) {
				// Shouldn't happen
			}
		} else if (irrx_state == IRRX_STATE_MID0) {
			if (symbol == 0) {
				irrx_state = IRRX_STATE_STA0;
			} else if (symbol == 1) {
				irrx_state = IRRX_STATE_MID1;
				irrx_byte <<= 1;	// Output 1
				irrx_byte |= 1;
				irrx_bitidx++;
			}
		} else if (irrx_state == IRRX_STATE_MID1) {
			if (symbol == 0) {
				irrx_state = IRRX_STATE_STA1;
			} else if (symbol == 1) {
				irrx_state = IRRX_STATE_MID0;
				irrx_byte <<= 1;	// Output 0
				irrx_bitidx++;
			}
		}

		if ((irrx_bitidx == 8) && (irrx_byteidx < IR_MAX_LENGTH)) {
			// Push new byte to rx buffer
			irrx_bitidx = 0;
			irrx_buffer[irrx_byteidx] = irrx_byte ^ 0xFF;	 // Output of IR RX is inverted

			if ((irrx_byteidx >= 2) && (irrx_buffer[1] == irrx_byteidx - 2)) {
				// End of frame, check checksum

				uint8_t sum = 0;
				for (byte = 0; byte < irrx_byteidx; byte++)
					sum += irrx_buffer[byte];
				if (irrx_buffer[irrx_byteidx] == sum) {
					memcpy((void*)&irrx_frame, irrx_buffer, irrx_byteidx);
					irrx_flag = 1;	// Mark new frame received and checked ok
					irrx_state = IRRX_STATE_IDLE;
				}
			} else
				irrx_byteidx++;
		}
	//}
}

void irtx_irq_handler() {
	uint8_t bit;

	// IR symbol timebase interrupt
	if (irtx_busy && (irtx_symbolsent < 20)) {
		bit = (uint8_t)((irtx_buffer[irtx_get] >> irtx_symbolsent) & 1);
		if (bit) {
			// Force active level (IRTIM NAND output: PWM)
			LL_TIM_OC_SetMode(TIM17, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_PWM1);
		} else {
			// Force inactive level (IRTIM NAND output: 0)
			LL_TIM_OC_SetMode(TIM17, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_FORCED_ACTIVE);
		}
		irtx_symbolsent++;
	} else {
		// Byte sent
		// Force inactive level (IRTIM NAND output: 0)
		LL_TIM_OC_SetMode(TIM17, LL_TIM_CHANNEL_CH1, LL_TIM_OCMODE_FORCED_ACTIVE);
		irtx_symbolsent = 0;

		irtx_get++;
		if (irtx_get >= irtx_totalbytes) {
			// Done sending all bytes
			irtx_busy = 0;
		}
	}
}

static uint32_t manchester_encode(const uint8_t value) {
	uint32_t codedmsg;

	// Manchester encode
	codedmsg = 0;
	for (uint8_t bit = 0; bit < 8; bit++) {
		codedmsg <<= 2;
		if ((value >> bit) & 1)
			codedmsg |= 0b01;	// Rising edge for 1
		else
			codedmsg |= 0b10;	// Falling edge for 0
	}

	// Header
	codedmsg <<= 4;
	codedmsg |= 0b1111;

	return codedmsg;
}

// buffer should NOT include parameter count byte, only command and parameters
void irtx_tx(uint8_t * buffer, uint8_t param_count) {
	uint8_t sum = buffer[0];
	uint8_t byte;

	while (irtx_busy) {};	// Wait for last transmit done

	irtx_buffer[0] = manchester_encode(buffer[0]);		// Command
	irtx_buffer[1] = manchester_encode(param_count);	// Parameters count

	// 0011223344556677SSSS (output direction <-)
	// With 527us symbols, 10.6ms per byte, ~94Bps
	for (byte = 1; byte < param_count + 1; byte++) {
		sum += buffer[byte];
		irtx_buffer[byte + 1] = manchester_encode(buffer[byte]);
	}
	irtx_buffer[byte + 1] = sum;	// Append checksum

	irtx_totalbytes = param_count + 2;	// With command byte and checksum byte
	irtx_get = 0;
	irtx_symbolsent = 0;
	irtx_busy = 1;
}

void handle_ir_cmd(const uint8_t ir_cmd) {
	if (ir_cmd == IR_CMD_TESTMODE) {
		// Priority above everything
		if ((irrx_frame.size == 2) && (irrx_frame.data[0] == 0x37))
			enter_testmode(irrx_frame.data[1]);
	} else if (ir_cmd == IR_CMD_TEST) {
		if (irrx_frame.size == 12) {
			if (!memcmp((void*)&irrx_frame.data, uid, 12u))
				ir_test_passed = 1;
		}
	} else if ((ir_cmd == IR_CMD_SETID) && (touched[TOUCH_NOSE])) {
		if (irrx_frame.size == 2) {
			//DBGPIN_TOGGLE	// DEBUG
			enter_temp(20);	// 1s
			beep(melody_prog);
			set_id(*((uint16_t*)&irrx_frame.data));
			irrx_enabled = 0;
			ClearDisp();
			PrintNum(user_data_ptr->user_id);
		}
	} else if ((ir_cmd == IR_CMD_SETNAME) && (touched[TOUCH_NOSE])) {
		if (irrx_frame.size <= 16) {
			enter_temp(20);	// 1s
			beep(melody_prog);
			set_name(irrx_frame.size, (char*)&irrx_frame.data);
			irrx_enabled = 0;
			ClearDisp();
			PrintTxt(0, user_data_ptr->name);
		}
	} else if ((ir_cmd == IR_CMD_SETSPECIES) && (touched[TOUCH_NOSE])) {
		if (irrx_frame.size <= (1 + 16)) {
			if (irrx_frame.data[0] < 2) {
				enter_temp(20);	// 1s
				beep(melody_prog);
				irrx_frame.data[1 + irrx_frame.size - 1] = 0;	// Force null termination
				set_species(irrx_frame.data[0], &irrx_frame.data[1]);
				irrx_enabled = 0;
				ClearDisp();
				PrintTxt(0, user_data_ptr->species_a);	// Debug
			}
		}
	} else if (ir_cmd == IR_CMD_BEACON) {
		// Received a beacon frame
		/*if (user_data_valid()) {
			// Check user ID fav match
			if ((irrx_frame[2] == user_data_ptr->user_id >> 8) &&
				(irrx_frame[3] == user_data_ptr->user_id)) {
				// Fav match has priority above species match
				prev_state = state;
				state = STATE_FAV;	// Temp state
			} else {
				// Check species match
				if ((irrx_frame[4] == user_data_ptr->species_code_a) ||
					(irrx_frame[5] == user_data_ptr->species_code_b)) {
					if (state == STATE_IDLE) {
						// Immediately send a species match frame
						irtx_match();
						match_timer = COMM_TIMEOUT;	// Window to receive back a match frame
					}
				}
			}
		}*/
		// Simplified for demo purposes
		irrx_enabled = 0;
		enter_alert();
		beep(melody_alert);
		irtx_match();
	} else if (ir_cmd == IR_CMD_MATCH) {
		/*if (user_data_valid()) {
			// Check species match
			if ((irrx_frame[2] == user_data_ptr->species_code_a) ||
				(irrx_frame[3] == user_data_ptr->species_code_b)) {
				if (match_timer) {
					// Didn't time out
					if (state == STATE_IDLE)
						state = STATE_MATCH;
				}
			}
		}*/
		// Simplified for demo purposes
		irrx_enabled = 0;
		enter_alert();
		beep(melody_alert);
	}
}

void irtx_beacon() {
	uint16_t checksum;
	uint8_t temp[7];

	temp[0] = IR_CMD_BEACON;
	temp[1] = user_data_ptr->user_id >> 8;
	temp[2] = (uint8_t)user_data_ptr->user_id;

	checksum = BSDChecksum(user_data_ptr->species_a);
	temp[3] = checksum >> 8;
	temp[4] = (uint8_t)checksum;

	checksum = BSDChecksum(user_data_ptr->species_b);
	temp[5] = checksum >> 8;
	temp[6] = (uint8_t)checksum;

	irtx_tx(temp, 6);
}

void irtx_match() {
	uint16_t checksum;
	uint8_t temp[5];

	temp[0] = IR_CMD_MATCH;

	checksum = BSDChecksum(user_data_ptr->species_a);
	temp[1] = checksum >> 8;
	temp[2] = (uint8_t)checksum;

	checksum = BSDChecksum(user_data_ptr->species_b);
	temp[3] = checksum >> 8;
	temp[4] = (uint8_t)checksum;

	irtx_tx(temp, 4);
}

void irtx_test() {
	// Transmit MCU UID
	uint8_t temp[13];
	temp[0] = IR_CMD_TEST;
	memcpy(&temp[1], uid, 12);
	irtx_tx(temp, 12);
}
