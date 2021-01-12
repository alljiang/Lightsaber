

#ifndef PINCONFIG_H_
#define PINCONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"

// ***** Power *****

#define PowerON_b GPIO_PORTA_BASE
#define PowerON_p GPIO_PIN_0

#define Vsense_b GPIO_PORTE_BASE
#define Vsense_p GPIO_PIN_1

#define Button_Sense_b GPIO_PORTE_BASE
#define Button_Sense_p GPIO_PIN_0

#define Charge_Bias_b GPIO_PORTE_BASE
#define Charge_Bias_p GPIO_PIN_7

#define Not_Charging_b GPIO_PORTC_BASE
#define Not_Charging_p GPIO_PIN_7

// *****************

// ****** ESP ******

#define ESP_IO0_b GPIO_PORTA_BASE
#define ESP_IO0_p GPIO_PIN_1

#define ESP_IO2_b GPIO_PORTF_BASE
#define ESP_IO2_p GPIO_PIN_2

#define ESP_RX_b GPIO_PORTC_BASE
#define ESP_RX_p GPIO_PIN_5

#define ESP_TX_b GPIO_PORTC_BASE
#define ESP_TX_p GPIO_PIN_4

#define ESP_EN_b GPIO_PORTF_BASE
#define ESP_EN_p GPIO_PIN_3

#define ESP_RST_b GPIO_PORTC_BASE
#define ESP_RST_p GPIO_PIN_6

// *****************

// ****** SPI ******

#define CS_SD_b GPIO_PORTA_BASE
#define CS_SD_p GPIO_PIN_3

#define CS_DAC_b GPIO_PORTE_BASE
#define CS_DAC_p GPIO_PIN_4

#define MISO_b GPIO_PORTA_BASE
#define MISO_p GPIO_PIN_4

#define MOSI_b GPIO_PORTA_BASE
#define MOSI_p GPIO_PIN_5

#define SCK_b GPIO_PORTA_BASE
#define SCK_p GPIO_PIN_2

// *****************

// ****** I2C ******

#define SCL_b GPIO_PORTA_BASE
#define SCL_p GPIO_PIN_6

#define SDA_b GPIO_PORTA_BASE
#define SDA_p GPIO_PIN_7

// *****************
// ****** LED ******

#define Neopixel_b GPIO_PORTF_BASE
#define Neopixel_p GPIO_PIN_0

#define LED_R_b GPIO_PORTD_BASE // PWM 6, Module 0 Generator 3
#define LED_R_p GPIO_PIN_0

#define LED_G_b GPIO_PORTE_BASE // PWM 5, Module 0 Generator 2
#define LED_G_p GPIO_PIN_5

#define LED_Anode_b GPIO_PORTB_BASE // PWM 3, Module 0 Generator 1
#define LED_Anode_p GPIO_PIN_5

// *****************

#endif /* PINCONFIG_H_ */
