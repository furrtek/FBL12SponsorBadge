// FBL 12 sponsor badge V1
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "init.h"
#include "audio.h"

void main_init() {
	LL_EXTI_InitTypeDef EXTI_Init;

	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();			// GPIO init must be done before other peripheral inits
	MX_USART2_UART_Init();	// DEBUG
	MX_ADC_Init();			// Mic and VBat reading
	MX_DAC_Init();			// Audio output

	MX_TSC_Init();
	MX_SPI1_Init();
	MX_SPI2_Init();
	MX_TIM2_Init();
	MX_TIM3_Init();
	MX_TIM14_Init();
	MX_TIM15_Init();
	MX_TIM16_Init();
	MX_TIM17_Init();

	EXTI_Init.LineCommand = ENABLE;
	EXTI_Init.Mode = LL_EXTI_MODE_IT;
	EXTI_Init.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	EXTI_Init.Line_0_31 = LL_EXTI_LINE_5;
	LL_EXTI_Init(&EXTI_Init);
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE5);
	NVIC_EnableIRQ(EXTI4_15_IRQn);	// IR RX pin

	NVIC_EnableIRQ(TIM2_IRQn);		// Audio timing
	NVIC_EnableIRQ(TIM3_IRQn);		// IR RX timing
	NVIC_EnableIRQ(TIM15_IRQn);		// LED scanning
	NVIC_EnableIRQ(TIM16_IRQn);		// IR TX period
	NVIC_EnableIRQ(SPI2_IRQn);		// LED updates
	NVIC_EnableIRQ(ADC1_COMP_IRQn);	// Mic sample store
	NVIC_EnableIRQ(TSC_IRQn);		// Touch sense end of conversion

	LL_TIM_EnableAllOutputs(TIM14);	// LED PWM
	LL_TIM_EnableAllOutputs(TIM16);	// IR env to TIM17
	LL_TIM_EnableAllOutputs(TIM17);	// IR carrier to IRTIM
	LL_TIM_EnableCounter(TIM2);
	LL_TIM_EnableCounter(TIM3);
	LL_TIM_EnableCounter(TIM14);
	LL_TIM_EnableCounter(TIM15);
	LL_TIM_EnableCounter(TIM17);
	LL_TIM_EnableCounter(TIM16);

	LL_ADC_Enable(ADC1);
	LL_DAC_Enable(DAC, LL_DAC_CHANNEL_1);
	LL_SPI_Enable(SPI1);
	LL_SPI_Enable(SPI2);

	HAL_TSC_IODischarge(&h_tsc, ENABLE);	// Not needed ?

	// Read MCU UID
	uint32_t word = 0;
	for (uint32_t c = 0; c < 12; c++) {
		if (!(c & 3))
			word = *(uint32_t *)(UID_BASE + c);
		uid[c] = (uint8_t)word;
		word >>= 8;
	}
}

void SystemClock_Config(void) {
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
	while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) { }
	LL_RCC_HSI14_Enable();

	while(LL_RCC_HSI14_IsReady() != 1) {}
	LL_RCC_HSI14_SetCalibTrimming(16);
	LL_RCC_HSI48_Enable();

	while(LL_RCC_HSI48_IsReady() != 1) {}
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_4);	// 48M / 4 = 12M
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);		// 12M / 1 = 12M
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI48);

	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI48) {}
	LL_SetSystemCoreClock(12000000);

	// Update the time base
	if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK) {
		Error_Handler();
	}
	LL_RCC_HSI14_EnableADCControl();
	LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);
	LL_RCC_SetUSBClockSource(LL_RCC_USB_CLKSOURCE_HSI48);
}

void MX_USART2_UART_Init(void) {
	LL_USART_InitTypeDef USART_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

	/**USART2 GPIO Configuration
	PA14   ------> USART2_TX
	PA15   ------> USART2_RX
	*/
	GPIO_InitStruct.Pin = LL_GPIO_PIN_14;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	USART_InitStruct.BaudRate = 57600;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	LL_USART_Init(USART2, &USART_InitStruct);
	LL_USART_DisableIT_CTS(USART2);
	LL_USART_ConfigAsyncMode(USART2);
	LL_USART_Enable(USART2);
}

