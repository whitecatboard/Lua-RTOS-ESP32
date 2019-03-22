/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, CPU driver
 *
 */

#ifndef CPU_H
#define	CPU_H

#include <stdint.h>

/*
 * ----------------------------------------------------------------
 * GPIO 
 * ----------------------------------------------------------------
 */

// ESP32 available GPIO pins
#define GPIO0	0
#define GPIO1	1
#define GPIO2	2
#define GPIO3	3
#define GPIO4	4
#define GPIO5	5
#define GPIO6	6
#define GPIO7	7
#define GPIO8	8
#define GPIO9	9
#define GPIO10	10
#define GPIO11	11
#define GPIO12	12
#define GPIO13	13
#define GPIO14	14
#define GPIO15	15
#define GPIO16	16
#define GPIO17	17
#define GPIO18	18
#define GPIO19	19
#define GPIO20	20
#define GPIO21	21
#define GPIO22	22
#define GPIO23	23
#define GPIO25	25
#define GPIO26	26
#define GPIO27	27
#define GPIO32	32
#define GPIO33	33
#define GPIO34	34
#define GPIO35	35
#define GPIO36	36
#define GPIO37	37
#define GPIO38	38
#define GPIO39	39

// ESP32 available pin names
#define GPIO0_NAME	"GPIO0"
#define GPIO1_NAME	"GPIO1"
#define GPIO2_NAME	"GPIO2"
#define GPIO3_NAME	"GPIO3"
#define GPIO4_NAME	"GPIO4"
#define GPIO5_NAME	"GPIO5"
#define GPIO6_NAME	"GPIO6"
#define GPIO7_NAME	"GPIO7"
#define GPIO8_NAME	"GPIO8"
#define GPIO9_NAME	"GPIO9"
#define GPIO10_NAME	"GPIO10"
#define GPIO11_NAME	"GPIO11"
#define GPIO12_NAME	"GPIO12"
#define GPIO13_NAME	"GPIO13"
#define GPIO14_NAME	"GPIO14"
#define GPIO15_NAME	"GPIO15"
#define GPIO16_NAME	"GPIO16"
#define GPIO17_NAME	"GPIO17"
#define GPIO18_NAME	"GPIO18"
#define GPIO19_NAME	"GPIO19"
#define GPIO20_NAME	"GPIO20"
#define GPIO21_NAME	"GPIO21"
#define GPIO22_NAME	"GPIO22"
#define GPIO23_NAME	"GPIO23"
#define GPIO25_NAME	"GPIO25"
#define GPIO26_NAME	"GPIO26"
#define GPIO27_NAME	"GPIO27"
#define GPIO32_NAME	"GPIO32"
#define GPIO33_NAME	"GPIO33"
#define GPIO34_NAME	"GPIO34"
#define GPIO35_NAME	"GPIO35"
#define GPIO36_NAME	"GPIO36"
#define GPIO37_NAME	"GPIO37"
#define GPIO38_NAME	"GPIO38"
#define GPIO39_NAME	"GPIO39"

#if EXTERNAL_GPIO
#define GPIO_PORTS (EXTERNAL_GPIO_PORTS + 1)
#else
#define GPIO_PORTS 1
#endif

// ESP32 has 40 GPIO per port
#define GPIO_PER_PORT 40

// ESP32 needs 64 bits for pin mask
typedef uint64_t gpio_pin_mask_t;

// ESP32 gpio masks
#define GPIO_ALL     0b1111111100001110111111111111111111111111ULL
#define GPIO_ALL_IN  0b1111111100001110111111111111111111111111ULL
#define GPIO_ALL_OUT 0b0000001100001110111111111111111111111111ULL

 // ESP32 port masks
#define GPIO_PORT_ALL 0b1

#define GPIO_BIT_MASK 1ULL

#define CPU_FIRST_GPIO GPIO0

