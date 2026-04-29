/*!
 * \file      rtc-board.c
 *
 * \brief     Target board RTC timer and low power modes management
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech - STMicroelectronics
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    MCD Application Team (C)( STMicroelectronics International )
 */
#include <math.h>
#include <time.h>
#include "utilities.h"
#include "delay.h"
#include "board.h"
#include "timer.h"
#include "systime.h"
#include "gpio.h"
#include "rtc-board.h"

#include "esp_timer.h"
#include "esp_attr.h"

#include <sys/time.h>

// MCU Wake Up Time
#define MIN_ALARM_DELAY                             300 // in ticks

/*!
 * \brief Indicates if the RTC is already Initialized or not
 */
static bool RtcInitialized = false;

// Using RTC_DATA_ATTR ensures these survive deep sleep if needed
RTC_DATA_ATTR static uint32_t RtcTimerContextValue = 0;
RTC_DATA_ATTR static uint32_t RtcBkupData[2] = { 0 };

static esp_timer_handle_t rtc_timer;

extern void _unblock_lora_task(void *arg);

static void _timer_cb(void *arg) {    	
	_unblock_lora_task(arg);
}

void RtcInit( void )
{
	if( RtcInitialized == false )
	{
		esp_timer_create_args_t rtc_timer_args = {
	        .callback = _timer_cb,
	        .arg = TimerIrqHandler,
	        .dispatch_method =  ESP_TIMER_TASK,
	        .name = "rtc_timer"
	    };
	    
	    esp_err_t ret = esp_timer_create(&rtc_timer_args, &rtc_timer);
	    if (ret != ESP_OK) {
			// TO DO
			assert(false);
			return;			
		}
		
	    RtcSetTimerContext( );
	    RtcInitialized = true;
	}
}

/*!
 * \brief Sets the RTC timer reference, sets also the RTC_DateStruct and RTC_TimeStruct
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcSetTimerContext( void )
{
	RtcTimerContextValue = RtcGetTimerValue( );
	return RtcTimerContextValue;
}

/*!
 * \brief Gets the RTC timer reference
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcGetTimerContext( void )
{
    return RtcTimerContextValue;
}

/*!
 * \brief returns the wake up time in ticks
 *
 * \retval wake up time in ticks
 */
uint32_t RtcGetMinimumTimeout( void )
{
    return( MIN_ALARM_DELAY );
}

/*!
 * \brief converts time in ms to time in ticks
 *
 * \param[IN] milliseconds Time in milliseconds
 * \retval returns time in timer ticks
 */
uint32_t RtcMs2Tick( uint32_t milliseconds )
{
    return milliseconds * 1000;
}

/*!
 * \brief converts time in ticks to time in ms
 *
 * \param[IN] time in timer ticks
 * \retval returns time in milliseconds
 */
uint32_t RtcTick2Ms( uint32_t tick )
{
	return (tick / 1000);
}

/*!
 * \brief a delay of delay ms by polling RTC
 *
 * \param[IN] delay in ms
 */
void RtcDelayMs( uint32_t delay )
{
    uint64_t delayTicks = 0;
    uint64_t refTicks = RtcGetTimerValue( );

    delayTicks = RtcMs2Tick( delay );

    // Wait delay ms
    while( ( ( RtcGetTimerValue( ) - refTicks ) ) < delayTicks )
    {
		__asm__ volatile ("nop");
    }
}

/*!
 * \brief Sets the alarm
 *
 * \note The alarm is set at now (read in this function) + timeout
 *
 * \param timeout Duration of the Timer ticks
 */
void RtcSetAlarm( uint32_t timeout )
{
    RtcStartAlarm( timeout );
}

void RtcStopAlarm( void )
{
	esp_timer_stop(rtc_timer);
}

void RtcStartAlarm( uint32_t timeout )
{
    CRITICAL_SECTION_BEGIN();
    RtcStopAlarm();
    esp_timer_start_once(rtc_timer, timeout);   
    CRITICAL_SECTION_END();
}

uint32_t RtcGetTimerValue( void )
{
	// Microseconds since boot
    return ( uint32_t )esp_timer_get_time( );
}

uint32_t RtcGetTimerElapsedTime( void )
{
	return ( uint32_t )( RtcGetTimerValue( ) - RtcTimerContextValue );
}

uint32_t RtcGetCalendarTime( uint16_t *milliseconds )
{
	struct timeval now;
    gettimeofday( &now, NULL );

    if( milliseconds != NULL )
    {
        *milliseconds = ( uint16_t )( now.tv_usec / 1000 );
    }

    return ( uint32_t )now.tv_sec;
}

void RtcBkupWrite(uint32_t data0, uint32_t data1)
{
    CRITICAL_SECTION_BEGIN();    
    RtcBkupData[0] = data0;
    RtcBkupData[1] = data1;    
    CRITICAL_SECTION_END();
}

void RtcBkupRead( uint32_t *data0, uint32_t *data1 )
{
	CRITICAL_SECTION_BEGIN();
	*data0 = RtcBkupData[0];
	*data1 = RtcBkupData[1];
	CRITICAL_SECTION_END();
}

void RtcProcess( void )
{
	// Not used on ESP32
}

TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature )
{
	// ESP32 internal timers are generally stable
    return period;
}
