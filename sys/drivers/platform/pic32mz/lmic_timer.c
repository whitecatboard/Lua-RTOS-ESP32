#include "FreeRTOS.h"
#include "timers.h"

#include "lmic.h"

#include <stdlib.h>
#include <stdint.h>

#include <machine/pic32mz.h>

#include <sys/delay.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/error.h>
#include <sys/drivers/resource.h>
#include <sys/syslog.h>

char lmic_timer = 0; // Timer used by lmic

static volatile ostime_t ticks = 0;
static volatile ostime_t delta = 0;

tdriver_error *lmic_setup_timer() {
    unsigned int pr;
    unsigned int preescaler;
    unsigned int preescaler_bits;
	
	tresource_lock *lock;

    // Lock needed resources: timer
    if (!lmic_timer) {
        lock = resource_lock(RES_TIMER, -1, RES_LMIC, -1);
        if (!lock) {
            tdriver_error *error;

            error = (tdriver_error *)malloc(sizeof(tdriver_error));
            if (error) {
                error->type = LOCK;
                error->resource = RES_TIMER;
                error->resource_unit = -1;            
                error->owner = -1;
                error->owner_unit = -1;
            }

            syslog(LOG_ERR,"lmic, no available timer");        
            free(lock);

            return error;
        } else {
            lmic_timer = lock->unit + 1;
        }    
		    
        free(lock);
    }
	
    // Enable timer module
    PMD4CLR = (1 << (lmic_timer - 1));
        
    // Disable timer
    TCON(lmic_timer) = 0;
    
    // Computes most lower preescaler for current frequency and period value
    preescaler_bits = 0;
    for(preescaler=1;preescaler <= 256;preescaler = preescaler * 2) {
        if (preescaler != 128) {
            pr = ( (PBCLK3_HZ / preescaler) / LMIC_TIMER_HZ ) - 1;
            if (pr <= 0xffff) {
                break;
            }
            
            preescaler_bits++;
        }
    }
            
    // Configure timer
    TCON(lmic_timer) = (preescaler_bits << 4);
    PR(lmic_timer) = pr;   
    
    // Configure timer interrupts
    int irq = PIC32_IRQ_T1 + (lmic_timer - 1) * 5;

    IPCCLR(irq >> 2) = 0x1f << (5 * (irq & 0x03));
    IPCSET(irq >> 2) = (0x1f << (8 * (irq & 0x03))) & 0x0d0d0d0d;
    IFSCLR(irq >> 5) = 1 << (irq & 31); 
    IECSET(irq >> 5) = 1 << (irq & 31); 

    // Start timer
    TCONSET(lmic_timer) = (1 << 15);
    
    syslog(LOG_INFO,"lmic, using timer%d, irq %d", lmic_timer, irq);        

	return NULL;
}

// Timer interrupt handler
void lmic_intr(u8_t irq) {  
	//BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	// Increment ticks
	ticks++;
	//delta++;
	
	//if (delta == 3) {
	//	delta = 0;
	//	xTimerPendFunctionCallFromISR(lmic_intr_deferred, NULL, 0, &xHigherPriorityTaskWoken);
	//}
	
    IFSCLR(irq >> 5) = 1 << (irq & 31); 
	
	//portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

ostime_t os_getTime () {
	ostime_t time;
	
	hal_disableIRQs();
	time = ticks;
	hal_enableIRQs();
		
    return time;
}