#if !EXTERNAL_GPIO
#define CPU_LAST_GPIO  GPIO39
#else
#define CPU_LAST_GPIO  (GPIO39 + EXTERNAL_GPIO_PINS)
#endif

 /*
  * ----------------------------------------------------------------
  * CAN
  * ----------------------------------------------------------------
  */

 // Unit bounds
 #define CPU_FIRST_CAN 0
 #define CPU_LAST_CAN  0

 // ESP32 available CAN units
 #define CPU_CAN0  0

 // ESP32 available CAN names
 #define CPU_CAN0_NAME  "CAN0"


/*
 * ----------------------------------------------------------------
 * UART
 * ----------------------------------------------------------------
 */

// Unit bounds
#define CPU_FIRST_UART 0
#define CPU_LAST_UART  2

// ESP32 available UART units
#define CPU_UART0  0
#define CPU_UART1  1
#define CPU_UART2  2

// ESP32 available UART names
#define CPU_UART0_NAME  "UART0"
#define CPU_UART1_NAME  "UART1"
#define CPU_UART2_NAME  "UART2"

/*
 * ----------------------------------------------------------------
 * SPI
 * ----------------------------------------------------------------
 */

// Unit bounds
#define CPU_FIRST_SPI 2
#define CPU_LAST_SPI  3

// ESP32 available SPI units
#define CPU_SPI2  2
#define CPU_SPI3  3

// ESP32 available SPI names
#define CPU_SPI2_NAME  "SPI2"
#define CPU_SPI3_NAME  "SPI3"

 /*
  * ----------------------------------------------------------------
  * TIMERS
  * ----------------------------------------------------------------
  */

// Unit bounds
#define CPU_FIRST_TIMER 0
#define CPU_LAST_TIMER  3

#define CPU_TIMER_ALL 0b11

// ESP32 available timer units
#define CPU_TIMER0  0 // Group 0, timer 0
#define CPU_TIMER1  1 // Group 0, timer 1
#define CPU_TIMER2  2 // Group 1, timer 0
#define CPU_TIMER3  3 // Group 1, timer 1

// ESP32 available timer unit names
#define CPU_TIMER0_NAME "TMR0"
#define CPU_TIMER1_NAME "TMR1"
#define CPU_TIMER2_NAME "TMR2"
#define CPU_TIMER3_NAME "TMR3"

/*
 * ----------------------------------------------------------------
 * IC2 
 * ----------------------------------------------------------------
 */

// Unit bounds
#define CPU_FIRST_I2C 0
#define CPU_LAST_I2C  1

#define CPU_I2C_ALL 0b11

// ESP32 available i2c units
#define CPU_I2C0     0
#define CPU_I2C1     1

// ESP32 available i2c unit names
#define CPU_I2C0_NAME  "I2C0"
#define CPU_I2C1_NAME  "I2C1"

/*
 * ----------------------------------------------------------------
 * RMT
 * ----------------------------------------------------------------
 */

// Channel bounds
#define CPU_FIRST_RMT_CH 0
#define CPU_LAST_RMT_CH 7

/*
 * ----------------------------------------------------------------
 * ADC
 * ----------------------------------------------------------------
 */

// Unit bounds
#define CPU_FIRST_ADC 1
#define CPU_LAST_ADC  1

// Channel bounds
#define CPU_FIRST_ADC_CH 0
#define CPU_LAST_ADC_CH  7

// ESP32 available ADC units
#define CPU_ADC1     1

// ESP32 available ADC unit names
#define CPU_ADC1_NAME  "ADC1"

// ESP32 available ADC channels
#define CPU_ADC_CH0  0
#define CPU_ADC_CH1  1
#define CPU_ADC_CH2  2
#define CPU_ADC_CH3  3
#define CPU_ADC_CH4  4
#define CPU_ADC_CH5  5
#define CPU_ADC_CH6  6
#define CPU_ADC_CH7  7

