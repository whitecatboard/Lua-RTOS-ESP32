/*********************************************************************
 *
 * ENC424J600/624J600 Registers and Bits
 *
 *********************************************************************
 * FileName:        ENCX24J600.h
 * Dependencies:    None
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Howard Schlunder		11/30/07	Original
 ********************************************************************/

#include "sdkconfig.h"

#if CONFIG_LUA_RTOS_ETH_HW_TYPE_SPI

#ifndef __ENCX24J600_H
#define __ENCX24J600_H

#include "lwip/err.h"
#include "lwip/netif.h"
#include "netif/etharp.h"

#include <stdint.h>

// Define macro for 8-bit PSP SFR address translation to SPI addresses
#define ENC100_TRANSLATE_TO_PIN_ADDR(a)		((a) & 0x00FFu)

// ENC424J600 config
#define ENC424J600_RAMSIZE	(0x6000)
#define ENC424J600_TXSTART	(0x0000)
#define ENC424J600_RXSTART	(0x3000) // Should be an even memory address

void enc424j600Init(void);
uint16_t enc424j600PacketReceive(uint16_t maxlen, uint8_t* packet);
void enc424j600PacketSend(uint16_t len, uint8_t* packet);
void enc424j600GetMACAddr(uint8_t addr[6]);
uint16_t enc424j600ReadReg(uint16_t address);
void enc424j600WriteReg(uint16_t address, uint16_t data);
uint16_t enc424j600ReadPHYReg(uint8_t address);
void enc424j600WritePHYReg(uint8_t address, uint16_t Data);

// Crypto memory addresses.  These are accessible by the DMA only and therefore
// have the same addresses no matter what MCU interface is being used (SPI,
// 8-bit PSP, or 16-bit PSP)
#define ENC100_MODEX_Y			(0x7880u)
#define ENC100_MODEX_E			(0x7800u)
#define ENC100_MODEX_X			(0x7880u)
#define ENC100_MODEX_M			(0x7900u)
#define ENC100_HASH_DATA_IN		(0x7A00u)
#define ENC100_HASH_IV_IN		(0x7A40u)
#define ENC100_HASH_LEN_IN		(0x7A54u)
#define ENC100_HASH_DIGEST_OUT          (0x7A70u)
#define ENC100_HASH_LEN_OUT		(0x7A84u)
#define ENC100_HASH_BASE_ADDR           (0x7A00u)
#define ENC100_AES_KEY			(0x7C00u)
#define ENC100_AES_TEXTA		(0x7C20u)
#define ENC100_AES_TEXTB		(0x7C30u)
#define ENC100_AES_XOROUT		(0x7C40u)


// Receive Status Vector bit fields
typedef union __attribute__((aligned(2), packed)) {
	uint8_t v[6];
	struct {
		uint16_t	 		ByteCount;

		unsigned char	PreviouslyIgnored:1;
		unsigned char	RXDCPreviouslySeen:1;
		unsigned char	CarrierPreviouslySeen:1;
		unsigned char	CodeViolation:1;
		unsigned char	CRCError:1;
		unsigned char	LengthCheckError:1;
		unsigned char	LengthOutOfRange:1;
		unsigned char	ReceiveOk:1;
		unsigned char	Multicast:1;
		unsigned char	Broadcast:1;
		unsigned char	DribbleNibble:1;
		unsigned char	ControlFrame:1;
		unsigned char	PauseControlFrame:1;
		unsigned char	UnsupportedOpcode:1;
		unsigned char	VLANType:1;
		unsigned char	RuntMatch:1;

		unsigned char	filler:1;
		unsigned char	HashMatch:1;
		unsigned char	MagicPacketMatch:1;
		unsigned char	PatternMatch:1;
		unsigned char	UnicastMatch:1;
		unsigned char	BroadcastMatch:1;
		unsigned char	MulticastMatch:1;
		unsigned char	ZeroH:1;
		unsigned char	Zero:8;
	} bits;
} RXSTATUS;


