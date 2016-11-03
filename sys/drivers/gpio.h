#ifndef __ROOT_GPIO_H__
#define __ROOT_GPIO_H__

void gpio_pin_input_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_output_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_set_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_clr_mask(unsigned int port, unsigned int pinmask);
unsigned int gpio_pin_get_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_pullup_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_pulldwn_mask(unsigned int port, unsigned int pinmask);
void gpio_pin_nopull_mask(unsigned int port, unsigned int pinmask);
void gpio_port_input(unsigned int port);
void gpio_port_output(unsigned int port);
void gpio_port_set(unsigned int port, unsigned int mask);
unsigned int gpio_port_get(unsigned int port);
char gpio_portname(int pin);
unsigned int cpu_gpio_number(unsigned int pin);
int gpio_pinno(int pin);
void gpio_disable_analog(int pin);

#ifdef PLATFORM_ESP8266
#include <sys/drivers/platform/esp8266/gpio.h>
#endif

#ifdef PLATFORM_ESP32
//#include <sys/drivers/platform/esp32/gpio.h>
#endif

#ifdef PLATFORM_PIC32MZ
#include <sys/drivers/platform/pic32mz/gpio.h>
#endif

#endif
