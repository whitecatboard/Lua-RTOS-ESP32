#ifndef _lmic_config_h_
#define _lmic_config_h_

#if LUA_USE_LORA
#if USE_LMIC

#include <stdio.h>

#define LMIC_DEBUG_LEVEL 1
#define LMIC_FAILURE_TO Serial

#endif
#endif

#endif // _lmic_config_h_
