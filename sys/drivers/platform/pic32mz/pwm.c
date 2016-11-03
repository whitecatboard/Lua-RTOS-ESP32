/*
 * Lua RTOS, pwm driver
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
 * 
 * Author: Jaume Olivé (jolive@iberoxarxa.com / jolive@whitecatboard.org)
 * 
 * All rights reserved.  
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */

#if LUA_USE_PWM

#include "whitecat.h"

#include <sys/syslog.h>
#include <sys/drivers/cpu.h>
#include <sys/drivers/pwm.h>
#include <sys/drivers/gpio.h>
#include <sys/drivers/error.h>
#include <sys/drivers/resource.h>
#include <math.h>
#include <stdlib.h>

/*
 * PBCLK      = peripheripal clock (MHZ)
 * pwm_freq   = pwm frequency hz
 * preescaler = timer preescaler
 * 
 *      PBCLK - pwm_freq * preescaler
 * pr = -----------------------------
 *                pwm_freq
 * 
 * 
 * 
 * 
 *                   /                               \
 *                  |           PBCLK                 |
 *            log10 |  ----------------------------   | 
 *                  |  pwm_freq * 1000 * preescaler   |
 *                   \                               /
 * pwm_res = ------------------------------------------------
 *                         log10(2) 
 */

struct pwm {
    unsigned int timer;      // Timer used by pwm
    unsigned int alternate;  // Alternate timer config?
};

struct pwm pwm[NOC] = {
    {4,0},
    {5,1},
    {4,0},
    {2,0},
    {3,1,},
    {2,0},
    {6,0},
    {7,1},
    {6,0},
};

static int output_map1 (unsigned channel) {
    switch (channel) {
        case 2: return 0b1011;   // OC3
        case 5: return 0b1100;   // OC6
    }

    syslog(LOG_ERR, "oc%u: cannot map OC pin, group 1", channel + 1);
    return 0;
}

static int output_map2 (unsigned channel) {
    switch (channel) {
        case 3: return 0b1011;   // OC4
        case 6: return 0b1100;   // OC7
    }

    syslog(LOG_ERR, "oc%u: cannot map OC pin, group 2", channel + 1);
    return 0;
}

static int output_map3 (unsigned channel) {
    switch (channel) {
        case 4: return 0b1011;   // OC5
        case 7: return 0b1100;   // OC8
    }

    syslog(LOG_ERR, "oc%u: cannot map OC pin, group 3", channel + 1);
    return 0;
}

static int output_map4 (unsigned channel) {
    switch (channel) {
        case 1: return 0b1011;   // OC2
        case 0: return 0b1100;   // OC1
        case 8: return 0b1101;   // OC9
    }

    syslog(LOG_ERR, "oc%u: cannot map OC pin, group 4", channel + 1);
    return 0;
}

