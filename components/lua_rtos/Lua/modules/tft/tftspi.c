/* Lua-RTOS-ESP32 TFT module
 * SPI access functions
 * Author: LoBo (loboris@gmail.com, loboris.github)
 *
 * Module supporting SPI TFT displays based on ILI9341 & ST7735 controllers
*/

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_TFT

#include "error.h"

#include "freertos/FreeRTOS.h"
#include "tft/tftspi.h"
#include "freertos/task.h"
#include "stdio.h"
#include <sys/driver.h>
#include <drivers/gpio.h>
#include <sys/syslog.h>

uint16_t *tft_line = NULL;
uint16_t _width = 320;
uint16_t _height = 240;

static int colstart = 0;
static int rowstart = 0;	// May be overridden in init func

static int disp_spi;
static int ts_spi;

int TFT_type = -1;

// =======================================================================================
// ==== Display driver specific functions ================================================
// =======================================================================================

// Prior to executing any of the following functions
// CS has to be activated using spi_device_select() function

// some constants used by display driver
#define TFT_CASET      0x2A
#define TFT_PASET      0x2B
#define TFT_RAMWR      0x2C
#define TFT_RAMRD      0x2E

// Send 1 byte display command
//----------------------------------------------------------------------------
void IRAM_ATTR disp_spi_transfer_cmd(int deviceid, int8_t cmd) {
	gpio_ll_pin_clr(CONFIG_LUA_RTOS_TFT_CMD);
	spi_ll_transfer(deviceid, cmd, NULL);
}

// Set the address window for display write & read commands
//------------------------------------------------------------------------------------------------------------------------
void IRAM_ATTR disp_spi_transfer_addrwin(int deviceid, uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2) {
	uint32_t wd;

	disp_spi_transfer_cmd(deviceid, TFT_CASET);

	wd = (uint32_t)(x1>>8);
	wd |= (uint32_t)(x1&0xff) << 8;
	wd |= (uint32_t)(x2>>8) << 16;
	wd |= (uint32_t)(x2&0xff) << 24;

    // Set DC to 1 (data mode);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_TFT_CMD);
	spi_ll_bulk_write32(deviceid, 1, &wd);

    disp_spi_transfer_cmd(deviceid, TFT_PASET);

	wd = (uint32_t)(y1>>8);
	wd |= (uint32_t)(y1&0xff) << 8;
	wd |= (uint32_t)(y2>>8) << 16;
	wd |= (uint32_t)(y2&0xff) << 24;

    // Set DC to 1 (data mode);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_TFT_CMD);
	spi_ll_bulk_write32(deviceid, 1, &wd);
}

// Set one display pixel to given color, address window already set
//----------------------------------------------------------------------------------
void IRAM_ATTR disp_spi_transfer_pixel(int deviceid, uint16_t color) {
	uint32_t wd;

	disp_spi_transfer_cmd(deviceid, TFT_RAMWR);

	//wd = (uint32_t)color;
	wd = (uint32_t)(color >> 8);
	wd |= (uint32_t)(color & 0xff) << 8;

    // Set DC to 1 (data mode);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_TFT_CMD);
	spi_ll_bulk_write16(deviceid, 1, (uint16_t *)&wd);
}

// Set one display pixel at given coordinates to given color
//-----------------------------------------------------------------------------------------------------
void IRAM_ATTR disp_spi_set_pixel(int deviceid, uint16_t x, uint16_t y, uint16_t color) {
	disp_spi_transfer_addrwin(deviceid, x, x+1, y, y+1);
	disp_spi_transfer_pixel(deviceid, color);
}

// If rep==true  repeat sending color data to display 'len' times
// If rep==false send 'len' color data from color buffer to display
// address window must be already set
//---------------------------------------------------------------------------------------------------------------
void IRAM_ATTR disp_spi_transfer_color_rep(int deviceid, uint8_t *color, uint32_t len, uint8_t rep) {
	int i;
	uint16_t *buffer;

	if (rep) {
		buffer = (uint16_t *)malloc(sizeof(uint16_t) * 1024);

		for(i=0;i < 1024;i++) {
			buffer[i] = *((uint16_t *)color);
		}
	} else {
		buffer = (uint16_t *)color;
	}

	disp_spi_transfer_cmd(deviceid, TFT_RAMWR);

    // Set DC to 1 (data mode);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_TFT_CMD);

	if (rep) {
		int clen;
		while (len) {
			clen = (len >1024?1024:len);
			spi_ll_bulk_write16(deviceid, clen, buffer);
			len = len - clen;
		}
		free(buffer);
	} else {
		spi_ll_bulk_write16(deviceid, len, buffer);
	}
}