////////////////////////////////////////////////////
// ENC424J600/624J600 SPI Opcodes		  //
////////////////////////////////////////////////////
#define RCR 		(0x0u<<5)// Read Control Register
#define WCR		(0x2u<<5)// Write Control Register
#define RCRU		(0x20u)	// Read Control Register Unbanked
#define WCRU		(0x22u)	// Write Control Register Unbanked
#define BFS		(0x4u<<5)// Bit Field Set
#define BFSU		(0x24u)	// Bit Field Set Unbanked
#define BFC		(0x5u<<5)// Bit Field Clear
#define BFCU		(0x26u)	// Bit Field Clear Unbanked
#define RBMGP		(0x28u)	// Read Buffer Memory General Purpose
#define WBMGP		(0x2Au)	// Write Buffer Memory General Purpose
#define RBMRX		(0x2Cu)	// Read Buffer Memory RX
#define WBMRX		(0x2Eu)	// Write Buffer Memory RX
#define RBMUDA		(0x30u)	// Read Buffer Memory User Defined Area
#define WBMUDA		(0x32u)	// Write Buffer Memory User Defined Area
#define WGPRDPT		(0x60u)	// Write General Purpose Read Pointer
#define RGPRDPT		(0x62u)	// Read General Purpose Read Pointer
#define WRXRDPT		(0x64u)	// Write RX Read Pointer
#define RRXRDPT		(0x66u)	// Read RX Read Pointer
#define WUDARDPT	(0x68u)	// Write User Defined Area Read Pointer
#define RUDARDPT	(0x6Au)	// Read User Defined Area Read Pointer
#define WGPWRPT		(0x6Cu)	// Write General Purpose Write Pointer
#define RGPWRPT		(0x6Eu)	// Read General Purpose Write Pointer
#define WRXWRPT		(0x70u)	// Write RX Write Pointer
#define RRXWRPT		(0x72u)	// Read RX Write Pointer
#define	WUDAWRPT	(0x74u)	// Write User Defined Area Write Pointer
#define RUDAWRPT	(0x76u)	// Read User Defined Area Write Pointer
#define B0SEL		(0xC0u)	// Bank 0 Select
#define B1SEL		(0xC2u)	// Bank 1 Select
#define B2SEL		(0xC4u)	// Bank 2 Select
#define B3SEL		(0xC6u)	// Bank 3 Select
#define RBSEL		(0xC8u)	// Read Bank Select
#define SETETHRST	(0xCAu)	// Set ETHRST bit (perform system reset)
#define FCDIS		(0xE0u)	// Flow Control Disable
#define FCSINGLE	(0xE2u)	// Flow Control Single
#define FCMULTIPLE	(0xE4u)	// Flow Control Multiple
#define FCCLEAR		(0xE6u)	// Flow Control Clear
#define SETPKTDEC	(0xCCu)	// Set PKTDEC bit (decrement RX packet pending counter)
#define DMASTOP		(0xD0u)	// DMA Stop
#define DMACKSUM	(0xD8u)	// DMA Start Checksum
#define DMACKSUMS	(0xDAu)	// DMA Start Checksum with Seed
#define DMACOPY		(0xDCu)	// DMA Start Copy
#define DMACOPYS	(0xDEu)	// DMA Start Copy and Checksum with Seed
#define SETTXRTS	(0xD4u)	// Set TXRTS bit (transmit a packet)
#define ENABLERX	(0xE8u)	// Enable RX
#define DISABLERX	(0xEAu)	// Disable RX
#define SETEIE		(0xECu)	// Set Ethernet Interrupt Enable (EIE)
#define CLREIE		(0xEEu)	// Clear Ethernet Interrupt Enable (EIE)


////////////////////////////////////////////////////
// ENC424J600/624J600 register addresses          //
////////////////////////////////////////////////////
// SPI Bank 0 registers --------
#define ETXST		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E00u)
#define ETXSTL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E00u)
#define ETXSTH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E01u)
#define ETXLEN		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E02u)
#define ETXLENL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E02u)
#define ETXLENH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E03u)
#define ERXST		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E04u)
#define ERXSTL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E04u)
#define ERXSTH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E05u)
#define ERXTAIL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E06u)
#define ERXTAILL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E06u)
#define ERXTAILH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E07u)
#define ERXHEAD		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E08u)
#define ERXHEADL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E08u)
#define ERXHEADH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E09u)
#define EDMAST		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Au)
#define EDMASTL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Au)
#define EDMASTH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Bu)
#define EDMALEN		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Cu)
#define EDMALENL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Cu)
#define EDMALENH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Du)
#define EDMADST		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Eu)
#define EDMADSTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Eu)
#define EDMADSTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E0Fu)
#define EDMACS		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E10u)
#define EDMACSL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E10u)
#define EDMACSH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E11u)
#define ETXSTAT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E12u)
#define ETXSTATL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E12u)
#define ETXSTATH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E13u)
#define ETXWIRE		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E14u)
#define ETXWIREL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E14u)
#define ETXWIREH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E15u)