static void assign_oc_pin(int channel, int pin) {
    switch (pin) {
        case RP('A',14): RPA14R = output_map1(channel); return;
        case RP('A',15): RPA15R = output_map2(channel); return;
        case RP('B',0):  RPB0R  = output_map3(channel); return;
        case RP('B',10): RPB10R = output_map1(channel); return;
        case RP('B',14): RPB14R = output_map4(channel); return;
        case RP('B',15): RPB15R = output_map3(channel); return;
        case RP('B',1):  RPB1R  = output_map2(channel); return;
        case RP('B',2):  RPB2R  = output_map4(channel); return;
        case RP('B',3):  RPB3R  = output_map2(channel); return;
        case RP('B',5):  RPB5R  = output_map1(channel); return;
        case RP('B',6):  RPB6R  = output_map4(channel); return;
        case RP('B',7):  RPB7R  = output_map3(channel); return;
        case RP('B',8):  RPB8R  = output_map3(channel); return;
        case RP('B',9):  RPB9R  = output_map1(channel); return;
        case RP('C',13): RPC13R = output_map2(channel); return;
        case RP('C',14): RPC14R = output_map1(channel); return;
        case RP('C',1):  RPC1R  = output_map1(channel); return;
        case RP('C',2):  RPC2R  = output_map4(channel); return;
        case RP('C',3):  RPC3R  = output_map3(channel); return;
        case RP('C',4):  RPC4R  = output_map2(channel); return;
        case RP('D',0):  RPD0R  = output_map4(channel); return;
        case RP('D',10): RPD10R = output_map1(channel); return;
        case RP('D',11): RPD11R = output_map2(channel); return;
        case RP('D',12): RPD12R = output_map3(channel); return;
        case RP('D',14): RPD14R = output_map1(channel); return;
        case RP('D',15): RPD15R = output_map2(channel); return;
        case RP('D',1):  RPD1R  = output_map4(channel); return;
        case RP('D',2):  RPD2R  = output_map1(channel); return;
        case RP('D',3):  RPD3R  = output_map2(channel); return;
        case RP('D',4):  RPD4R  = output_map3(channel); return;
        case RP('D',5):  RPD5R  = output_map4(channel); return;
        case RP('D',6):  RPD6R  = output_map1(channel); return;
        case RP('D',7):  RPD7R  = output_map2(channel); return;
        case RP('D',9):  RPD9R  = output_map3(channel); return;
        case RP('E',3):  RPE3R  = output_map3(channel); return;
        case RP('E',5):  RPE5R  = output_map2(channel); return;
        case RP('E',8):  RPE8R  = output_map4(channel); return;
        case RP('E',9):  RPE9R  = output_map3(channel); return;
        case RP('F',0):  RPF0R  = output_map2(channel); return;
        case RP('F',12): RPF12R = output_map3(channel); return;
        case RP('F',13): RPF13R = output_map4(channel); return;
        case RP('F',1):  RPF1R  = output_map1(channel); return;
        case RP('F',2):  RPF2R  = output_map4(channel); return;
        case RP('F',3):  RPF3R  = output_map4(channel); return;
        case RP('F',4):  RPF4R  = output_map1(channel); return;
        case RP('F',5):  RPF5R  = output_map2(channel); return;
        case RP('F',8):  RPF8R  = output_map3(channel); return;
        case RP('G',0):  RPG0R  = output_map2(channel); return;
        case RP('G',1):  RPG1R  = output_map1(channel); return;
        case RP('G',6):  RPG6R  = output_map3(channel); return;
        case RP('G',7):  RPG7R  = output_map2(channel); return;
        case RP('G',8):  RPG8R  = output_map1(channel); return;
        case RP('G',9):  RPG9R  = output_map4(channel); return;
    }
}

// Calculate PR value for timer for desired pwm frequency
unsigned int pwm_pr_freq(int pwmhz, int preescaler) {
    return (unsigned int)((((double)1.0 / (double)pwmhz) / (((double)1.0 / (double)PBCLK3_HZ) * (double)preescaler)) - (double)1.0);
}

// Calculate PR value for timer for desired pwm resolution
unsigned int pwm_pr_res(int res, int preescaler) {
    return pwm_pr_freq(PBCLK3_HZ / ((2 << (res - 1)) * preescaler), preescaler);    
}

static void pwm_configure_timer_freq(int unit, int pwmhz) {
    unsigned int pr;
    unsigned int preescaler;
    unsigned int preescaler_bits;
        
    preescaler_bits = 0;
    for(preescaler=1;preescaler <= 256;preescaler = preescaler * 2) {
        if (preescaler != 128) {
            pr = pwm_pr_freq(pwmhz, preescaler);
        
            if (pr <= 0xffff) {
                break;
            }
            
            preescaler_bits++;
        }
    }
    
    TCON(pwm[unit].timer) = (preescaler_bits << 4);
    TMR(pwm[unit].timer) = 0;
    PR(pwm[unit].timer) = pr;    
}

static void pwm_configure_timer_res(int unit, int res) {
    unsigned int pr;
    unsigned int preescaler;
    unsigned int preescaler_bits;
        
    preescaler_bits = 0;
    for(preescaler=1;preescaler <= 256;preescaler = preescaler * 2) {
        if (preescaler != 128) {
            pr = pwm_pr_res(res, preescaler);
        
            if (pr <= 0xffff) {
                break;
            }
            
            preescaler_bits++;
        }
    }
    
    TCON(pwm[unit].timer) = (preescaler_bits << 4);
    TMR(pwm[unit].timer) = 0;
    PR(pwm[unit].timer) = pr;    
}

