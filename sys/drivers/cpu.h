#ifndef __ROOT_CPU_H__
#define __ROOT_CPU_H__

#ifdef PLATFORM_ESP8266
#include <sys/drivers/platform/esp8266/cpu.h>
#endif

#ifdef PLATFORM_ESP32
#include <sys/drivers/platform/esp32/cpu.h>
#endif

#ifdef PLATFORM_PIC32MZ
#include <sys/drivers/platform/pic32mz/cpu.h>
#endif

void _cpu_init();
int cpu_revission();
void cpu_model(char *buffer);
void cpu_reset();
void cpu_show_info();
unsigned int cpu_pins();
void cpu_assign_pin(unsigned int pin, unsigned int by);
void cpu_release_pin(unsigned int pin);
unsigned int cpu_pin_assigned(unsigned int pin);
unsigned int cpu_pin_number(unsigned int pin);
unsigned int cpu_port_number(unsigned int pin);
unsigned int cpu_port_io_pin_mask(unsigned int port);
unsigned int cpu_port_adc_pin_mask(unsigned int port);
void cpu_idle(int seconds);
const char *cpu_pin_name(unsigned int pin);
const char *cpu_port_name(int pin);
unsigned int cpu_has_gpio(unsigned int port, unsigned int pin);
unsigned int cpu_has_port(unsigned int port);
void cpu_sleep(int seconds);
int cpu_reset_reason();

#endif