void MX_ADC_Init(void) {
	LL_ADC_InitTypeDef ADC_InitStruct = {0};
	LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);

	// PB0   ------> ADC_IN8
	GPIO_InitStruct.Pin = AUDIO_IN_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(AUDIO_IN_GPIO_Port, &GPIO_InitStruct);

	LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_8);

	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_VBAT);

	ADC_InitStruct.Clock = LL_ADC_CLOCK_ASYNC;
	ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_8B;
	ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
	ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
	LL_ADC_Init(ADC1, &ADC_InitStruct);
	ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
	ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
	ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
	ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
	ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
	LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
	LL_ADC_REG_SetSequencerScanDirection(ADC1, LL_ADC_REG_SEQ_SCAN_DIR_FORWARD);
	LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_1CYCLE_5);
}

void MX_DAC_Init(void) {
	LL_DAC_InitTypeDef DAC_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DAC1);

	// PA4   ------> DAC_OUT1
	GPIO_InitStruct.Pin = AUDIO_OUT_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(AUDIO_OUT_GPIO_Port, &GPIO_InitStruct);

	DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
	DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
	DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_ENABLE;
	LL_DAC_Init(DAC, LL_DAC_CHANNEL_1, &DAC_InitStruct);
}

#if 0
void MX_I2C1_Init(void) {
	LL_I2C_InitTypeDef I2C_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

	// PB8   ------> I2C1_SCL
	// PB9   ------> I2C1_SDA
	GPIO_InitStruct.Pin = SCL_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(SCL_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SDA_Pin;
	LL_GPIO_Init(SDA_GPIO_Port, &GPIO_InitStruct);

	LL_I2C_DisableOwnAddress2(I2C1);
	LL_I2C_DisableGeneralCall(I2C1);
	LL_I2C_EnableClockStretching(I2C1);
	I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
	I2C_InitStruct.Timing = 0xB0420F13;	// 48M / (11 + 1) = 4M, 100kHz	TODO: 4x slower now
	I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_ENABLE;
	I2C_InitStruct.DigitalFilter = 0;
	I2C_InitStruct.OwnAddress1 = 0;
	I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
	I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
	LL_I2C_Init(I2C1, &I2C_InitStruct);
	LL_I2C_EnableAutoEndMode(I2C1);
	LL_I2C_SetOwnAddress2(I2C1, 0, LL_I2C_OWNADDRESS2_NOMASK);
	LL_I2C_Enable(I2C1);
}
#endif

void MX_SPI1_Init(void) {
	LL_SPI_InitTypeDef SPI_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SPI1);

	// PA5   ------> SPI1_SCK
	// PA6   ------> SPI1_MISO
	// PA7   ------> SPI1_MOSI
	// PA15   ------> SPI1_NSS
	GPIO_InitStruct.Pin = SCK_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(SCK_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = MISO_Pin;
	LL_GPIO_Init(MISO_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = MOSI_Pin;
	LL_GPIO_Init(MOSI_GPIO_Port, &GPIO_InitStruct);

	SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
	SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
	SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
	SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
	SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
	SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
	SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;	// 12M / 2 = 6MHz
	SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
	SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
	SPI_InitStruct.CRCPoly = 7;
	LL_SPI_Init(SPI1, &SPI_InitStruct);
	LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
}

void MX_SPI2_Init(void) {
	LL_SPI_InitTypeDef SPI_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

	// PB10   ------> SPI2_SCK
	// PB15   ------> SPI2_MOSI
	GPIO_InitStruct.Pin = SRCLK_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
	LL_GPIO_Init(SRCLK_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = DS_Pin;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(DS_GPIO_Port, &GPIO_InitStruct);

	SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
	SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
	SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_16BIT;
	SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
	SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
	SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
	SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV4;	// 12M / 4 = 3MHz
	SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
	SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
	SPI_InitStruct.CRCPoly = 7;
	LL_SPI_Init(SPI2, &SPI_InitStruct);
	LL_SPI_SetStandard(SPI2, LL_SPI_PROTOCOL_MOTOROLA);
	LL_SPI_EnableNSSPulseMgt(SPI2);
}

void MX_TIM2_Init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 1500;	// 12M / 1500 = 8kHz
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	TIM_InitStruct.RepetitionCounter = 0;
	LL_TIM_Init(TIM2, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM2);

	// OC units unused - TODO: Maybe not needed
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_FROZEN;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 0;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
	TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
	LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH4, &TIM_OC_InitStruct);

	LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_UPDATE);	// TRGO output: update event, this should trigger an ADC conversion
	LL_TIM_EnableIT_UPDATE(TIM2);
}

