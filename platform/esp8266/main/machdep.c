#include "FreeRTOS.h"
#include "whitecat.h"

extern void mach_dev();
extern void _clock_init();
extern void _syscalls_init();
extern void _pthread_init();
extern void _console_init();
extern void _lora_init();
extern void _signal_init();
extern void _mtx_init();

void mach_init() {  
    //resource_init();
    _mtx_init();
    _pthread_init();
    _clock_init();
    _syscalls_init();
	_console_init();
    _signal_init();
    
    #if LUA_USE_LORA
    _lora_init();
    #endif
    
    mach_dev();
}