//==============================================================================

#define DELAY 0x80

// ======== Low level TFT SPI functions ========================================

// ===========================================================================
// !!IMPORTANT!! spi_device_select & spi_device_deselect must be used in pairs
// ===========================================================================

//Send a command to the TFT.
//-----------------------------
void tft_cmd(const uint8_t cmd)
{
	spi_ll_select(disp_spi);

	// Set DC to 0 (command mode);
	gpio_ll_pin_clr(CONFIG_LUA_RTOS_TFT_CMD);

	disp_spi_transfer_cmd(disp_spi, cmd);
    spi_ll_deselect(disp_spi);
}

//Send command data to the TFT.
//-----------------------------------------
void tft_data(const uint8_t *data, int len)
{
    if (len==0) return;             //no need to send anything

	spi_ll_select(disp_spi);

	// Set DC to 1 (data mode);
	gpio_ll_pin_set(CONFIG_LUA_RTOS_TFT_CMD);

	spi_ll_bulk_write(disp_spi, len, (uint8_t *)data);
	spi_ll_deselect(disp_spi);
}

// Draw pixel on TFT on x,y position using given color
//---------------------------------------------------------------
void drawPixel(int16_t x, int16_t y, uint16_t color, uint8_t sel)
{
	if (sel) {
		spi_ll_select(disp_spi);
	}

	// ** Send pixel color **
	disp_spi_set_pixel(disp_spi, x, y, color);

	if (sel) spi_ll_deselect(disp_spi);
}

// Write 'len' 16-bit color data to TFT 'window' (x1,y2),(x2,y2)
// uses the buffer to fill the color values
//---------------------------------------------------------------------------------
void TFT_pushColorRep(int x1, int y1, int x2, int y2, uint16_t color, uint32_t len)
{
	uint16_t ccolor = color;
	spi_ll_select(disp_spi);

	// ** Send address window **
	disp_spi_transfer_addrwin(disp_spi, x1, x2, y1, y2);

	// ** Send repeated pixel color **
	disp_spi_transfer_color_rep(disp_spi, (uint8_t *)&ccolor, len, 1);

	spi_ll_deselect(disp_spi);
}

// Write 'len' 16-bit color data to TFT 'window' (x1,y2),(x2,y2) from given buffer
//-------------------------------------------------------------------------
void send_data(int x1, int y1, int x2, int y2, uint32_t len, uint16_t *buf)
{
	spi_ll_select(disp_spi);

	// ** Send address window **
	disp_spi_transfer_addrwin(disp_spi, x1, x2, y1, y2);

	// ** Send pixel buffer **
	disp_spi_transfer_color_rep(disp_spi, (uint8_t *)buf, len, 0);

	spi_ll_deselect(disp_spi);
}

// Reads one pixel/color from the TFT's GRAM
//--------------------------------------
uint16_t readPixel(int16_t x, int16_t y)
{
	uint8_t inbuf[4] = {0};

	spi_ll_select(disp_spi);

	// ** Send address window **
	disp_spi_transfer_addrwin(disp_spi, x, x+1, y, y+1);

    // ** GET pixel color **
	disp_spi_transfer_cmd(disp_spi, TFT_RAMRD);

	spi_ll_bulk_read(disp_spi, 4, inbuf);

	spi_ll_deselect(disp_spi);

	//printf("READ DATA: [%02x, %02x, %02x, %02x]\r\n", inbuf[0],inbuf[1],inbuf[2],inbuf[3]);
    return (uint16_t)((uint16_t)((inbuf[1] & 0xF8) << 8) | (uint16_t)((inbuf[2] & 0xFC) << 3) | (uint16_t)(inbuf[3] >> 3));
}