// SPI all bank registers
#define EUDAST		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E16u)
#define EUDASTL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E16u)
#define EUDASTH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E17u)
#define EUDAND		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E18u)
#define EUDANDL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E18u)
#define EUDANDH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E19u)
#define ESTAT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Au)
#define ESTATL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Au)
#define ESTATH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Bu)
#define EIR		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Cu)
#define EIRL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Cu)
#define EIRH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Du)
#define ECON1		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Eu)
#define ECON1L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Eu)
#define ECON1H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E1Fu)


// SPI Bank 1 registers -----
#define EHT1		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E20u)
#define EHT1L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E20u)
#define EHT1H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E21u)
#define EHT2		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E22u)
#define EHT2L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E22u)
#define EHT2H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E23u)
#define EHT3		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E24u)
#define EHT3L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E24u)
#define EHT3H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E25u)
#define EHT4		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E26u)
#define EHT4L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E26u)
#define EHT4H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E27u)
#define EPMM1		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E28u)
#define EPMM1L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E28u)
#define EPMM1H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E29u)
#define EPMM2		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Au)
#define EPMM2L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Au)
#define EPMM2H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Bu)
#define EPMM3		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Cu)
#define EPMM3L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Cu)
#define EPMM3H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Du)
#define EPMM4		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Eu)
#define EPMM4L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Eu)
#define EPMM4H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E2Fu)
#define EPMCS		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E30u)
#define EPMCSL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E30u)
#define EPMCSH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E31u)
#define EPMO		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E32u)
#define EPMOL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E32u)
#define EPMOH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E33u)
#define ERXFCON		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E34u)
#define ERXFCONL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E34u)
#define ERXFCONH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E35u)

// SPI all bank registers from 0x36 to 0x3F


// SPI Bank 2 registers -----
#define MACON1		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E40u)
#define MACON1L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E40u)
#define MACON1H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E41u)
#define MACON2		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E42u)
#define MACON2L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E42u)
#define MACON2H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E43u)
#define MABBIPG		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E44u)
#define MABBIPGL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E44u)
#define MABBIPGH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E45u)
#define MAIPG		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E46u)
#define MAIPGL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E46u)
#define MAIPGH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E47u)
#define MACLCON		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E48u)
#define MACLCONL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E48u)
#define MACLCONH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E49u)
#define MAMXFL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E4Au)
#define MAMXFLL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E4Au)
#define MAMXFLH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E4Bu)
#define MICMD		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E52u)
#define MICMDL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E52u)
#define MICMDH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E53u)
#define MIREGADR	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E54u)
#define MIREGADRL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E54u)
#define MIREGADRH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E55u)

// SPI all bank registers from 0x56 to 0x5F


// SPI Bank 3 registers -----
#define MAADR3		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E60u)
#define MAADR3L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E60u)
#define MAADR3H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E61u)
#define MAADR2		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E62u)
#define MAADR2L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E62u)
#define MAADR2H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E63u)
#define MAADR1		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E64u)
#define MAADR1L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E64u)
#define MAADR1H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E65u)
#define MIWR		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E66u)
#define MIWRL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E66u)
#define MIWRH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E67u)
#define MIRD		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E68u)
#define MIRDL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E68u)
#define MIRDH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E69u)
#define MISTAT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Au)
#define MISTATL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Au)
#define MISTATH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Bu)
#define EPAUS		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Cu)
#define EPAUSL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Cu)
#define EPAUSH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Du)
#define ECON2		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Eu)
#define ECON2L		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Eu)
#define ECON2H		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E6Fu)
#define ERXWM		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E70u)
#define ERXWML		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E70u)
#define ERXWMH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E71u)
#define EIE		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E72u)
#define EIEL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E72u)
#define EIEH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E73u)
#define EIDLED		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E74u)
#define EIDLEDL		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E74u)
#define EIDLEDH		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E75u)

// SPI all bank registers from 0x66 to 0x6F


