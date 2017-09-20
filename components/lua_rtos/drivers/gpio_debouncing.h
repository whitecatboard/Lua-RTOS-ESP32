#ifndef _GPIO_DEBOUNCING_H_
#define _GPIO_DEBOUNCING_H_

#include <sys/mutex.h>

typedef void (*gpio_debouncing_callback_t)(void *, uint8_t);

typedef struct {
	uint64_t mask;  	///< Mask. If bit i = 1 on mask, internal GPIO(i) is debounced
	uint64_t latch; 	///< Internal latch values

#if EXTERNAL_GPIO
	uint64_t mask_ext;  ///< Mask. If bit i = 1 on mask, external GPIO(i) is debounced
	uint64_t latch_ext; ///< External latch values
#endif

	uint16_t threshold[CPU_LAST_GPIO + 1]; ///< Threshold values, in timer period units
	uint16_t time[CPU_LAST_GPIO + 1];      ///< Time counter for GPIO

	gpio_debouncing_callback_t callback[CPU_LAST_GPIO + 1]; ///< Callback for GPIO
	void *arg[CPU_LAST_GPIO + 1]; // Callback args
	struct mtx mtx;
} debouncing_t;

// Period for debouncing timer, in microseconds
#define GPIO_DEBOUNCING_PERIOD 20

driver_error_t *gpio_debouncing_register(uint8_t pin, uint16_t threshold, gpio_debouncing_callback_t callback, void *args);
driver_error_t *gpio_debouncing_unregister(uint8_t pin);
void gpio_debouncing_force_isr(void *args);

#endif /* _GPIO_DEBOUNCING_H_ */
