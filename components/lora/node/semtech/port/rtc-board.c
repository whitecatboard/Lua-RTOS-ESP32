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

#include <time.h>
#include <sys/drivers/timer.h>

extern void TimerIrqHandler(void);

// MCU Wake Up Time
#define MIN_ALARM_DELAY                             1 // in ticks, 1 tick = 1 msec

// sub-second number of bits
#define N_PREDIV_S                                  10

// Synchronous prediv
#define PREDIV_S                                    ( ( 1 << N_PREDIV_S ) - 1 )

// Asynchronous prediv
#define PREDIV_A                                    ( 1 << ( 15 - N_PREDIV_S ) ) - 1

// Sub-second mask definition
#define ALARM_SUBSECOND_MASK                        ( N_PREDIV_S << RTC_ALRMASSR_MASKSS_Pos )

// RTC Time base in us
#define USEC_NUMBER                                 1000000
#define MSEC_NUMBER                                 ( USEC_NUMBER / 1000 )

#define COMMON_FACTOR                               3
#define CONV_NUMER                                  1
#define CONV_DENOM                                  1

/*!
 * \brief Days, Hours, Minutes and seconds
 */
#define DAYS_IN_LEAP_YEAR                           ( ( uint32_t )  366U )
#define DAYS_IN_YEAR                                ( ( uint32_t )  365U )
#define SECONDS_IN_1DAY                             ( ( uint32_t )86400U )
#define SECONDS_IN_1HOUR                            ( ( uint32_t ) 3600U )
#define SECONDS_IN_1MINUTE                          ( ( uint32_t )   60U )
#define MINUTES_IN_1HOUR                            ( ( uint32_t )   60U )
#define HOURS_IN_1DAY                               ( ( uint32_t )   24U )

/*!
 * \brief Correction factors
 */
#define  DAYS_IN_MONTH_CORRECTION_NORM              ( ( uint32_t )0x99AAA0 )
#define  DAYS_IN_MONTH_CORRECTION_LEAP              ( ( uint32_t )0x445550 )

/*!
 * \brief Calculates ceiling( X / N )
 */
#define DIVC( X, N )                                ( ( ( X ) + ( N ) -1 ) / ( N ) )

/*!
 * RTC timer context
 */
typedef struct
{
    uint32_t        Time;         // Reference time
}RtcTimerContext_t;

/*!
 * \brief Indicates if the RTC is already Initialized or not
 */
static bool RtcInitialized = false;

/*!
 * \brief Indicates if the RTC Wake Up Time is calibrated or not
 */
static bool McuWakeUpTimeInitialized = false;

/*!
 * \brief Compensates MCU wakeup time
 */
static int16_t McuWakeUpTimeCal = 0;

/*!
 * Number of days in each month on a normal year
 */
static const uint8_t DaysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*!
 * Number of days in each month on a leap year
 */
static const uint8_t DaysInMonthLeapYear[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*!
 * Keep the value of the RTC timer when the RTC alarm is set
 * Set with the \ref RtcSetTimerContext function
 * Value is kept as a Reference to calculate alarm
 */
static RtcTimerContext_t RtcTimerContext;

static void isr(void *args) {
    TimerIrqHandler();
}

void RtcInit( void )
{
    if( RtcInitialized == false )
    {
        driver_error_t *error;

        if ((error = tmr_setup(0, 0, isr, 0, 1))) {
            free(error);
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
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    RtcTimerContext.Time = (uint32_t)tp.tv_sec * 1000 + (uint32_t)(tp.tv_nsec / 1000000);
    return RtcTimerContext.Time;
}

/*!
 * \brief Gets the RTC timer reference
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcGetTimerContext( void )
{
    return RtcTimerContext.Time;
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
    return milliseconds;
}

/*!
 * \brief converts time in ticks to time in ms
 *
 * \param[IN] time in timer ticks
 * \retval returns time in milliseconds
 */
uint32_t RtcTick2Ms( uint32_t tick )
{
    return tick;
}

/*!
 * \brief a delay of delay ms by polling RTC
 *
 * \param[IN] delay in ms
 */
void RtcDelayMs( uint32_t del )
{
    delay(del);
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
    driver_error_t *error;

    if ((error = tmr_set_period(0, timeout * 1000))) {
        free(error);
        return;
    }

    RtcStartAlarm( timeout );
}

void RtcStopAlarm( void )
{
    tmr_stop(0);
}

void RtcStartAlarm( uint32_t timeout )
{
    tmr_start(0);
}

uint32_t RtcGetTimerValue( void )
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    return (uint32_t)tp.tv_sec * 1000 + (uint32_t)(tp.tv_nsec / 1000000);
}

uint32_t RtcGetTimerElapsedTime( void )
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);

    return (uint32_t)tp.tv_sec * 1000 + (uint32_t)(tp.tv_nsec / 1000000) - RtcTimerContext.Time;
}

void RtcSetMcuWakeUpTime( void )
{
}

int16_t RtcGetMcuWakeUpTime( void )
{
    return 0;
}

#if 0
void RtcBkupWrite( uint32_t data0, uint32_t data1 )
{
    HAL_RTCEx_BKUPWrite( &RtcHandle, RTC_BKP_DR0, data0 );
    HAL_RTCEx_BKUPWrite( &RtcHandle, RTC_BKP_DR1, data1 );
}

void RtcBkupRead( uint32_t *data0, uint32_t *data1 )
{
  *data0 = HAL_RTCEx_BKUPRead( &RtcHandle, RTC_BKP_DR0 );
  *data1 = HAL_RTCEx_BKUPRead( &RtcHandle, RTC_BKP_DR1 );
}
#endif

void RtcProcess( void )
{
}

TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature )
{
    return period;
}