// SPI Non-banked Special Function Registers
#define EGPDATA		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E80u)
#define EGPDATAL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E80u)
//#define r			ENC100_TRANSLATE_TO_PIN_ADDR(0x7E81u)
#define ERXDATA		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E82u)
#define ERXDATAL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E82u)
//#define r			ENC100_TRANSLATE_TO_PIN_ADDR(0x7E83u)
#define EUDADATA	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E84u)
#define EUDADATAL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E84u)
//#define r			ENC100_TRANSLATE_TO_PIN_ADDR(0x7E85u)
#define EGPRDPT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E86u)
#define EGPRDPTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E86u)
#define EGPRDPTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E87u)
#define EGPWRPT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E88u)
#define EGPWRPTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E88u)
#define EGPWRPTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E89u)
#define ERXRDPT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Au)
#define ERXRDPTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Au)
#define ERXRDPTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Bu)
#define ERXWRPT		ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Cu)
#define ERXWRPTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Cu)
#define ERXWRPTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Du)
#define EUDARDPT	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Eu)
#define EUDARDPTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Eu)
#define EUDARDPTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E8Fu)
#define EUDAWRPT	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E90u)
#define EUDAWRPTL	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E90u)
#define EUDAWRPTH	ENC100_TRANSLATE_TO_PIN_ADDR(0x7E91u)



////////////////////////////////////////////////////
// ENC424J600/624J600 PHY Register Addresses	  //
////////////////////////////////////////////////////
#define PHCON1	0x00u
#define PHSTAT1	0x01u
#define PHANA	0x04u
#define PHANLPA	0x05u
#define PHANE	0x06u
#define PHCON2	0x11u
#define PHSTAT2	0x1Bu
#define PHSTAT3	0x1Fu



////////////////////////////////////////////////////
// ENC424J600/624J600 register bits				  //
////////////////////////////////////////////////////
// ESTAT bits ----------
#define ESTAT_INT		((uint16_t)1<<15)
#define ESTAT_FCIDLE            ((uint16_t)1<<14)
#define ESTAT_RXBUSY            ((uint16_t)1<<13)
#define ESTAT_CLKRDY            ((uint16_t)1<<12)
#define ESTAT_RSTDONE           ((uint16_t)1<<11)
#define ESTAT_PHYDPX            ((uint16_t)1<<10)
#define ESTAT_PHYRDY            ((uint16_t)1<<9)
#define ESTAT_PHYLNK            ((uint16_t)1<<8)
#define ESTAT_PKTCNT7           (1<<7)
#define ESTAT_PKTCNT6           (1<<6)
#define ESTAT_PKTCNT5           (1<<5)
#define ESTAT_PKTCNT4           (1<<4)
#define ESTAT_PKTCNT3           (1<<3)
#define ESTAT_PKTCNT2           (1<<2)
#define ESTAT_PKTCNT1           (1<<1)
#define ESTAT_PKTCNT0           (1)

// EIR bits ------------
#define EIR_CRYPTEN		((uint16_t)1<<15)
#define EIR_MODEXIF		((uint16_t)1<<14)
#define EIR_HASHIF		((uint16_t)1<<13)
#define EIR_AESIF		((uint16_t)1<<12)
#define EIR_LINKIF		((uint16_t)1<<11)
#define EIR_PRDYIF		((uint16_t)1<<10)
#define EIR_r9			((uint16_t)1<<9)
#define EIR_r8			((uint16_t)1<<8)
#define EIR_r7			(1<<7)
#define EIR_PKTIF		(1<<6)
#define EIR_DMAIF		(1<<5)
#define EIR_r4			(1<<4)
#define EIR_TXIF		(1<<3)
#define EIR_TXABTIF		(1<<2)
#define EIR_RXABTIF		(1<<1)
#define EIR_PCFULIF		(1)

// ECON1 bits ----------
#define ECON1_MODEXST           ((uint16_t)1<<15)
#define ECON1_HASHEN            ((uint16_t)1<<14)
#define ECON1_HASHOP            ((uint16_t)1<<13)
#define ECON1_HASHLST           ((uint16_t)1<<12)
#define ECON1_AESST		((uint16_t)1<<11)
#define ECON1_AESOP1            ((uint16_t)1<<10)
#define ECON1_AESOP0            ((uint16_t)1<<9)
#define ECON1_PKTDEC            ((uint16_t)1<<8)
#define ECON1_FCOP1		(1<<7)
#define ECON1_FCOP0		(1<<6)
#define ECON1_DMAST		(1<<5)
#define ECON1_DMACPY            (1<<4)
#define ECON1_DMACSSD           (1<<3)
#define ECON1_DMANOCS           (1<<2)
#define ECON1_TXRTS		(1<<1)
#define ECON1_RXEN		(1)