// Reads pixels/colors from the TFT's GRAM
//-------------------------------------------------------------------
int read_data(int x1, int y1, int x2, int y2, int len, uint8_t *buf)
{
	memset(buf, 0, len*2);

	uint8_t *rbuf = malloc((len*3)+1);
    if (!rbuf) return -1;

    memset(rbuf, 0, (len*3)+1);

	spi_ll_select(disp_spi);

    // ** Send address window **
	disp_spi_transfer_addrwin(disp_spi, x1, x2, y1, y2);

    // ** GET pixels/colors **
	disp_spi_transfer_cmd(disp_spi, TFT_RAMRD);

	spi_ll_bulk_read(disp_spi, (len*3)+1, rbuf);

	spi_ll_deselect(disp_spi);

    int idx = 0;
    uint16_t color;
    for (int i=1; i<(len*3); i+=3) {
    	color = (uint16_t)((uint16_t)((rbuf[i] & 0xF8) << 8) | (uint16_t)((rbuf[i+1] & 0xFC) << 3) | (uint16_t)(rbuf[i+2] >> 3));
    	buf[idx++] = color >> 8;
    	buf[idx++] = color & 0xFF;
    }
    free(rbuf);

    return 0;
}

//-----------------------------------
uint16_t touch_get_data(uint8_t type)
{
	uint8_t cmd[3] = {0};

	cmd[0] = type;

	spi_ll_select(ts_spi);

	spi_ll_bulk_rw(ts_spi, 3, cmd);

	spi_ll_deselect(ts_spi);

	//printf("TOUCH: cmd=%02x, data: [%02x %02x %02x]\r\n", type, rxbuf[0], rxbuf[1], rxbuf[2]);
    return (((uint16_t)(cmd[1] << 8) | (uint16_t)(cmd[0])) >> 4);
}

//======== Display initialization data =========================================



// Initialization commands for 7735B screens
// -----------------------------------------
static const uint8_t Bcmd[] = {
  18,						// 18 commands in list:
  ST7735_SWRESET,   DELAY,	//  1: Software reset, no args, w/delay
  50,						//     50 ms delay
  ST7735_SLPOUT ,   DELAY,	//  2: Out of sleep mode, no args, w/delay
  255,						//     255 = 500 ms delay
  ST7735_COLMOD , 1+DELAY,	//  3: Set color mode, 1 arg + delay:
  0x05,						//     16-bit color 5-6-5 color format
  10,						//     10 ms delay
  ST7735_FRMCTR1, 3+DELAY,	//  4: Frame rate control, 3 args + delay:
  0x00,						//     fastest refresh
  0x06,						//     6 lines front porch
  0x03,						//     3 lines back porch
  10,						//     10 ms delay
  TFT_MADCTL , 1      ,		//  5: Memory access ctrl (directions), 1 arg:
  0x08,						//     Row addr/col addr, bottom to top refresh
  ST7735_DISSET5, 2      ,	//  6: Display settings #5, 2 args, no delay:
  0x15,						//     1 clk cycle nonoverlap, 2 cycle gate
  // rise, 3 cycle osc equalize
  0x02,						//     Fix on VTL
  ST7735_INVCTR , 1      ,	//  7: Display inversion control, 1 arg:
  0x0,						//     Line inversion
  ST7735_PWCTR1 , 2+DELAY,	//  8: Power control, 2 args + delay:
  0x02,						//     GVDD = 4.7V
  0x70,						//     1.0uA
  10,						//     10 ms delay
  ST7735_PWCTR2 , 1      ,	//  9: Power control, 1 arg, no delay:
  0x05,						//     VGH = 14.7V, VGL = -7.35V
  ST7735_PWCTR3 , 2      ,	// 10: Power control, 2 args, no delay:
  0x01,						//     Opamp current small
  0x02,						//     Boost frequency
  ST7735_VMCTR1 , 2+DELAY,	// 11: Power control, 2 args + delay:
  0x3C,						//     VCOMH = 4V
  0x38,						//     VCOML = -1.1V
  10,						//     10 ms delay
  ST7735_PWCTR6 , 2      ,	// 12: Power control, 2 args, no delay:
  0x11, 0x15,
  ST7735_GMCTRP1,16      ,	// 13: Magical unicorn dust, 16 args, no delay:
  0x09, 0x16, 0x09, 0x20,	//     (seriously though, not sure what
  0x21, 0x1B, 0x13, 0x19,	//      these config values represent)
  0x17, 0x15, 0x1E, 0x2B,
  0x04, 0x05, 0x02, 0x0E,
  ST7735_GMCTRN1,16+DELAY,	// 14: Sparkles and rainbows, 16 args + delay:
  0x0B, 0x14, 0x08, 0x1E,	//     (ditto)
  0x22, 0x1D, 0x18, 0x1E,
  0x1B, 0x1A, 0x24, 0x2B,
  0x06, 0x06, 0x02, 0x0F,
  10,						//     10 ms delay
  TFT_CASET  , 4      , 	// 15: Column addr set, 4 args, no delay:
  0x00, 0x02,				//     XSTART = 2
  0x00, 0x81,				//     XEND = 129
  TFT_PASET  , 4      , 	// 16: Row addr set, 4 args, no delay:
  0x00, 0x02,				//     XSTART = 1
  0x00, 0x81,				//     XEND = 160
  ST7735_NORON  ,   DELAY,	// 17: Normal display on, no args, w/delay
  10,						//     10 ms delay
  TFT_DISPON ,   DELAY,  	// 18: Main screen turn on, no args, w/delay
  255						//     255 = 500 ms delay
};