void MX_TIM3_Init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

	TIM_InitStruct.Prescaler = 3;	// 12M / 4 = 333ns precision
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 9500;		// IR RX timeout, 527us * 6 = 3162us = 9495 333ns ticks
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	// One 527us IR timeslot with 333ns ticks = 1583 ticks
	TIM_InitStruct.RepetitionCounter = 0;
	LL_TIM_Init(TIM3, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM3);
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_INACTIVE;	// Channel init useless for proto A ?
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 0;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_LOW;
	TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
	TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_HIGH;
	LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH1);

	LL_TIM_EnableIT_UPDATE(TIM3);	// Overflow interrupt for timeout
	// TODO: Proto B needs capture interrupt - Could this cause interrupt spamming the presence of continuous IR signal ?
}

void MX_TIM14_Init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 1000;	// 12M / 1000 = 12k
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIM14, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM14);
	LL_TIM_OC_EnablePreload(TIM14, LL_TIM_CHANNEL_CH1);
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 500;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_LOW;
	TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
	LL_TIM_OC_Init(TIM14, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM14, LL_TIM_CHANNEL_CH1);

	// PB1   ------> TIM14_CH1
	GPIO_InitStruct.Pin = nOE_LED_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	LL_GPIO_Init(nOE_LED_GPIO_Port, &GPIO_InitStruct);
}

void MX_TIM15_Init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
	LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {0};

	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM15);

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 20000;	// 12M / 20000 = 600Hz
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	TIM_InitStruct.RepetitionCounter = 0;
	LL_TIM_Init(TIM15, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM15);

	// OC units unused
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_FROZEN;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 0;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
	TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
	LL_TIM_OC_Init(TIM15, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_Init(TIM15, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM15, LL_TIM_CHANNEL_CH1);
	LL_TIM_OC_DisableFast(TIM15, LL_TIM_CHANNEL_CH2);

	// OC units unused
	TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
	TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
	TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
	TIM_BDTRInitStruct.DeadTime = 0;
	TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
	TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
	TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
	LL_TIM_BDTR_Init(TIM15, &TIM_BDTRInitStruct);

	LL_TIM_EnableIT_UPDATE(TIM15);
}

void MX_TIM16_Init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
	LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {0};

	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM16);

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 6320;	// 527us, ~20 cycles
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	TIM_InitStruct.RepetitionCounter = 0;
	LL_TIM_Init(TIM16, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM16);

	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_FORCED_ACTIVE;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 0;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_LOW;
	TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
	TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_HIGH;
	LL_TIM_OC_Init(TIM16, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM16, LL_TIM_CHANNEL_CH1);
	TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
	TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
	TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
	TIM_BDTRInitStruct.DeadTime = 0;
	TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
	TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
	TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
	LL_TIM_BDTR_Init(TIM16, &TIM_BDTRInitStruct);

	LL_TIM_EnableIT_UPDATE(TIM16);
}

void MX_TIM17_Init(void) {
	LL_TIM_InitTypeDef TIM_InitStruct = {0};
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
	LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {0};

	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM17);

	TIM_InitStruct.Prescaler = 0;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 316;	// 12M / 316 = ~38kHz
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	TIM_InitStruct.RepetitionCounter = 0;
	LL_TIM_Init(TIM17, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM17);
	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_FORCED_ACTIVE;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 316 / 4;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_LOW;
	TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
	TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_HIGH;
	LL_TIM_OC_Init(TIM17, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM17, LL_TIM_CHANNEL_CH1);
	TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
	TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
	TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
	TIM_BDTRInitStruct.DeadTime = 0;
	TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
	TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
	TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
	LL_TIM_BDTR_Init(TIM17, &TIM_BDTRInitStruct);
}

