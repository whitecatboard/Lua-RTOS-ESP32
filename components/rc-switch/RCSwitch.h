/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2019, Thomas E. Horner (whitecatboard.org@horner.it)
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
 * This file incorporates work covered by the following copyright and permission notice:

  rcswitch-avr - avr-c port of the RCSwitch library (https://github.com/sui77/rc-switch/)
  Copyright (c) 2017 Yann Lochet
  This file incorporates work covered by the following copyright and permission notice:

  RCSwitch - Arduino library for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.
  
  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Andreas Steinel / A.<lastname>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com
  - Johann Richard / <first name>.<last name>(at)gmail(dot)com
  - Vlad Gheorghe / <first name>.<last name>(at)gmail(dot)com https://github.com/vgheo
  
  Project home: https://github.com/sui77/rc-switch/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _RCSwitch_h
#define _RCSwitch_h

#include "sdkconfig.h"
#if CONFIG_LUA_RTOS_LUA_USE_RCSWITCH

#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad, FraunchPad and StellarPad specific
    #include "Energia.h"
#elif defined(RPI) // Raspberry Pi
    #define RaspberryPi

    // Include libraries for RPi:
    #include <string.h> /* memcpy */
    #include <stdlib.h> /* abs */
    #include <wiringPi.h>
#elif defined(__XTENSA__)
    #include <string.h> /* memcpy */
    #include <stdlib.h> /* abs */
    #include <arduino.h>
#elif defined(SPARK)
    #include "application.h"
#else
    #include "WProgram.h"
#endif

#include <stdint.h>


// At least for the ATTiny X4/X5, receiving has to be disabled due to
// missing libm depencies (udivmodhi4)
#if defined( __AVR_ATtinyX5__ ) || defined ( __AVR_ATtinyX4__ )
#define RCSwitchDisableReceiving
#endif

// Number of maximum high/Low changes per packet.
// We can handle up to (unsigned long) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 67


    void RCSwitch_init();
    
    void RCSwitch_switchOnBy1(int nGroupNumber, int nSwitchNumber);
    void RCSwitch_switchOffBy1(int nGroupNumber, int nSwitchNumber);
    void RCSwitch_switchOnBy2(const char* sGroup, int nSwitchNumber);
    void RCSwitch_switchOffBy2(const char* sGroup, int nSwitchNumber);
    void RCSwitch_switchOnBy3(char sFamily, int nGroup, int nDevice);
    void RCSwitch_switchOffBy3(char sFamily, int nGroup, int nDevice);
    void RCSwitch_switchOnBy4(const char* sGroup, const char* sDevice);
    void RCSwitch_switchOffBy4(const char* sGroup, const char* sDevice);
    void RCSwitch_switchOnBy5(char sGroup, int nDevice);
    void RCSwitch_switchOffBy5(char sGroup, int nDevice);

    void RCSwitch_sendTriState(const char* sCodeWord);
    void RCSwitch_send(unsigned long code, unsigned int length);
    void RCSwitch_sendBinary(const char* sCodeWord);
    
    #if !defined( RCSwitchDisableReceiving )
    void RCSwitch_enableReceive(int interrupt);
    void RCSwitch_enableReceiveAgain();
    void RCSwitch_disableReceive();
    bool RCSwitch_available();
    void RCSwitch_resetAvailable();

    unsigned long RCSwitch_getReceivedValue();
    unsigned int RCSwitch_getReceivedBitlength();
    unsigned int RCSwitch_getReceivedDelay();
    unsigned int RCSwitch_getReceivedProtocol();
    unsigned int* RCSwitch_getReceivedRawdata();
    #endif
  
    void RCSwitch_enableTransmit(int nTransmitterPin);
    void RCSwitch_disableTransmit();
    void RCSwitch_setPulseLength(int nPulseLength);
    void RCSwitch_setRepeatTransmit(int nRepeatTransmit);
    #if !defined( RCSwitchDisableReceiving )
    void RCSwitch_setReceiveTolerance(int nPercent);
    #endif

    /**
     * Description of a single pule, which consists of a high signal
     * whose duration is "high" times the base pulse length, followed
     * by a low signal lasting "low" times the base pulse length.
     * Thus, the pulse overall lasts (high+low)*pulseLength
     */
    struct HighLow {
        uint8_t high;
        uint8_t low;
    };
    typedef struct HighLow HighLow;

    /**
     * A "protocol" describes how zero and one bits are encoded into high/low
     * pulses.
     */
    struct Protocol {
        /** base pulse length in microseconds, e.g. 350 */
        uint16_t pulseLength;

        HighLow syncFactor;
        HighLow zero;
        HighLow one;

        /**
         * If true, interchange high and low logic levels in all transmissions.
         *
         * By default, RCSwitch assumes that any signals it sends or receives
         * can be broken down into pulses which start with a high signal level,
         * followed by a a low signal level. This is e.g. the case for the
         * popular PT 2260 encoder chip, and thus many switches out there.
         *
         * But some devices do it the other way around, and start with a low
         * signal level, followed by a high signal level, e.g. the HT6P20B. To
         * accommodate this, one can set invertedSignal to true, which causes
         * RCSwitch to change how it interprets any HighLow struct FOO: It will
         * then assume transmissions start with a low signal lasting
         * FOO.high*pulseLength microseconds, followed by a high signal lasting
         * FOO.low*pulseLength microseconds.
         */
        bool invertedSignal;
    };
    typedef struct Protocol Protocol;

    void RCSwitch_setProtocolUserdefined(Protocol protocolUserdefined);
    void RCSwitch_setProtocol(int nProtocol);
    void RCSwitch_setProtocolAndPulse(int nProtocol, int nPulseLength);

    #if 0 // static, moved to RCSwitch.c
    char* getCodeWordA(const char* sGroup, const char* sDevice, bool bStatus);
    char* getCodeWordB(int nGroupNumber, int nSwitchNumber, bool bStatus);
    char* getCodeWordC(char sFamily, int nGroup, int nDevice, bool bStatus);
    char* getCodeWordD(char group, int nDevice, bool bStatus);
    void transmit(HighLow pulses);

    #if !defined( RCSwitchDisableReceiving )
    static void handleInterrupt(void* arg);
    static bool receiveProtocol(const int p, unsigned int changeCount);
    int nReceiverInterrupt;
    #endif
    int nTransmitterPin;
    int nRepeatTransmit;
    
    struct Protocol protocol;

    #if !defined( RCSwitchDisableReceiving )
    static int nReceiveTolerance;
    volatile static unsigned long nReceivedValue;
    volatile static unsigned int nReceivedBitlength;
    volatile static unsigned int nReceivedDelay;
    volatile static unsigned int nReceivedProtocol;
    const static unsigned int nSeparationLimit;
    /* 
     * timings[0] contains sync timing, followed by a number of bits
     */
    static unsigned int timings[RCSWITCH_MAX_CHANGES];
    #endif
    #endif
    

#endif
#endif