// Init for 7735R, part 1 (red or green tab)
// -----------------------------------------
static const uint8_t  Rcmd1[] = {
  15,						// 15 commands in list:
  ST7735_SWRESET,   DELAY,	//  1: Software reset, 0 args, w/delay
  150,						//     150 ms delay
  ST7735_SLPOUT ,   DELAY,	//  2: Out of sleep mode, 0 args, w/delay
  255,						//     500 ms delay
  ST7735_FRMCTR1, 3      ,	//  3: Frame rate ctrl - normal mode, 3 args:
  0x01, 0x2C, 0x2D,			//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST7735_FRMCTR2, 3      ,	//  4: Frame rate control - idle mode, 3 args:
  0x01, 0x2C, 0x2D,			//     Rate = fosc/(1x2+40) * (LINE+2C+2D)
  ST7735_FRMCTR3, 6      ,	//  5: Frame rate ctrl - partial mode, 6 args:
  0x01, 0x2C, 0x2D,			//     Dot inversion mode
  0x01, 0x2C, 0x2D,			//     Line inversion mode
  ST7735_INVCTR , 1      ,	//  6: Display inversion ctrl, 1 arg, no delay:
  0x07,						//     No inversion
  ST7735_PWCTR1 , 3      ,	//  7: Power control, 3 args, no delay:
  0xA2,
  0x02,						//     -4.6V
  0x84,						//     AUTO mode
  ST7735_PWCTR2 , 1      ,	//  8: Power control, 1 arg, no delay:
  0xC5,						//     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
  ST7735_PWCTR3 , 2      ,	//  9: Power control, 2 args, no delay:
  0x0A,						//     Opamp current small
  0x00,						//     Boost frequency
  ST7735_PWCTR4 , 2      ,	// 10: Power control, 2 args, no delay:
  0x8A,						//     BCLK/2, Opamp current small & Medium low
  0x2A,
  ST7735_PWCTR5 , 2      ,	// 11: Power control, 2 args, no delay:
  0x8A, 0xEE,
  ST7735_VMCTR1 , 1      ,	// 12: Power control, 1 arg, no delay:
  0x0E,
  TFT_INVOFF , 0      ,		// 13: Don't invert display, no args, no delay
  TFT_MADCTL , 1      ,		// 14: Memory access control (directions), 1 arg:
  0xC0,						//     row addr/col addr, bottom to top refresh, RGB order
  ST7735_COLMOD , 1+DELAY,	//  15: Set color mode, 1 arg + delay:
  0x05,						//     16-bit color 5-6-5 color format
  10						//     10 ms delay
};

// Init for 7735R, part 2 (green tab only)
// ---------------------------------------
static const uint8_t Rcmd2green[] = {
  2,						//  2 commands in list:
  TFT_CASET  , 4      ,		//  1: Column addr set, 4 args, no delay:
  0x00, 0x02,				//     XSTART = 0
  0x00, 0x7F+0x02,			//     XEND = 129
  TFT_PASET  , 4      ,	    //  2: Row addr set, 4 args, no delay:
  0x00, 0x01,				//     XSTART = 0
  0x00, 0x9F+0x01			//     XEND = 160
};