void MX_TSC_Init(void) {
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_TSC);

	h_tsc.Instance = TSC;
	h_tsc.Init.CTPulseHighLength = TSC_CTPH_2CYCLES;
	h_tsc.Init.CTPulseLowLength = TSC_CTPL_2CYCLES;
	h_tsc.Init.SpreadSpectrum = DISABLE;
	h_tsc.Init.SpreadSpectrumDeviation = 1;
	h_tsc.Init.SpreadSpectrumPrescaler = TSC_SS_PRESC_DIV1;
	h_tsc.Init.PulseGeneratorPrescaler = TSC_PG_PRESC_DIV4;
	h_tsc.Init.MaxCountValue = TSC_MCV_1023;	// DEBUG TSC_MCV_8191;
	h_tsc.Init.IODefaultMode = TSC_IODEF_OUT_PP_LOW;
	h_tsc.Init.SynchroPinPolarity = TSC_SYNC_POLARITY_FALLING;
	h_tsc.Init.AcquisitionMode = TSC_ACQ_MODE_NORMAL;
	h_tsc.Init.MaxCountInterrupt = DISABLE;
	h_tsc.Init.ShieldIOs = 0;
	h_tsc.Init.ChannelIOs = TSC_GROUP1_IO2|TSC_GROUP1_IO3|TSC_GROUP1_IO4|
		TSC_GROUP5_IO2|TSC_GROUP5_IO3|TSC_GROUP5_IO4;
	h_tsc.Init.SamplingIOs = TSC_GROUP1_IO1|TSC_GROUP5_IO1;
	if (HAL_TSC_Init(&h_tsc) != HAL_OK)
		Error_Handler();
}

void MX_USB_PCD_Init(void) {
	hpcd_USB_FS.Instance = USB;
	hpcd_USB_FS.Init.dev_endpoints = 8;
	hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
	hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
	hpcd_USB_FS.Init.low_power_enable = DISABLE;
	hpcd_USB_FS.Init.lpm_enable = DISABLE;
	hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
	if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
		Error_Handler();
}

void MX_GPIO_Init(void) {
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOF);

	Disable_SpkAmp();
	Disable_OpAmp();	// PCB B
	LL_GPIO_SetOutputPin(DIG8_GPIO_Port, DIG8_Pin);
	LL_GPIO_SetOutputPin(nPWRSD_GPIO_Port, nPWRSD_Pin);
	LL_GPIO_ResetOutputPin(RCLK_GPIO_Port, RCLK_Pin);

	// Inputs
	GPIO_InitStruct.Pin = INT_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	LL_GPIO_Init(INT_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = BTN1_Pin;
	LL_GPIO_Init(BTN1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = nCHRG_Pin;
	LL_GPIO_Init(nCHRG_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = IR_RX_Pin;
	LL_GPIO_Init(IR_RX_GPIO_Port, &GPIO_InitStruct);

	// Outputs
	GPIO_InitStruct.Pin = DIG8_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(DIG8_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = OPAMP_PWR_Pin;	// PCB B
	LL_GPIO_Init(OPAMP_PWR_GPIO_Port, &GPIO_InitStruct);	// PCB B

	GPIO_InitStruct.Pin = nPWRSD_Pin;
	LL_GPIO_Init(nPWRSD_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = AMPSD_Pin;
	LL_GPIO_Init(AMPSD_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = RCLK_Pin;
	LL_GPIO_Init(RCLK_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = DBG_Pin;
	LL_GPIO_Init(DBG_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = nFLASH_CS_Pin;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	LL_GPIO_Init(nFLASH_CS_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = IR_OUT_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(IR_OUT_GPIO_Port, &GPIO_InitStruct);

	// PB8   ------> I2C1_SCL -- not anymore !
	// PB9   ------> I2C1_SDA -- not anymore !
	GPIO_InitStruct.Pin = SCL_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(SCL_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SDA_Pin;
	LL_GPIO_Init(SDA_GPIO_Port, &GPIO_InitStruct);
}
