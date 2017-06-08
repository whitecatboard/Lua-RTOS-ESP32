/*
 * Lua RTOS, some common macros used in Lua RTOS
 *
 * Copyright (C) 2015 - 2017
 * IBEROXARXA SERVICIOS INTEGRALES, S.L.
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

#ifndef _LUA_RTOS_SYS_MACROS_H_
#define _LUA_RTOS_SYS_MACROS_H_

/**
 * @brief Test that all elements in a 2-size set are different.
 *
 * @param a first element
 * @param b second element
 * @param c third element
 *
 */
#define TEST_UNIQUE2(a,b) ((a!=b))

/**
 * @brief Test that all elements in a 3-size set are different.
 *
 * @param a first element
 * @param b second element
 * @param c third element
 *
 */
#define TEST_UNIQUE3(a,b,c) ((a!=b) && (a!=c) && (b!=c))

/**
 * @brief Test that all elements in a 4-size set are different.
 *
 * @param a first element
 * @param b second element
 * @param c third element
 * @param d fourth elements
 *
 */
#define TEST_UNIQUE4(a,b,c,d) ((a!=b) && (a!=c) && (a!=d) && (b!=c) && (b!=d) && (c!=d))

#endif