// Init for 7735R, part 2 (red tab only)
// -------------------------------------
static const uint8_t Rcmd2red[] = {
  2,						//  2 commands in list:
  TFT_CASET  , 4      ,	    //  1: Column addr set, 4 args, no delay:
  0x00, 0x00,				//     XSTART = 0
  0x00, 0x7F,				//     XEND = 127
  TFT_PASET  , 4      ,	    //  2: Row addr set, 4 args, no delay:
  0x00, 0x00,				//     XSTART = 0
  0x00, 0x9F				//     XEND = 159
};

// Init for 7735R, part 3 (red or green tab)
// -----------------------------------------
static const uint8_t Rcmd3[] = {
  4,						//  4 commands in list:
  ST7735_GMCTRP1, 16      ,	//  1: Magical unicorn dust, 16 args, no delay:
  0x02, 0x1c, 0x07, 0x12,
  0x37, 0x32, 0x29, 0x2d,
  0x29, 0x25, 0x2B, 0x39,
  0x00, 0x01, 0x03, 0x10,
  ST7735_GMCTRN1, 16      ,	//  2: Sparkles and rainbows, 16 args, no delay:
  0x03, 0x1d, 0x07, 0x06,
  0x2E, 0x2C, 0x29, 0x2D,
  0x2E, 0x2E, 0x37, 0x3F,
  0x00, 0x00, 0x02, 0x10,
  ST7735_NORON  ,    DELAY,	//  3: Normal display on, no args, w/delay
  10,						//     10 ms delay
  TFT_DISPON ,    DELAY,	//  4: Main screen turn on, no args w/delay
  100						//     100 ms delay
};

// Init for ILI7341
// ----------------
static const uint8_t ILI9341_init[] = {
  23,                   					        // 23 commands in list
  ILI9341_SWRESET, DELAY,   						//  1: Software reset, no args, w/delay
  200,												//     50 ms delay
  ILI9341_POWERA, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  ILI9341_POWERB, 3, 0x00, 0XC1, 0X30,
  0xEF, 3, 0x03, 0x80, 0x02,
  ILI9341_DTCA, 3, 0x85, 0x00, 0x78,
  ILI9341_DTCB, 2, 0x00, 0x00,
  ILI9341_POWER_SEQ, 4, 0x64, 0x03, 0X12, 0X81,
  ILI9341_PRC, 1, 0x20,
  ILI9341_PWCTR1, 1,  								//Power control
  0x23,               								//VRH[5:0]
  ILI9341_PWCTR2, 1,   								//Power control
  0x10,                 							//SAP[2:0];BT[3:0]
  ILI9341_VMCTR1, 2,    							//VCM control
  0x3e,                 							//Contrast
  0x28,
  ILI9341_VMCTR2, 1,  								//VCM control2
  0x86,
  TFT_MADCTL, 1,    								// Memory Access Control
  0x48,
  ILI9341_PIXFMT, 1,
  0x55,
  ILI9341_FRMCTR1, 2,
  0x00,
  0x18,
  ILI9341_DFUNCTR, 3,   							// Display Function Control
  0x08,
  0x82,
  0x27,
  TFT_PTLAR, 4, 0x00, 0x00, 0x01, 0x3F,
  ILI9341_3GAMMA_EN, 1,								// 3Gamma Function Disable
  0x00, // 0x02
  ILI9341_GAMMASET, 1, 								//Gamma curve selected
  0x01,
  ILI9341_GMCTRP1, 15,   							//Positive Gamma Correction
  0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
  0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
  ILI9341_GMCTRN1, 15,   							//Negative Gamma Correction
  0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
  0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  ILI9341_SLPOUT, DELAY, 							//  Sleep out
  120,			 									//  120 ms delay
  TFT_DISPON, 0,
};

//------------------------------------------------------
// Companion code to the above tables.  Reads and issues
// a series of LCD commands stored in byte array
//--------------------------------------------
static void commandList(const uint8_t *addr) {
  uint8_t  numCommands, numArgs, cmd;
  uint16_t ms;

  numCommands = *addr++;         // Number of commands to follow
  while(numCommands--) {         // For each command...
    cmd = *addr++;               // save command
    numArgs  = *addr++;          //   Number of args to follow
    ms       = numArgs & DELAY;  //   If high bit set, delay follows args
    numArgs &= ~DELAY;           //   Mask out delay bit

    tft_cmd(cmd);
    tft_data(addr, numArgs);

    addr += numArgs;

    if(ms) {
      ms = *addr++;              // Read post-command delay time (ms)
      if(ms == 255) ms = 500;    // If 255, delay for 500 ms
	  vTaskDelay(ms / portTICK_RATE_MS);
    }
  }
}

