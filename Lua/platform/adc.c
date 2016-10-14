#include "whitecat.h"

#if LUA_USE_ADC

#include "Lua/modules/adc.h"
#include <drivers/adc/adc.h>

int platform_adc_exists( unsigned id ) {
    return ((id >= 0) && (id <= NADC));
}

/*
u32 platform_adc_get_maxval( unsigned id )
{
  return pow( 2, ADC_BIT_RESOLUTION ) - 1;
}

u32 platform_adc_set_smoothing( unsigned id, u32 length )
{
  return adc_update_smoothing( id, ( u8 )intlog2( ( unsigned ) length ) );
}

void platform_adc_set_blocking( unsigned id, u32 mode )
{
  adc_get_ch_state( id )->blocking = mode;
}

void platform_adc_set_freerunning( unsigned id, u32 mode )
{
  adc_get_ch_state( id )->freerunning = mode;
}

u32 platform_adc_is_done( unsigned id )
{
  return adc_get_ch_state( id )->op_pending == 0;
}

void platform_adc_set_timer( unsigned id, u32 timer )
{
  elua_adc_dev_state *d = adc_get_dev_state( 0 );

  if ( d->timer_id != timer )
    d->running = 0;
  platform_adc_stop( id );
  d->timer_id = timer;
}
*/

#endif