// ESP32 available ADC channel names
#define CPU_ADC_CH0_NAME  "ADC_CH0"
#define CPU_ADC_CH1_NAME  "ADC_CH1"
#define CPU_ADC_CH2_NAME  "ADC_CH2"
#define CPU_ADC_CH3_NAME  "ADC_CH3"
#define CPU_ADC_CH4_NAME  "ADC_CH4"
#define CPU_ADC_CH5_NAME  "ADC_CH5"
#define CPU_ADC_CH6_NAME  "ADC_CH6"
#define CPU_ADC_CH7_NAME  "ADC_CH7"

#define CPU_ADC_ALL 0b11111111

/*
 * ----------------------------------------------------------------
 * PWM
 * ----------------------------------------------------------------
 */

// Unit bounds
#define CPU_FIRST_PWM 0
#define CPU_LAST_PWM  0

// Channel bounds
#define CPU_FIRST_PWM_CH 0
#define CPU_LAST_PWM_CH  15

// ESP32 available PWM units
#define CPU_PWM0     0

// ESP32 available PWM unit names
#define CPU_PWM0_NAME  "PWM0"

// ESP32 available PWM channels
#define CPU_PWM_CH0   0
#define CPU_PWM_CH1   1
#define CPU_PWM_CH2   2
#define CPU_PWM_CH3   3
#define CPU_PWM_CH4   4
#define CPU_PWM_CH5   5
#define CPU_PWM_CH6   6
#define CPU_PWM_CH7   7
#define CPU_PWM_CH8   8
#define CPU_PWM_CH9   9
#define CPU_PWM_CH10  10
#define CPU_PWM_CH11  11
#define CPU_PWM_CH12  12
#define CPU_PWM_CH13  13
#define CPU_PWM_CH14  14
#define CPU_PWM_CH15  15

// ESP32 available PWM channel names
#define CPU_PWM_CH0_NAME   "PWM_CH0"
#define CPU_PWM_CH1_NAME   "PWM_CH1"
#define CPU_PWM_CH2_NAME   "PWM_CH2"
#define CPU_PWM_CH3_NAME   "PWM_CH3"
#define CPU_PWM_CH4_NAME   "PWM_CH4"
#define CPU_PWM_CH5_NAME   "PWM_CH5"
#define CPU_PWM_CH6_NAME   "PWM_CH6"
#define CPU_PWM_CH7_NAME   "PWM_CH7"
#define CPU_PWM_CH8_NAME   "PWM_CH8"
#define CPU_PWM_CH9_NAME   "PWM_CH9"
#define CPU_PWM_CH10_NAME  "PWM_CH10"
#define CPU_PWM_CH11_NAME  "PWM_CH11"
#define CPU_PWM_CH12_NAME  "PWM_CH12"
#define CPU_PWM_CH13_NAME  "PWM_CH13"
#define CPU_PWM_CH14_NAME  "PWM_CH14"
#define CPU_PWM_CH15_NAME  "PWM_CH15"

#define CPU_PWM0_ALL 0b1111111111111111

#endif

void _cpu_init();
int cpu_revision();
void cpu_model(char *buffer, int buflen);
void cpu_reset();
void cpu_show_info();
unsigned int cpu_pins();
void cpu_assign_pin(unsigned int pin, unsigned int by);
void cpu_release_pin(unsigned int pin);
unsigned int cpu_pin_assigned(unsigned int pin);
unsigned int cpu_pin_number(unsigned int pin);
unsigned int cpu_port_number(unsigned int pin);
gpio_pin_mask_t cpu_port_io_pin_mask(unsigned int port);
unsigned int cpu_port_adc_pin_mask(unsigned int port);
void cpu_idle(int seconds);
unsigned int cpu_has_gpio(unsigned int port, unsigned int pin);
unsigned int cpu_has_port(unsigned int port);
void cpu_sleep(int seconds);
void cpu_deepsleep();
int cpu_reset_reason();
int cpu_wakeup_reason();
uint8_t cpu_gpio_number(uint8_t pin);
void cpu_show_flash_info();
uint32_t cpu_speed() __attribute__((deprecated));
uint32_t cpu_speed_mhz();
uint32_t cpu_speed_hz();