// Initialization code common to both 'B' and 'R' type displays
//-----------------------------------------------------
static void ST7735_commonInit(const uint8_t *cmdList) {
	// toggle RST low to reset; CS low so it'll listen to us
#if CONFIG_LUA_RTOS_TFT_RESET == 0
  tft_cmd(ST7735_SWRESET);
  vTaskDelay(130 / portTICK_RATE_MS);
#else
  TFT_RST1;
  vTaskDelay(10 / portTICK_RATE_MS);
  TFT_RST0;
  vTaskDelay(50 / portTICK_RATE_MS);
  TFT_RST1;
  vTaskDelay(130 / portTICK_RATE_MS);
#endif
  if(cmdList) commandList(cmdList);
}

// Initialization for ST7735B screens
//------------------------------
static void ST7735_initB(void) {
  ST7735_commonInit(Bcmd);
}

// Initialization for ST7735R screens (green or red tabs)
//-----------------------------------------
static void ST7735_initR(uint8_t options) {
  vTaskDelay(50 / portTICK_RATE_MS);
  ST7735_commonInit(Rcmd1);
  if(options == INITR_GREENTAB) {
    commandList(Rcmd2green);
    colstart = 2;
    rowstart = 1;
  } else {
    // colstart, rowstart left at default '0' values
    commandList(Rcmd2red);
  }
  commandList(Rcmd3);

  // if black, change MADCTL color filter
  if (options == INITR_BLACKTAB) {
    tft_cmd(TFT_MADCTL);
    uint8_t dt = 0xC0;
    tft_data(&dt, 1);
  }

  //  tabcolor = options;
}

// Init tft SPI interface
//-----------------------------------
int tft_spi_init(lua_State* L, uint8_t typ, uint8_t cs, uint8_t tcs) {
	driver_error_t *error = spi_setup(CONFIG_LUA_RTOS_TFT_SPI, 1, cs, 0, CONFIG_LUA_RTOS_TFT_HZ, SPI_FLAG_WRITE, &disp_spi);
	if (error) {
		return luaL_driver_error(L, error);
	}

	if (typ == 3) {
		driver_error_t *error = spi_setup(CONFIG_LUA_RTOS_TFT_TP_SPI, 1, tcs, 0, CONFIG_LUA_RTOS_TFT_TP_HZ, SPI_FLAG_READ, &disp_spi);
		if (error) {
			return luaL_driver_error(L, error);
		}
	}

    #if CONFIG_LUA_RTOS_TFT_RESET
    gpio_pin_output(CONFIG_LUA_RTOS_TFT_RESET);
	#endif

    gpio_pin_output(CONFIG_LUA_RTOS_TFT_CMD);
    gpio_ll_pin_clr(CONFIG_LUA_RTOS_TFT_CMD);

    // Initialize display
    if (typ == 0) {
    	ST7735_initB();
    }
    else if (typ == 1) {
    	ST7735_initR(INITR_BLACKTAB);
    }
    else if (typ == 2) {
    	ST7735_initR(INITR_GREENTAB);
    }
    else if (typ == 3) {
		#if CONFIG_LUA_RTOS_TFT_RESET
		//Reset the display
		TFT_RST0;
		vTaskDelay(100 / portTICK_RATE_MS);
		TFT_RST0;
		vTaskDelay(100 / portTICK_RATE_MS);
		#endif
        commandList(ILI9341_init);
    }

	spi_ll_deselect(disp_spi);

	if (!tft_line) tft_line = malloc(TFT_LINEBUF_MAX_SIZE*2);

	syslog(LOG_INFO, "tft is at spi%d, pin cs=%s%d", CONFIG_LUA_RTOS_TFT_TP_SPI,
        gpio_portname(cs), gpio_name(cs)
	);

	if (typ == 3) {
		syslog(LOG_INFO, "tft touch pannel is at spi%d, pin cs=%s%d", CONFIG_LUA_RTOS_TFT_TP_SPI,
	        gpio_portname(tcs), gpio_name(tcs)
		);
	}

	return disp_spi;
}

#endif
