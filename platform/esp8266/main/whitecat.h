/*
 * Whitecat main configuration file
 *
 * Copyright (C) 2015 - 2016
 * IBEROXARXA SERVICIOS INTEGRALES, S.L. & CSS IBÉRICA, S.L.
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

#ifndef WHITECAT_H
#define	WHITECAT_H

#define LUA_OS_VER "beta 0.1"

#define UNUSED_ARG(x) (void)(x)
	
// ---------------------------------------------------------------------------
// System frequencies
// ---------------------------------------------------------------------------

//#define PBCLK2_HZ 100000000L // PMP/I2C/UART/SPI
//#define PBCLK3_HZ  40000000L // TIMERS
//#define PBCLK4_HZ 200000000L // PORTS
//#define PBCLK5_HZ  40000000L // CAN

//#define PBCLK2_DIV ((CPU_HZ / PBCLK2_HZ) - 1)
//#define PBCLK3_DIV ((CPU_HZ / PBCLK3_HZ) - 1)
//#define PBCLK4_DIV ((CPU_HZ / PBCLK4_HZ) - 1)
//#define PBCLK5_DIV ((CPU_HZ / PBCLK5_HZ) - 1)

#define USE_LORA             0
#define USE_RTC              0
#define USE_ETHERNET         0
#define USE_WIFI             0
#define USE_GPRS             0
#define USE_GPS              0
#define USE_SD               0
#define USE_DISPLAY          0
#define USE_CAN              0
#define USE_STEPPER          0

// ---------------------------------------------------------------------------
// Leds
// ---------------------------------------------------------------------------
#define LED_1   0
#define LED_ACT LED_1
#define LED_NET LED_1

// ---------------------------------------------------------------------------
// LORAWAN
// ---------------------------------------------------------------------------
#define LORA_TRANSCEIVER_RN2483     1
#define LORA_TRANSCEIVER_TYPE       LORA_TRANSCEIVER_RN2483


// ---------------------------------------------------------------------------
// Ethernet
// ---------------------------------------------------------------------------
#define IP4_GET_ADDR(a,b,c,d) \
        (((u32_t)((d) & 0xff) << 24) | \
        ((u32_t)((c) & 0xff) << 16) | \
        ((u32_t)((b) & 0xff) << 8)  | \
        (u32_t)((a) & 0xff))

// Phy settings
#define PHY_TYPE_LAN8720     1
#define PHY_TYPE_ENC424J600  2

#define PHY_TYPE PHY_TYPE_ENC424J600

// DMA descriptor settings
#define EMAC_TX_DESCRIPTORS  8    // TX descriptors to be created
#define EMAC_RX_DESCRIPTORS  8    // RX descriptors and RX buffers to be created
#define	EMAC_RX_BUFF_SIZE    1536 // size of a RX buffer (multiple of 16)

// IP configuration
#define ETH_USE_FIXED_IP     0    // Set to 0 if you want to use DHCP

#if ETH_USE_FIXED_IP
    //#define eth_ip       IP4_GET_ADDR(192,168,18,200)
    //#define eth_netmask  IP4_GET_ADDR(255,255,255,0)
    //#define eth_gateway  IP4_GET_ADDR(192,168,18,1)

    #define eth_ip       IP4_GET_ADDR(192,168,1,250)
    #define eth_netmask  IP4_GET_ADDR(255,255,255,0)
    #define eth_gateway  IP4_GET_ADDR(192,168,1,1)
#else
    #define eth_ip       IP4_GET_ADDR(0,0,0,0)
    #define eth_netmask  IP4_GET_ADDR(0,0,0,0)
    #define eth_gateway  IP4_GET_ADDR(0,0,0,0)

    #define LWIP_DHCP                   1
    #define LWIP_DHCP_CHECK_LINK_UP     1
#endif

#define DNS1        IP4_GET_ADDR(8,8,8,8)
#define DNS2        IP4_GET_ADDR(4,4,4,4)
#define PING_TARGET DNS1
#define PING_DELAY  30000

// ---------------------------------------------------------------------------
// GPRS
// ---------------------------------------------------------------------------
#define SIM908_UART       4
#define SIM908_BR         115200  // Baud rate
#define SIM908_PWR_PIN      0x50    // RE0
#define SIM908_STATUS_PIN   0x51    // RE1

// GPS
#define SIM908_UART_DBG   3       // GPS datata UART
#define GPS_BR            115200  // Baud rate


// OUTPUT COMPARE
#define OC1_PINS      0x22
#define OC2_PINS      0x26
#define OC3_PINS      0x00
#define OC4_PINS      0x21
#define OC5_PINS      0x20
#define OC6_PINS      0x00
#define OC7_PINS      0x23
#define OC8_PINS      0x28
#define OC9_PINS      0x00

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------
#define I2C1_PINS     0x4a49    // scl=D10, sda=D9
#define I2C2_PINS     0x0000
#define I2C3_PINS     0x0000
#define I2C4_PINS     0x7877    // scl=RG8, sda=RG7
#define I2C5_PINS     0x0000

// -----------------------------------------------------------------------------
// UARTS
// -----------------------------------------------------------------------------
#define UART1_PINS    0x6465    // rx=RF4 , tx=RF5
#define UART2_PINS    0x2726    // rx=RB7 , tx=RB6
#define UART3_PINS    0x3d3e    // rx=RC13, tx=RC14
#define UART4_PINS    0x4544    // rx=RD5 , tx=RD4
#define UART5_PINS    0x0000    // NOT USED
#define UART6_PINS    0x0000    // NOT USED

// ---------------------------------------------------------------------------
// SPI
// ---------------------------------------------------------------------------
#define SPI1_PINS     0x4243    // sdi=RD2 ,sdo=RD3
#define SPI1_CS       0x00
#define SPI1_LED      0x00

#define SPI2_PINS     0x7778    // sdi=RG7 ,sdo=RG8
#define SPI2_CS       0x79      // cs=G9
#define SPI2_LED      0x00

#define SPI3_PINS     0x2a29    // sdi=RB10 ,sdo=RB9
#define SPI3_CS       0x2b      // cs=RB11
#define SPI3_LED      0x00

#define SPI4_PINS     0x4b40    // sdi=RD11 ,sdo=RD0
#define SPI4_CS       0x49      // cs=RD9
#define SPI4_LED      0x00

// ---------------------------------------------------------------------------
// EXTERNAL INTERRUPTS
// ---------------------------------------------------------------------------
#define INT0_PIN      0x00
#define INT1_PIN      0x00
#define INT2_PIN      0x2f      // RB15
#define INT3_PIN      0x00
#define INT4_PIN      0x00

// ---------------------------------------------------------------------------
// SDCARD
// ---------------------------------------------------------------------------
#define SD_SPI        1
#define SD_CS         SPI1_CS
#define SD_LED        LED_1

// ---------------------------------------------------------------------------
// CFI
// ---------------------------------------------------------------------------
			
// ---------------------------------------------------------------------------
// DISPLAY
// ---------------------------------------------------------------------------
#define DISPLAY_SPI        2
#define DISPLAY_CS         SPI2_CS  
#define DISPLAY_RE         0x25
#define DISPLAY_RS         0x24

// ---------------------------------------------------------------------------
// CAN
// ---------------------------------------------------------------------------
#define CAN_BUS_SPEED 250000
#define CAN_RX_QUEUE  100

#define CAN1_PINS     0x6061    // rx=RF0, tx=RF1
#define CAN2_PINS     0x0000 

// ---------------------------------------------------------------------------
// Task / threads stack sizes
// ---------------------------------------------------------------------------


#define tcpipTaskStack configMINIMAL_STACK_SIZE * 64
#define defaultStack   configMINIMAL_STACK_SIZE * 4
#define initTaskStack  configMINIMAL_STACK_SIZE * 10
#define netTaskStack   configMINIMAL_STACK_SIZE * 10
#define mqttStack      configMINIMAL_STACK_SIZE * 10
#define ppinTaskStack  configMINIMAL_STACK_SIZE * 10
#define loraTaskStack  configMINIMAL_STACK_SIZE * 10

// ---------------------------------------------------------------------------
// ETHERNET PINS
// ---------------------------------------------------------------------------
#if USE_ETHERNET
    #if PHY_TYPE == PHY_TYPE_LAN8720
        #define USE_PIC_PHY       1
        #define USE_SPI_PHY       0
        #define PHY_CLOCK         2500000
        #define	PHY_ADDRESS       0x01

        #define USE_ETH_ALTERNATE     0     // Use alternate conf

        // RMII pin assignements
        #if USE_ETH_ALTERNATE
            #define ERXD0_PIN         0x49  // RD9
            #define ERXD1_PIN         0x40  // RD0
            #define RXERR_PIN         0x43  // RD3
            #define RXCLK_PIN         0x4B  // RD11
            #define RXDV_PIN          0x52  // RE2
            #define MDC_PIN           0x2F  // RB15
            #define MDIO_PIN          0x41  // RD1
            #define ETXD0_PIN         0x61  // RF1
            #define ETXD1_PIN         0x60  // RF0
            #define ETXEN_PIN         0x42  // RD2
        #else
            #define ERXD0_PIN         0x51  // RE1
            #define ERXD1_PIN         0x50  // RE0
            #define RXERR_PIN         0x54  // RE4
            #define RXCLK_PIN         0x53  // RE3
            #define RXDV_PIN          0x52  // RE2
            #define MDC_PIN           0x2f  // RB15
            #define MDIO_PIN          0x41  // RD1
            #define ETXD0_PIN         0x56  // RE6
            #define ETXD1_PIN         0x57  // RE7
            #define ETXEN_PIN         0x55  // RE5
        #endif
    #else
        #define USE_PIC_PHY       0
        #define USE_SPI_PHY       1
        #define SPI_PHY_SPI       4
        #define SPI_PHY_CS        SPI4_CS
        #define SPI_PHY_INT       PIC32_IRQ_INT2
    #endif
#else
    #define USE_PIC_PHY 0
    #define USE_SPI_PHY 0
#endif

#define USE_FAT     USE_SD
#define USE_SPIFFS  USE_CFI

// ---------------------------------------------------------------------------
// APPLICATION SETTINGS
// ---------------------------------------------------------------------------
#define HISTORY_DEFAULT_STATE 0

#define SHELL_DEFAULT_STATE   0 // Initial LuaOS shell status (1 = on, 0 = off)

// ---------------------------------------------------------------------------
// Lua modules
// ---------------------------------------------------------------------------
#define LUA_USE_CAN         (1 && USE_CAN)
#define LUA_USE_NET         (1 && (USE_ETHERNET || USE_WIFI || USE_GPRS))
#define LUA_USE_ADC         0
#define LUA_USE_SPI         0
#define LUA_USE_MQTT        (1 && (USE_ETHERNET || USE_WIFI || USE_GPRS))
#define LUA_USE_SCREEN      (1 && USE_DISPLAY)
#define LUA_USE_UART        0
#define LUA_USE_PWM         0
#define LUA_USE_GPS         (1 && USE_GPS)
#define LUA_USE_HTTP        (1 && (USE_ETHERNET || USE_WIFI || USE_GPRS))
#define LUA_USE_STEPPER     (1 && USE_STEPPER)
#define LUA_USE_I2C         0
#define LUA_USE_SHELL       0
#define LUA_USE_HISTORY	    0
#endif	/* CONFIG_H */
