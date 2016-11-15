#include "lua.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct lua_rtos_tcb {
    int        threadid;
 	uint32_t   signaled;	
  	lua_State *L;
};

// This macro is not present in all FreeRTOS ports. In Lua RTOS is used in some places
// in the source code shared by all the supported platforms. ESP32 don't define this macro,
// so we define it for reuse code beetween platforms.
#define portEND_SWITCHING_ISR(xSwitchRequired) \
if (xSwitchRequired) {	  \
	_frxt_setup_switch(); \
}

void enter_critical_section();
void exit_critical_section();

UBaseType_t uxGetTaskId();
UBaseType_t uxGetThreadId();
void uxSetThreadId(UBaseType_t id);
void uxSetLuaState(lua_State* L);
lua_State* pvGetLuaState();
void uxSetSignaled(TaskHandle_t h, int s);
uint32_t uxGetSignaled(TaskHandle_t h);
TaskHandle_t xGetCurrentTask();
