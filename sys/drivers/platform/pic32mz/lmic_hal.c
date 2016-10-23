#include "FreeRTOS.h"

#include <stdio.h>

#include <sys/delay.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/spi.h>
#include <sys/drivers/lmic/oslmic.h>

#include <sys/syslog.h>

// This variable stores the current value for DIO pins. The nth DIO pin
// value is stored in the nth bit of this variable.
static char dio_states = 0;
static int nested = 0;

void os_resume_nunloop();

void hal_check_dio() {
	if (LMIC_DIO0) {
		// Check for DIO0 changes
        if ((dio_states & (1 << 0)) != gpio_pin_get(LMIC_DIO0)) {
			// Has changed, so store the new value, inverting its bit
			dio_states ^= 1 << 0;

            if (dio_states & (1 << 0)) {
				hal_disableIRQs();
                radio_irq_handler(0);
				hal_enableIRQs();
			}
        }
	}	

	if (LMIC_DIO1) {
		// Check for DIO1 changes
        if ((dio_states & (1 << 1)) != gpio_pin_get(LMIC_DIO1)) {
			// Has changed, so store the new value, inverting its bit
			dio_states ^= 1 << 1;
			
            if (dio_states & (1 << 1)) {
				hal_disableIRQs();
                radio_irq_handler(1);
				hal_enableIRQs();
			}
        }
	}	

	if (LMIC_DIO2) {
		// Check for DIO2 changes
        if ((dio_states & (1 << 2)) != gpio_pin_get(LMIC_DIO2)) {
			// Has changed, so store the new value, inverting its bit
			dio_states ^= 1 << 2;
			
            if (dio_states & (1 << 2)) {
				hal_disableIRQs();
                radio_irq_handler(1);
				hal_enableIRQs();
			}
		}
	}	
	
	//os_resume_nunloop();
}

void hal_init (void) {
	// Init SPI bus
    if (spi_init(LMIC_SPI) != 0) {
        syslog(LOG_ERR, "lmic cannot open spi%u port", LMIC_SPI);
        return;
    }
    
    spi_set_cspin(LMIC_SPI, LMIC_CS);
    spi_set_speed(LMIC_SPI, LMIC_SPI_KHZ);
    spi_set(LMIC_SPI, PIC32_SPICON_CKE);

    if (spi_cspin(LMIC_SPI) >= 0) {
        syslog(LOG_INFO, "lmic is at port %s, pin cs=%c%d",
            spi_name(LMIC_SPI), spi_csname(LMIC_SPI), spi_cspin(LMIC_SPI));
    }
	
	// Init RESET pin
	gpio_pin_output(LMIC_RST);
	
	// Init DIO pins
	if (LMIC_DIO0)	
		gpio_pin_input(LMIC_DIO0);
	
	if (LMIC_DIO1)
		gpio_pin_input(LMIC_DIO1);
	
	if (LMIC_DIO2)
		gpio_pin_input(LMIC_DIO2);
}

/*
 * drive radio NSS pin (0=low, 1=high).
 */
void hal_pin_nss (u1_t val) {
	if (!val) {
		spi_select(LMIC_SPI);
	} else {
		spi_deselect(LMIC_SPI);	
	}
}

/*
 * drive radio RX/TX pins (0=rx, 1=tx).
 */
void hal_pin_rxtx (u1_t val) {
	
}

/*
 * control radio RST pin (0=low, 1=high, 2=floating)
 */
void hal_pin_rst (u1_t val) {
	if (val == 1) {
		gpio_pin_output(LMIC_RST);
		gpio_pin_set(LMIC_RST);
	} else if (val == 0) {
		gpio_pin_output(LMIC_RST);
		gpio_pin_clr(LMIC_RST);
	} else {
		gpio_pin_input(LMIC_RST);
	}	
}

/*
 * perform 8-bit SPI transaction with radio.
 *   - write given byte 'outval'
 *   - read byte and return value
 */
u1_t hal_spi (u1_t outval) {
	return spi_transfer(LMIC_SPI, outval);
}

void hal_disableIRQs (void) {
	portDISABLE_INTERRUPTS();
	nested++;
}

void hal_enableIRQs (void) {
	if (--nested == 0) {
		nested = 0;
		
		hal_check_dio();

		portENABLE_INTERRUPTS();
	}	
}

void hal_sleep (void) {
}

// Returns the number of ticks until time. Negative values indicate that
// time has already passed.
static s4_t delta_time(u4_t time) {
    return (s4_t)(time - os_getTime());
}

/*
 * busy-wait until specified timestamp (in ticks) is reached.
 */
void hal_waitUntil (u4_t time) {
    s4_t delta = delta_time(time);
    // From delayMicroseconds docs: Currently, the largest value that
    // will produce an accurate delay is 16383.
    while (delta > (16000 / US_PER_OSTICK)) {
        udelay(16);
        delta -= (16000 / US_PER_OSTICK);
    }
	
    if (delta > 0)
        udelay(delta * US_PER_OSTICK);	
}

/*
 * check and rewind timer for target time.
 *   - return 1 if target time is close
 *   - otherwise rewind timer for target time or full period and return 0
 */
u1_t hal_checkTimer (u4_t targettime) {
	return delta_time(targettime) <= 0;
}

/*
 * perform fatal failure action.
 *   - called by assertions
 *   - action could be HALT or reboot
 */
void hal_failed (char *file, int line) {
	printf("assert: at %s, line %d\n", file, line);
	
	for(;;);
}