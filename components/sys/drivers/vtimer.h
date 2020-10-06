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
 * Lua RTOS, virtual timer driver
 *
 */
#ifndef VTIMER_H
#define VTIMER_H

#include <stdint.h>
#include <sys/driver.h>

// VTIMER errors
#define VTIMER_ERR_INVALID_CALLBACK        (DRIVER_EXCEPTION_BASE(VTIMER_DRIVER_ID) |  0)

typedef void (*vtmr_callback_t)(void *);

/**
 * @brief Start a virtual timer. A callback function is executed once the timer expires.
 *
 * @param uiWhen        Time, in usecs, when the timer callback will be executed.
 *
 * @param pCallback     A pointer to a callback function that will be executed when the timer expires. Please,
 *                      take note that the callback is executed inside an ISR context, so it is intended to be
 *                      fast, and executed in IRAM. The callback function can be used later, if timer needs to
 *                      be stopped.
 *
 * @param pArgs         A pointer to a memory structure with the arguments to pass when the callback will called.
 *
 * @return
 *     - true if all is ok
 *     - false if an error occurred. In this case an error log message is written to the console.
 */
driver_error_t *vtmr_start(uint64_t uiWhen, vtmr_callback_t pCallback, void *pArgs);

/**
 * @brief Stop a virttual timer.
 *
 * @param pCallback     A pointer to a callback function, which identifies a previously started timer with
 *                      VTMR_StartTimer.
 *
 * @return
 *     - true if all is ok
 *     - false if an error occurred. In this case an error log message is written to the console.
 */
driver_error_t *vtmr_stop(vtmr_callback_t pCallback);

#endif /* VTIMER_H */