// ETXSTAT bits --------
#define ETXSTAT_r12		((uint16_t)1<<12)
#define ETXSTAT_r11		((uint16_t)1<<11)
#define ETXSTAT_LATECOL         ((uint16_t)1<<10)
#define ETXSTAT_MAXCOL          ((uint16_t)1<<9)
#define ETXSTAT_EXDEFER         ((uint16_t)1<<8)
#define ETXSTAT_DEFER           (1<<7)
#define ETXSTAT_r6		(1<<6)
#define ETXSTAT_r5		(1<<5)
#define ETXSTAT_CRCBAD          (1<<4)
#define ETXSTAT_COLCNT3         (1<<3)
#define ETXSTAT_COLCNT2         (1<<2)
#define ETXSTAT_COLCNT1         (1<<1)
#define ETXSTAT_COLCNT0         (1)

// ERXFCON bits --------
#define ERXFCON_HTEN            ((uint16_t)1<<15)
#define ERXFCON_MPEN            ((uint16_t)1<<14)
#define ERXFCON_NOTPM           ((uint16_t)1<<12)
#define ERXFCON_PMEN3           ((uint16_t)1<<11)
#define ERXFCON_PMEN2           ((uint16_t)1<<10)
#define ERXFCON_PMEN1           ((uint16_t)1<<9)
#define ERXFCON_PMEN0           ((uint16_t)1<<8)
#define ERXFCON_CRCEEN          (1<<7)
#define ERXFCON_CRCEN           (1<<6)
#define ERXFCON_RUNTEEN         (1<<5)
#define ERXFCON_RUNTEN          (1<<4)
#define ERXFCON_UCEN            (1<<3)
#define ERXFCON_NOTMEEN         (1<<2)
#define ERXFCON_MCEN            (1<<1)
#define ERXFCON_BCEN            (1)

// MACON1 bits ---------
#define MACON1_r15		((uint16_t)1<<15)
#define MACON1_r14		((uint16_t)1<<14)
#define MACON1_r11		((uint16_t)1<<11)
#define MACON1_r10		((uint16_t)1<<10)
#define MACON1_r9		((uint16_t)1<<9)
#define MACON1_r8		((uint16_t)1<<8)
#define MACON1_LOOPBK           (1<<4)
#define MACON1_r3		(1<<3)
#define	MACON1_RXPAUS           (1<<2)
#define	MACON1_PASSALL          (1<<1)
#define MACON1_r0		(1)

// MACON2 bits ---------
#define	MACON2_DEFER            ((uint16_t)1<<14)
#define	MACON2_BPEN		((uint16_t)1<<13)
#define	MACON2_NOBKOFF          ((uint16_t)1<<12)
#define MACON2_r9		((uint16_t)1<<9)
#define MACON2_r8		((uint16_t)1<<8)
#define	MACON2_PADCFG2          (1<<7)
#define	MACON2_PADCFG1          (1<<6)
#define	MACON2_PADCFG0          (1<<5)
#define	MACON2_TXCRCEN          (1<<4)
#define	MACON2_PHDREN           (1<<3)
#define	MACON2_HFRMEN           (1<<2)
#define MACON2_r1		(1<<1)
#define	MACON2_FULDPX           (1)

// MABBIPG bits --------
#define MABBIPG_BBIPG6          (1<<6)
#define MABBIPG_BBIPG5          (1<<5)
#define MABBIPG_BBIPG4          (1<<4)
#define MABBIPG_BBIPG3          (1<<3)
#define MABBIPG_BBIPG2          (1<<2)
#define MABBIPG_BBIPG1          (1<<1)
#define MABBIPG_BBIPG0          (1)

// MAIPG bits ----------
#define MAIPG_r14		((uint16_t)1<<14)
#define MAIPG_r13		((uint16_t)1<<13)
#define MAIPG_r12		((uint16_t)1<<12)
#define MAIPG_r11		((uint16_t)1<<11)
#define MAIPG_r10		((uint16_t)1<<10)
#define MAIPG_r9		((uint16_t)1<<9)
#define MAIPG_r8		((uint16_t)1<<8)
#define MAIPG_IPG6		(1<<6)
#define MAIPG_IPG5		(1<<5)
#define MAIPG_IPG4		(1<<4)
#define MAIPG_IPG3		(1<<3)
#define MAIPG_IPG2		(1<<2)
#define MAIPG_IPG1		(1<<1)
#define MAIPG_IPG0		(1)

