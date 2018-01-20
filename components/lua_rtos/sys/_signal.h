/*
 * signal.h
 *
 *  Created on: Jan 16, 2018
 *      Author: jaumeolivepetrus
 */

#ifndef COMPONENTS_LUA_RTOS_SYS___SIGNAL_H_
#define COMPONENTS_LUA_RTOS_SYS___SIGNAL_H_

typedef struct {
	int s;    // Signal number
	int dest; // Signal destination
} signal_data_t;

void _signal_queue(int dest, int s);
void _signal_init();

#endif /* COMPONENTS_LUA_RTOS_SYS___SIGNAL_H_ */
