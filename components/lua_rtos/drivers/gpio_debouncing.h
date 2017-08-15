#ifndef _GPIO_DEBOUNCING_H_
#define _GPIO_DEBOUNCING_H_

typedef void (*gpio_debouncing_callback_t)(void *, uint8_t);

typedef struct {
	uint64_t mask;  ///< Mask. If bit i = 1 on mask, GPIO(i) is debounced
	uint64_t latch; ///< Latch values

	uint16_t threshold[CPU_LAST_GPIO + 1]; ///< Threshold values, in timer period units
	uint16_t time[CPU_LAST_GPIO + 1];      ///< Time counter for GPIO

	gpio_debouncing_callback_t callback[CPU_LAST_GPIO + 1]; ///< Callback for GPIO
	void *arg[CPU_LAST_GPIO + 1]; // Callback args
} debouncing_t;

// Period for debouncing timer, in microseconds
#define GPIO_DEBOUNCING_PERIOD 10

driver_error_t *gpio_debouncing_register(uint64_t pins, uint16_t threshold, gpio_debouncing_callback_t callback, void *args);

#endif /* _GPIO_DEBOUNCING_H_ */
