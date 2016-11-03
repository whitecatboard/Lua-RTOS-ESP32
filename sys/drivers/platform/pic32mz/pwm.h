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

#ifndef PWM_H
#define	PWM_H

#define NOC 9

#include <sys/drivers/error.h>
#include <sys/drivers/resource.h>

unsigned int pwm_pr_freq(int pwmhz, int presscaler);
unsigned int pwm_pr_res(int res, int presscaler);
unsigned int pwm_res(int pwmhz);
unsigned int pwm_freq(int unit);
void pwm_start(int unit);
void pwm_stop(int unit);
void pwm_set_duty(int unit, double duty);
void pwm_write(int unit, int res, int value);
void pwm_setup_freq(int unit, int pwmhz, double duty);
void pwm_setup_res(int unit, int res, int value);
tdriver_error *pwm_init_freq(int unit, int pwmhz, double duty); 
tdriver_error *pwm_init_res(int unit, int res, int val);
void pwm_pins(int unit, unsigned char *pin);
void pwm_end(int unit);

#endif	/* PWM_H */