// MACLCON bits --------
#define MACLCON_r13		((uint16_t)1<<13)
#define MACLCON_r12		((uint16_t)1<<12)
#define MACLCON_r11		((uint16_t)1<<11)
#define MACLCON_r10		((uint16_t)1<<10)
#define MACLCON_r9		((uint16_t)1<<9)
#define MACLCON_r8		((uint16_t)1<<8)
#define MACLCON_MAXRET3         (1<<3)
#define MACLCON_MAXRET2         (1<<2)
#define MACLCON_MAXRET1         (1<<1)
#define MACLCON_MAXRET0         (1)

// MICMD bits ----------
#define	MICMD_MIISCAN           (1<<1)
#define	MICMD_MIIRD		(1)

// MIREGADR bits -------
#define MIREGADR_r12            ((uint16_t)1<<12)
#define MIREGADR_r11            ((uint16_t)1<<11)
#define MIREGADR_r10            ((uint16_t)1<<10)
#define MIREGADR_r9		((uint16_t)1<<9)
#define MIREGADR_r8		((uint16_t)1<<8)
#define MIREGADR_PHREG4         (1<<4)
#define MIREGADR_PHREG3         (1<<3)
#define MIREGADR_PHREG2         (1<<2)
#define MIREGADR_PHREG1         (1<<1)
#define MIREGADR_PHREG0         (1)

// MISTAT bits ---------
#define MISTAT_r3		(1<<3)
#define	MISTAT_NVALID           (1<<2)
#define	MISTAT_SCAN		(1<<1)
#define	MISTAT_BUSY		(1)

// ECON2 bits ----------
#define ECON2_ETHEN		((uint16_t)1<<15)
#define ECON2_STRCH		((uint16_t)1<<14)
#define ECON2_TXMAC		((uint16_t)1<<13)
#define ECON2_SHA1MD5           ((uint16_t)1<<12)
#define ECON2_COCON3            ((uint16_t)1<<11)
#define ECON2_COCON2            ((uint16_t)1<<10)
#define ECON2_COCON1            ((uint16_t)1<<9)
#define ECON2_COCON0            ((uint16_t)1<<8)
#define ECON2_AUTOFC            (1<<7)
#define ECON2_TXRST		(1<<6)
#define ECON2_RXRST		(1<<5)
#define ECON2_ETHRST            (1<<4)
#define ECON2_MODLEN1           (1<<3)
#define ECON2_MODLEN0           (1<<2)
#define ECON2_AESLEN1           (1<<1)
#define ECON2_AESLEN0           (1)

// ERXWM bits ----------
#define ERXWM_RXFWM7            ((uint16_t)1<<15)
#define ERXWM_RXFWM6            ((uint16_t)1<<14)
#define ERXWM_RXFWM5            ((uint16_t)1<<13)
#define ERXWM_RXFWM4            ((uint16_t)1<<12)
#define ERXWM_RXFWM3            ((uint16_t)1<<11)
#define ERXWM_RXFWM2            ((uint16_t)1<<10)
#define ERXWM_RXFWM1            ((uint16_t)1<<9)
#define ERXWM_RXFWM0            ((uint16_t)1<<8)
#define ERXWM_RXEWM7            (1<<7)
#define ERXWM_RXEWM6            (1<<6)
#define ERXWM_RXEWM5            (1<<5)
#define ERXWM_RXEWM4            (1<<4)
#define ERXWM_RXEWM3            (1<<3)
#define ERXWM_RXEWM2            (1<<2)
#define ERXWM_RXEWM1            (1<<1)
#define ERXWM_RXEWM0            (1)

// EIE bits ------------
#define EIE_INTIE		((uint16_t)1<<15)
#define EIE_MODEXIE		((uint16_t)1<<14)
#define EIE_HASHIE		((uint16_t)1<<13)
#define EIE_AESIE		((uint16_t)1<<12)
#define EIE_LINKIE		((uint16_t)1<<11)
#define EIE_PRDYIE		((uint16_t)1<<10)
#define EIE_r9			((uint16_t)1<<9)
#define EIE_r8			((uint16_t)1<<8)
#define EIE_r7			(1<<7)
#define EIE_PKTIE		(1<<6)
#define EIE_DMAIE		(1<<5)
#define EIE_r4			(1<<4)
#define EIE_TXIE		(1<<3)
#define EIE_TXABTIE		(1<<2)
#define EIE_RXABTIE		(1<<1)
#define EIE_PCFULIE		(1)

