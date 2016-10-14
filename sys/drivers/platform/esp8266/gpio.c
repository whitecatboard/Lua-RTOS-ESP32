#include "gpio.h"

#include <sys/drivers/cpu.h>

void gpio_disable_analog(int pin) {
	
}

// Configure gpio as input using a mask
// If bit n on mask is set to 1 the gpio is configured
void gpio_pin_input_mask(unsigned int port, unsigned int pinmask) {
	unsigned int mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_input(i);
		}
		
		mask = (mask << 1);
	}	
}

// Configure all gpio's port as input
void gpio_port_input(unsigned int port) {
	gpio_pin_input_mask(port, GPIO_ALL);
}

// Configure gpio as output using a mask
// If bit n on mask is set to 1 the gpio is configured
void gpio_pin_output_mask(unsigned int port, unsigned int pinmask) {
	unsigned int mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_output(i);
		}
		
		mask = (mask << 1);
	}	
}

// Configure all gpio's port as output
void gpio_port_output(unsigned int port) {
	gpio_pin_output_mask(port, GPIO_ALL);
}

// Set gpio pull-up using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
void gpio_pin_pullup_mask(unsigned int port, unsigned int pinmask) {
	unsigned int mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_pullup(i);
		}
		
		mask = (mask << 1);
	}	
}

// Set gpio pull-down using a mask
// If bit n on mask is set to 1 the gpio is set pull-up
void gpio_pin_pulldwn_mask(unsigned int port, unsigned int pinmask) {
	unsigned int mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_pulldwn(i);
		}
		
		mask = (mask << 1);
	}	
}

// Set gpio with no pull-up and no pull-down using a mask
// If bit n on mask is set to 1 the gpio with no pull-up and no pull-down
void gpio_pin_nopull_mask(unsigned int port, unsigned int pinmask) {
	unsigned int mask = 1;
	int i;
	
	for(i=0; i < GPIO_PER_PORT; i++) {
		if (pinmask & mask) {
			gpio_pin_nopull(i);
		}
		
		mask = (mask << 1);
	}	
}

// Put gpio on the high state using a mask
// If bit n on mask is set to 1 the gpio is put on the high state
void gpio_pin_set_mask(unsigned int port, unsigned int pinmask) {
	GPIO.OUT_SET = pinmask;
}

// Put port gpio's on the high state
// If bit n on mask is set to 1 the gpio is put on the high state
void gpio_port_set(unsigned int port, unsigned int mask) {
	GPIO.OUT_SET = mask;
}

// Put gpio on the low state using a mask
// If bit n on mask is set to 1 the gpio is put on the low state
void gpio_pin_clr_mask(unsigned int port, unsigned int pinmask) {
	GPIO.OUT_CLEAR = pinmask;
}

// Get gpio values using a mask
unsigned int gpio_pin_get_mask(unsigned int port, unsigned int pinmask) {
	return (GPIO.IN & pinmask);
}

// Get port gpio values
unsigned int gpio_port_get(unsigned int port) {
	return gpio_pin_get_mask(port, GPIO_ALL);
}

// Get all port gpio values
unsigned int gpio_port_get_mask(unsigned int port) {
	return gpio_pin_get_mask(port, GPIO_ALL);
}