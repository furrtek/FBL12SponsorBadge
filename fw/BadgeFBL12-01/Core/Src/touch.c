// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "touch.h"
#include "luts.h"

#define SCL_LOW LL_GPIO_ResetOutputPin(SCL_GPIO_Port, SCL_Pin);
#define SCL_HIGH LL_GPIO_SetOutputPin(SCL_GPIO_Port, SCL_Pin);

#define SDA_LOW LL_GPIO_ResetOutputPin(SDA_GPIO_Port, SDA_Pin);
#define SDA_HIGH LL_GPIO_SetOutputPin(SDA_GPIO_Port, SDA_Pin);

// PCB B MCU touch					touch_val	testmode	Normal	Touched	Mid
// TOUCH12 G1 IO1 Left btn top A	[0]			0			316     170		243
// TOUCH13 G1 IO2 Left ear			[4]			4			171     100		135	key4
// TOUCH14 G1 IO3 Right ear			[8]			8			191		102		146 key8
// TOUCH19 G5 IO1 Right button OK	[1]			1			621		310		465
// TOUCH20 G5 IO2 Nose				[5]			5			400		220 	310	key5
// TOUCH21 G5 IO3 Left btn bot B	[9]			9			370		210		290	key9

volatile uint16_t touch_val[12] = { 0 };	// (io << 2) + group
uint8_t touched[12] = { 0 };
uint8_t touched_prev[12] = { 0 };
uint8_t touched_rise[12] = { 0 };

// These values depend on sense caps and touch pad ares, they are found by testing (test mode 1)
const uint16_t touch_thr[12] = {	// (io << 2) + group
	243, 465, 0, 0,
	135, 310, 0, 0,
	146, 290, 0, 0
};

uint16_t ReadExtTouch() {
	uint8_t c;
	uint16_t data = (0x42 << 1) | 1;	// Read address 0x42

	// Can't use the I2C peripheral with the XW12A because it returns a 16bit word
	// Only 8bit transfers are supported on the STM32F072 >:(
	// Whatever, just bit-bang the thing, it's simple enough

	// XW12A address: 0x42
	// Read: 0x42 << 1 | 1, should get ack, then read 16 bits without acks, nack on 17th bit
	// Pad bits active low: 0123456789ABxxxx

	// Prereq: Have SDA and SCL as open-collector outputs

	// Shift out (0x42 << 1) | 1, and a final 1 for the slave ack (should be set low)
	LL_GPIO_SetPinMode(SDA_GPIO_Port, SDA_Pin, LL_GPIO_MODE_OUTPUT);
	SDA_LOW	// SDA drops while SCL high: start condition
	ShortWait(2);
	for (c = 0; c < 9; c++) {
		SCL_LOW
		if (data & 0x80)
			SDA_HIGH
		else
			SDA_LOW
		ShortWait(2);
		data <<= 1;
		data |= 1;
		SCL_HIGH
		ShortWait(2);
	}
	// TODO: Check that SDA is low (ack from slave) here

	// Switch SDA to input
	LL_GPIO_SetPinMode(SDA_GPIO_Port, SDA_Pin, LL_GPIO_MODE_INPUT);

	// Read 16 bits, last 4 are ignored
	data = 0;
	for (c = 0; c < 16; c++) {
		SCL_LOW
		ShortWait(2);
		if (!LL_GPIO_IsInputPinSet(SDA_GPIO_Port, SDA_Pin))
			data |= key_lut[c];	// Remap bits
		SCL_HIGH
		ShortWait(2);
	}

	// NACK bit
	LL_GPIO_SetPinMode(SDA_GPIO_Port, SDA_Pin, LL_GPIO_MODE_OUTPUT);
	SDA_HIGH
	SCL_LOW
	ShortWait(2);
	SCL_HIGH
	ShortWait(2);

	// Stop
	SDA_LOW
	SCL_LOW
	ShortWait(2);
	SCL_HIGH
	ShortWait(2);
	SDA_HIGH	// SDA rises while SCL high: stop condition

	return data;
}