static int pwm_timer_preescaler(int unit) {
    unsigned int preescaler_bits = ((TCON(pwm[unit].timer) >> 4) & 0b111);
    
    switch (preescaler_bits) {
        case 0: return 1;
        case 1: return 2;
        case 2: return 4;
        case 3: return 8;
        case 4: return 16;
        case 5: return 32;
        case 6: return 64;
        case 7: return 256;
    }
    
    return 0;
}

void pwm_start(int unit) {
    unit--;
    
    *(&OC1CONSET + unit * 0x80) = (1 << 15); // Enable OC module
    TCONSET(pwm[unit].timer) = (1 << 15);    // Start timer
}

void pwm_stop(int unit) {
    unit--;

    *(&OC1CONCLR + unit * 0x80) = (1 << 15); // Disable OC module
    TCONCLR(pwm[unit].timer) = (1 << 15);    // Stop timer
}


// Set new duty cycle
void pwm_set_duty(int unit, double duty) {
    unit--;

    *(&OC1R  + unit * 0x80) = PR(pwm[unit].timer) * duty;
    *(&OC1RS + unit * 0x80) = PR(pwm[unit].timer) * duty;    
}

void pwm_write(int unit, int res, int value) {
    unit--;

    double duty = (double)value / (double)(2 << (res - 1));
    
    *(&OC1R  + unit * 0x80) = PR(pwm[unit].timer) * duty;
    *(&OC1RS + unit * 0x80) = PR(pwm[unit].timer) * duty;
}

// Calculate real pwm frequency
unsigned int pwm_freq(int unit) {
    unit--;
    
    return (PBCLK3_HZ / (PR(pwm[unit].timer) *  pwm_timer_preescaler(unit)));
}

void pwm_end(int unit) {
    int pin = 0;
    unit--;

    pwm_stop(unit + 1);
    
    // Unlock pwm pin
    switch (unit) {
        case 0: pin = OC1_PINS;break;
        case 1: pin = OC2_PINS;break;
        case 2: pin = OC3_PINS;break;
        case 3: pin = OC4_PINS;break;
        case 4: pin = OC5_PINS;break;
        case 5: pin = OC6_PINS;break;
        case 6: pin = OC7_PINS;break;
        case 7: pin = OC8_PINS;break;
        case 8: pin = OC9_PINS;break;
    }
    
    resource_unlock(RES_GPIO, cpu_pin_number(pin) - 1);
}

void pwm_setup_freq(int unit, int pwmhz, double duty) {
    // Enable timer
    PMD4CLR = (1 << (pwm[unit].timer - 1));

    // Stop timer
    TCONCLR(pwm[unit].timer) = (1 << 15);
    
    pwm_configure_timer_freq(unit, pwmhz);
    
    // Configure OC module
    *(&OC1CON + unit * 0x80) = 0;
    *(&OC1R  + unit * 0x80)  = PR(pwm[unit].timer) * duty;
    *(&OC1RS + unit * 0x80)  = PR(pwm[unit].timer) * duty;
    *(&OC1CON + unit * 0x80) = 0x0006;   
    
    if (pwm[unit].alternate) {
        *(&OC1CONSET + unit * 0x80) = (1 << 3);
    } else {
        *(&OC1CONCLR + unit * 0x80) = (1 << 3);
    }    
}

void pwm_setup_res(int unit, int res, int value) {
    double duty = (double)value / (double)(2 << (res - 1));
    
    // Enable timer
    PMD4CLR = (1 << (pwm[unit].timer - 1));

    // Stop timer
    TCONCLR(pwm[unit].timer) = (1 << 15);

    pwm_configure_timer_res(unit, res);
    
    // Configure OC module
    *(&OC1CON + unit * 0x80) = 0;
    *(&OC1R  + unit * 0x80)  = PR(pwm[unit].timer) * duty;
    *(&OC1RS + unit * 0x80)  = PR(pwm[unit].timer) * duty;
    *(&OC1CON + unit * 0x80) = 0x0006;
    
    if (pwm[unit].alternate) {
        *(&OC1CONSET + unit * 0x80) = (1 << 3);
    } else {
        *(&OC1CONCLR + unit * 0x80) = (1 << 3);
    }    
}

