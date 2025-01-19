// FBL 12 sponsor badge bootloader
// For STM32F072CBT6
// furrtek 2024

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

// Pin definitions
#define INT_Pin LL_GPIO_PIN_13
#define INT_GPIO_Port GPIOC
#define DIG8_Pin LL_GPIO_PIN_14
#define DIG8_GPIO_Port GPIOC
#define nPWRSD_Pin LL_GPIO_PIN_15
#define nPWRSD_GPIO_Port GPIOC
#define AMPSD_Pin LL_GPIO_PIN_0
#define AMPSD_GPIO_Port GPIOF
#define BTN1_Pin LL_GPIO_PIN_1
#define BTN1_GPIO_Port GPIOF
#define TSC_G1_SAMP_Pin LL_GPIO_PIN_0
#define TSC_G1_SAMP_GPIO_Port GPIOA
#define TOUCH12_Pin LL_GPIO_PIN_1
#define TOUCH12_GPIO_Port GPIOA
#define TOUCH13_Pin LL_GPIO_PIN_2
#define TOUCH13_GPIO_Port GPIOA
#define TOUCH14_Pin LL_GPIO_PIN_3
#define TOUCH14_GPIO_Port GPIOA
#define AUDIO_OUT_Pin LL_GPIO_PIN_4
#define AUDIO_OUT_GPIO_Port GPIOA
#define SCK_Pin LL_GPIO_PIN_5
#define SCK_GPIO_Port GPIOA
#define MISO_Pin LL_GPIO_PIN_6
#define MISO_GPIO_Port GPIOA
#define MOSI_Pin LL_GPIO_PIN_7
#define MOSI_GPIO_Port GPIOA
#define AUDIO_IN_Pin LL_GPIO_PIN_0
#define AUDIO_IN_GPIO_Port GPIOB
#define nOE_LED_Pin LL_GPIO_PIN_1
#define nOE_LED_GPIO_Port GPIOB
#define RCLK_Pin LL_GPIO_PIN_2
#define RCLK_GPIO_Port GPIOB
#define SRCLK_Pin LL_GPIO_PIN_10
#define SRCLK_GPIO_Port GPIOB
#define DS_Pin LL_GPIO_PIN_15
#define DS_GPIO_Port GPIOB
#define nCHRG_Pin LL_GPIO_PIN_8
#define nCHRG_GPIO_Port GPIOA
#define IR_OUT_Pin LL_GPIO_PIN_13
#define IR_OUT_GPIO_Port GPIOA
#define TSC_G4_SAMP_Pin LL_GPIO_PIN_3
#define TSC_G4_SAMP_GPIO_Port GPIOB
#define TOUCH19_Pin LL_GPIO_PIN_4
#define TOUCH19_GPIO_Port GPIOB
#define TOUCH20_Pin LL_GPIO_PIN_6
#define TOUCH20_GPIO_Port GPIOB
#define TOUCH21_Pin LL_GPIO_PIN_7
#define TOUCH21_GPIO_Port GPIOB
#define SCL_Pin LL_GPIO_PIN_8
#define SCL_GPIO_Port GPIOB
#define SDA_Pin LL_GPIO_PIN_9
#define SDA_GPIO_Port GPIOB
#define IR_RX_Pin LL_GPIO_PIN_5
#define IR_RX_GPIO_Port GPIOB
#define OPAMP_PWR_Pin LL_GPIO_PIN_9
#define OPAMP_PWR_GPIO_Port GPIOA
#define nFLASH_CS_Pin LL_GPIO_PIN_1
#define nFLASH_CS_GPIO_Port GPIOF
#define DBG_Pin LL_GPIO_PIN_15
#define DBG_GPIO_Port GPIOA

// Debug output pin
#define DBGPIN_RESET LL_GPIO_ResetOutputPin(DBG_GPIO_Port, DBG_Pin);
#define DBGPIN_SET LL_GPIO_SetOutputPin(DBG_GPIO_Port, DBG_Pin);
#define DBGPIN_TOGGLE LL_GPIO_TogglePin(DBG_GPIO_Port, DBG_Pin);

#endif /* INC_GPIO_H_ */