// EIDLED bits ---------
#define EIDLED_LACFG3           ((uint16_t)1<<15)
#define EIDLED_LACFG2           ((uint16_t)1<<14)
#define EIDLED_LACFG1           ((uint16_t)1<<13)
#define EIDLED_LACFG0           ((uint16_t)1<<12)
#define EIDLED_LBCFG3           ((uint16_t)1<<11)
#define EIDLED_LBCFG2           ((uint16_t)1<<10)
#define EIDLED_LBCFG1           ((uint16_t)1<<9)
#define EIDLED_LBCFG0           ((uint16_t)1<<8)
#define EIDLED_DEVID2           (1<<7)
#define EIDLED_DEVID1           (1<<6)
#define EIDLED_DEVID0           (1<<5)
#define EIDLED_REVID4           (1<<4)
#define EIDLED_REVID3           (1<<3)
#define EIDLED_REVID2           (1<<2)
#define EIDLED_REVID1           (1<<1)
#define EIDLED_REVID0           (1)

// PHCON1 bits ---------
#define PHCON1_PRST		((uint16_t)1<<15)
#define PHCON1_PLOOPBK          ((uint16_t)1<<14)
#define PHCON1_SPD100           ((uint16_t)1<<13)
#define PHCON1_ANEN		((uint16_t)1<<12)
#define PHCON1_PSLEEP           ((uint16_t)1<<11)
#define PHCON1_r10		((uint16_t)1<<10)
#define PHCON1_RENEG            ((uint16_t)1<<9)
#define PHCON1_PFULDPX          ((uint16_t)1<<8)
#define PHCON1_r7		(1<<7)
#define PHCON1_r6		(1<<6)
#define PHCON1_r5		(1<<5)
#define PHCON1_r4		(1<<4)
#define PHCON1_r3		(1<<3)
#define PHCON1_r2		(1<<2)
#define PHCON1_r1		(1<<1)
#define PHCON1_r0		(1)

// PHSTAT1 bits --------
#define PHSTAT1_r15		((uint16_t)1<<15)
#define PHSTAT1_FULL100         ((uint16_t)1<<14)
#define PHSTAT1_HALF100         ((uint16_t)1<<13)
#define PHSTAT1_FULL10          ((uint16_t)1<<12)
#define PHSTAT1_HALF10          ((uint16_t)1<<11)
#define PHSTAT1_r10		((uint16_t)1<<10)
#define PHSTAT1_r9		((uint16_t)1<<9)
#define PHSTAT1_r8		((uint16_t)1<<8)
#define PHSTAT1_r7		(1<<7)
#define PHSTAT1_r6		(1<<6)
#define PHSTAT1_ANDONE          (1<<5)
#define PHSTAT1_LRFAULT         (1<<4)
#define PHSTAT1_ANABLE          (1<<3)
#define PHSTAT1_LLSTAT          (1<<2)
#define PHSTAT1_r1		(1<<1)
#define PHSTAT1_EXTREGS         (1)

// PHANA bits ----------
#define PHANA_ADNP		((uint16_t)1<<15)
#define PHANA_r14		((uint16_t)1<<14)
#define PHANA_ADFAULT           ((uint16_t)1<<13)
#define PHANA_r12		((uint16_t)1<<12)
#define PHANA_ADPAUS1           ((uint16_t)1<<11)
#define PHANA_ADPAUS0           ((uint16_t)1<<10)
#define PHANA_r9		((uint16_t)1<<9)
#define PHANA_AD100FD           ((uint16_t)1<<8)
#define PHANA_AD100		(1<<7)
#define PHANA_AD10FD            (1<<6)
#define PHANA_AD10		(1<<5)
#define PHANA_ADIEEE4           (1<<4)
#define PHANA_ADIEEE3           (1<<3)
#define PHANA_ADIEEE2           (1<<2)
#define PHANA_ADIEEE1           (1<<1)
#define PHANA_ADIEEE0           (1)

// PHANLPA bits --------
#define PHANLPA_LPNP            ((uint16_t)1<<15)
#define PHANLPA_LPACK           ((uint16_t)1<<14)
#define PHANLPA_LPFAULT 	((uint16_t)1<<13)
#define PHANLPA_r12		((uint16_t)1<<12)
#define PHANLPA_LPPAUS1         ((uint16_t)1<<11)
#define PHANLPA_LPPAUS0     	((uint16_t)1<<10)
#define PHANLPA_LP100T4     	((uint16_t)1<<9)
#define PHANLPA_LP100FD         ((uint16_t)1<<8)
#define PHANLPA_LP100           (1<<7)
#define PHANLPA_LP10FD          (1<<6)
#define PHANLPA_LP10            (1<<5)
#define PHANLPA_LPIEEE4         (1<<4)
#define PHANLPA_LPIEEE3         (1<<3)
#define PHANLPA_LPIEEE2         (1<<2)
#define PHANLPA_LPIEEE1         (1<<1)
#define PHANLPA_LPIEEE0         (1)

