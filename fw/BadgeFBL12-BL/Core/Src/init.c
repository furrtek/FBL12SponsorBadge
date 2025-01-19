// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#include "main.h"
#include "init.h"
#include "audio.h"
#include "usb_device.h"

void main_init() {
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();			// GPIO init must be done before other peripheral inits
	MX_USART2_UART_Init();
	MX_DAC_Init();			// Audio output
	MX_TSC_Init();
	MX_SPI1_Init();
	MX_SPI2_Init();
	MX_TIM2_Init();
	MX_TIM14_Init();
	MX_TIM15_Init();

	NVIC_EnableIRQ(TIM2_IRQn);		// Audio timing
	NVIC_EnableIRQ(TIM15_IRQn);		// LED scanning
	NVIC_EnableIRQ(SPI2_IRQn);		// LED updates
	NVIC_EnableIRQ(ADC1_COMP_IRQn);	// Mic sample store
	NVIC_EnableIRQ(TSC_IRQn);		// Touch sense end of conversion

	LL_TIM_EnableAllOutputs(TIM14);	// LED PWM
	LL_TIM_EnableCounter(TIM2);
	LL_TIM_EnableCounter(TIM14);
	LL_TIM_EnableCounter(TIM15);

	LL_DAC_Enable(DAC, LL_DAC_CHANNEL_1);
	LL_SPI_Enable(SPI1);
	LL_SPI_Enable(SPI2);

	HAL_TSC_IODischarge(&htsc, ENABLE);	// Not needed ?

	// Read MCU UID
	uid[0] = *(uint32_t *)(UID_BASE + 0);
	uid[1] = *(uint32_t *)(UID_BASE + 4);
	uid[2] = *(uint32_t *)(UID_BASE + 8);
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
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);		// 48M / 1 = 48M
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI48);

	while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI48) {}
	LL_SetSystemCoreClock(12000000);

	// Update the time base
	if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK) {
		Error_Handler();
	}
	LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
	LL_RCC_HSI14_EnableADCControl();
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
	//LL_SPI_EnableNSSPulseMgt(SPI1);
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

void MX_TSC_Init(void) {
	htsc.Instance = TSC;
	htsc.Init.CTPulseHighLength = TSC_CTPH_2CYCLES;
	htsc.Init.CTPulseLowLength = TSC_CTPL_2CYCLES;
	htsc.Init.SpreadSpectrum = DISABLE;
	htsc.Init.SpreadSpectrumDeviation = 1;
	htsc.Init.SpreadSpectrumPrescaler = TSC_SS_PRESC_DIV1;
	htsc.Init.PulseGeneratorPrescaler = TSC_PG_PRESC_DIV4;
	htsc.Init.MaxCountValue = TSC_MCV_8191;
	htsc.Init.IODefaultMode = TSC_IODEF_OUT_PP_LOW;
	htsc.Init.SynchroPinPolarity = TSC_SYNC_POLARITY_FALLING;
	htsc.Init.AcquisitionMode = TSC_ACQ_MODE_NORMAL;
	htsc.Init.MaxCountInterrupt = DISABLE;
	htsc.Init.ShieldIOs = 0;
	htsc.Init.ChannelIOs = TSC_GROUP1_IO2|TSC_GROUP1_IO3|TSC_GROUP1_IO4|
		TSC_GROUP5_IO2|TSC_GROUP5_IO3|TSC_GROUP5_IO4;
	htsc.Init.SamplingIOs = TSC_GROUP1_IO1|TSC_GROUP5_IO1;
	if (HAL_TSC_Init(&htsc) != HAL_OK)
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

	Disable_Amp();
	LL_GPIO_SetOutputPin(DIG8_GPIO_Port, DIG8_Pin);
	LL_GPIO_SetOutputPin(nPWRSD_GPIO_Port, nPWRSD_Pin);
	LL_GPIO_SetOutputPin(OPAMP_PWR_GPIO_Port, OPAMP_PWR_Pin);	// PCB B
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
