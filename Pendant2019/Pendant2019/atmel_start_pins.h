/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */
#ifndef ATMEL_START_PINS_H_INCLUDED
#define ATMEL_START_PINS_H_INCLUDED

#include <hal_gpio.h>

// SAMD51 has 14 pin functions

#define GPIO_PIN_FUNCTION_A 0
#define GPIO_PIN_FUNCTION_B 1
#define GPIO_PIN_FUNCTION_C 2
#define GPIO_PIN_FUNCTION_D 3
#define GPIO_PIN_FUNCTION_E 4
#define GPIO_PIN_FUNCTION_F 5
#define GPIO_PIN_FUNCTION_G 6
#define GPIO_PIN_FUNCTION_H 7
#define GPIO_PIN_FUNCTION_I 8
#define GPIO_PIN_FUNCTION_J 9
#define GPIO_PIN_FUNCTION_K 10
#define GPIO_PIN_FUNCTION_L 11
#define GPIO_PIN_FUNCTION_M 12
#define GPIO_PIN_FUNCTION_N 13

#define ENABLE_E GPIO(GPIO_PORTA, 0)
#define ENABLE_O GPIO(GPIO_PORTA, 1)
#define SX1280_RESET GPIO(GPIO_PORTA, 3)
#define SX1280_MOSI GPIO(GPIO_PORTA, 4)
#define SX1280_SCLK GPIO(GPIO_PORTA, 5)
#define SX1280_SSEL GPIO(GPIO_PORTA, 6)
#define SX1280_MISO GPIO(GPIO_PORTA, 7)
#define DAT_E_U GPIO(GPIO_PORTA, 8)
#define DAT_O_U GPIO(GPIO_PORTA, 9)
#define DAT_E_V GPIO(GPIO_PORTA, 10)
#define DAT_O_V GPIO(GPIO_PORTA, 11)
#define I2C_SCL GPIO(GPIO_PORTA, 12)
#define I2C_SDA GPIO(GPIO_PORTA, 13)
#define BQ_INT GPIO(GPIO_PORTA, 16)
#define SW_2_UPPER GPIO(GPIO_PORTA, 17)
#define SW_2_LOWER GPIO(GPIO_PORTA, 18)
#define SW_1_LOWER GPIO(GPIO_PORTA, 19)
#define UART_TX GPIO(GPIO_PORTA, 22)
#define UART_RX GPIO(GPIO_PORTA, 23)
#define USB_DATD GPIO(GPIO_PORTA, 24)
#define USB_DATP GPIO(GPIO_PORTA, 25)
#define SX1280_BUSY GPIO(GPIO_PORTB, 8)
#define SX1280_DIO1 GPIO(GPIO_PORTB, 9)
#define OLED_RESE GPIO(GPIO_PORTB, 11)

#endif // ATMEL_START_PINS_H_INCLUDED