// PHANE bits ----------
#define PHANE_r15		((uint16_t)1<<15)
#define PHANE_r14		((uint16_t)1<<14)
#define PHANE_r13		((uint16_t)1<<13)
#define PHANE_r12		((uint16_t)1<<12)
#define PHANE_r11		((uint16_t)1<<11)
#define PHANE_r10		((uint16_t)1<<10)
#define PHANE_r9		((uint16_t)1<<9)
#define PHANE_r8		((uint16_t)1<<8)
#define PHANE_r7		(1<<7)
#define PHANE_r6		(1<<6)
#define PHANE_r5		(1<<5)
#define PHANE_PDFLT		(1<<4)
#define PHANE_r3		(1<<3)
#define PHANE_r2		(1<<2)
#define PHANE_LPARCD            (1<<1)
#define PHANA_LPANABL       	(1)

// PHCON2 bits ---------
#define PHCON2_r15		((uint16_t)1<<15)
#define PHCON2_r14		((uint16_t)1<<14)
#define PHCON2_EDPWRDN          ((uint16_t)1<<13)
#define PHCON2_r12		((uint16_t)1<<12)
#define PHCON2_EDTHRES          ((uint16_t)1<<11)
#define PHCON2_r10		((uint16_t)1<<10)
#define PHCON2_r9		((uint16_t)1<<9)
#define PHCON2_r8		((uint16_t)1<<8)
#define PHCON2_r7		(1<<7)
#define PHCON2_r6		(1<<6)
#define PHCON2_r5		(1<<5)
#define PHCON2_r4		(1<<4)
#define PHCON2_r3		(1<<3)
#define PHCON2_FRCLNK           (1<<2)
#define PHCON2_EDSTAT           (1<<1)
#define PHCON2_r0		(1)

// PHSTAT2 bits ---------
#define PHSTAT2_r15		((uint16_t)1<<15)
#define PHSTAT2_r14		((uint16_t)1<<14)
#define PHSTAT2_r13		((uint16_t)1<<13)
#define PHSTAT2_r12		((uint16_t)1<<12)
#define PHSTAT2_r11		((uint16_t)1<<11)
#define PHSTAT2_r10		((uint16_t)1<<10)
#define PHSTAT2_r9		((uint16_t)1<<9)
#define PHSTAT2_r8		((uint16_t)1<<8)
#define PHSTAT2_r7		(1<<7)
#define PHSTAT2_r6		(1<<6)
#define PHSTAT2_r5		(1<<5)
#define PHSTAT2_PLRITY          (1<<4)
#define PHSTAT2_r3		(1<<3)
#define PHSTAT2_r2		(1<<2)
#define PHSTAT2_r1		(1<<1)
#define PHSTAT2_r0		(1)

// PHSTAT3 bits --------
#define PHSTAT3_r15		((uint16_t)1<<15)
#define PHSTAT3_r14		((uint16_t)1<<14)
#define PHSTAT3_r13		((uint16_t)1<<13)
#define PHSTAT3_r12		((uint16_t)1<<12)
#define PHSTAT3_r11		((uint16_t)1<<11)
#define PHSTAT3_r10		((uint16_t)1<<10)
#define PHSTAT3_r9		((uint16_t)1<<9)
#define PHSTAT3_r8		((uint16_t)1<<8)
#define PHSTAT3_r7		(1<<7)
#define PHSTAT3_r6		(1<<6)
#define PHSTAT3_r5		(1<<5)
#define PHSTAT3_SPDDPX2 	(1<<4)
#define PHSTAT3_SPDDPX1         (1<<3)
#define PHSTAT3_SPDDPX0         (1<<2)
#define PHSTAT3_r1		(1<<1)
#define PHSTAT3_r0		(1)

int enc424j600_init(struct netif *netif);
err_t enc424j600_output(struct netif *netif, struct pbuf *p);
struct pbuf *enc424j600_input(struct netif *netif);

#endif

#endif