tdriver_error *pwm_init_freq(int unit, int pwmhz, double duty) {
    tresource_lock *lock;
    int pin = 0;
    unit--;

    // Lock pwm pin
    switch (unit) {
        case 0: pin = OC1_PINS;break;
        case 1: pin = OC2_PINS;break;
        case 2: pin = OC3_PINS;break;
        case 3: pin = OC4_PINS;break;
        case 4: pin = OC5_PINS;break;
        case 5: pin = OC6_PINS;break;
        case 6: pin = OC7_PINS;break;
        case 7: pin = OC8_PINS;break;
        case 8: pin = OC9_PINS;break;
    }
    
    lock = resource_lock(RES_GPIO, cpu_pin_number(pin) - 1, RES_PWM, unit);
    if (!resource_granted(lock)) {
        return lock_error(lock);
    }
    free(lock);

    // Enable module
    switch (unit) {
        case 0: PMD3CLR = OC1MD;break;
        case 1: PMD3CLR = OC2MD;break;
        case 2: PMD3CLR = OC3MD;break;
        case 3: PMD3CLR = OC4MD;break;
        case 4: PMD3CLR = OC5MD;break;
        case 5: PMD3CLR = OC6MD;break;
        case 6: PMD3CLR = OC7MD;break;
        case 7: PMD3CLR = OC8MD;break;
        case 8: PMD3CLR = OC9MD;break;
    }
    
    assign_oc_pin(unit, pin);
    
    pwm_setup_freq(unit, pwmhz, duty);
    
    syslog(LOG_INFO,"pwm%d, at pin %c%d using timer%d, %d hz", unit + 1, 
           gpio_portname(pin), gpio_pinno(pin), pwm[unit].timer, pwm_freq(unit + 1));
           
    return NULL;
}

tdriver_error *pwm_init_res(int unit, int res, int val) {
    tresource_lock *lock;
    int pin = 0;
    unit--;
    
    // Lock pwm pin
    switch (unit) {
        case 0: pin = OC1_PINS;break;
        case 1: pin = OC2_PINS;break;
        case 2: pin = OC3_PINS;break;
        case 3: pin = OC4_PINS;break;
        case 4: pin = OC5_PINS;break;
        case 5: pin = OC6_PINS;break;
        case 6: pin = OC7_PINS;break;
        case 7: pin = OC8_PINS;break;
        case 8: pin = OC9_PINS;break;
    }

    lock = resource_lock(RES_GPIO, cpu_pin_number(pin) - 1, RES_PWM, unit);
    if (!resource_granted(lock)) {
        return lock_error(lock);
    }
    free(lock);

    // Enable module
    switch (unit) {
        case 0: PMD3CLR = OC1MD;break;
        case 1: PMD3CLR = OC2MD;break;
        case 2: PMD3CLR = OC3MD;break;
        case 3: PMD3CLR = OC4MD;break;
        case 4: PMD3CLR = OC5MD;break;
        case 5: PMD3CLR = OC6MD;break;
        case 6: PMD3CLR = OC7MD;break;
        case 7: PMD3CLR = OC8MD;break;
        case 8: PMD3CLR = OC9MD;break;
    }    

    assign_oc_pin(unit, pin);
    
    pwm_setup_res(unit, res, val);

    syslog(LOG_INFO,"pwm%d, at pin %c%d, %d hz", unit + 1, 
           gpio_portname(pin), gpio_pinno(pin), pwm_freq(unit + 1));
           
    return NULL;
}

void pwm_pins(int unit, unsigned char *pin) {
    int channel = unit - 1;

    switch (channel) {
        case 0:
            *pin = OC1_PINS & 0xFF;
            break;
        case 1:
            *pin = OC2_PINS & 0xFF;
            break;
        case 2:
            *pin = OC3_PINS & 0xFF;
            break;
        case 3:
            *pin = OC4_PINS & 0xFF;
            break;
        case 4:
            *pin = OC5_PINS & 0xFF;
            break;
        case 5:
            *pin = OC6_PINS & 0xFF;
            break;
        case 6:
            *pin = OC7_PINS & 0xFF;
            break;
        case 7:
            *pin = OC8_PINS & 0xFF;
            break;
        case 8:
            *pin = OC9_PINS & 0xFF;
            break;
    }
}

#endif
