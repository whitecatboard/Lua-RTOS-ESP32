/* Lua-RTOS-ESP32 TFT module
 * SPI access functions
 * Author: LoBo (loboris@gmail.com, loboris.github)
 *
 * Module supporting SPI TFT displays based on ILI9341 & ST7735 controllers
*/


#ifndef _TFTSPI_H_
#define _TFTSPI_H_

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_TFT

#include "lua.h"

#include <string.h>
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "drivers/spi.h"
#include "sys/syslog.h"

//#define TFT_USE_BKLT	1

#if CONFIG_LUA_RTOS_TFT_RESET
  #define TFT_RST1  gpio_ll_pin_set(CONFIG_LUA_RTOS_TFT_RESET)
  #define TFT_RST0  gpio_ll_pin_clr(CONFIG_LUA_RTOS_TFT_RESET)
#endif

#define TFT_MAX_DISP_SIZE		480					// maximum display dimension in pixel
#define TFT_LINEBUF_MAX_SIZE	TFT_MAX_DISP_SIZE	// line buffer maximum size in words (uint16_t)

//#define tft_color(color) ( (uint16_t)((color >> 8) | (color << 8)) )
#define swap(a, b) { int16_t t = a; a = b; b = t; }

// The ILI9341 needs a bunch of command/argument values to be initialized. They are stored in this struct.
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} ili_init_cmd_t;


// Display constants
#define ST7735_WIDTH  128
#define ST7735_HEIGHT 160
#define ILI9341_WIDTH  240
#define ILI9341_HEIGHT 320
#define XADOW_WIDTH  240
#define XADOW_HEIGHT 240

#define INITR_GREENTAB 0x0
#define INITR_REDTAB   0x1
#define INITR_BLACKTAB 0x2

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define TFT_INVOFF     0x20
#define TFT_INVONN     0x21
#define TFT_DISPOFF    0x28
#define TFT_DISPON     0x29
#define TFT_CASET      0x2A
#define TFT_PASET      0x2B
#define TFT_RAMWR      0x2C
#define TFT_RAMRD      0x2E
#define TFT_MADCTL	   0x36
#define TFT_PTLAR 	   0x30
#define TFT_ENTRYM 	   0xB7

#define ST7735_COLMOD  0x3A

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0D
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_GAMMASET 0x26

#define ILI9341_PIXFMT  0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_POWERA	0xCB
#define ILI9341_POWERB	0xCF
#define ILI9341_POWER_SEQ       0xED
#define ILI9341_DTCA	0xE8
#define ILI9341_DTCB	0xEA
#define ILI9341_PRC	0xF7
#define ILI9341_3GAMMA_EN	0xF2

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04


// Color definitions
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFD20      /* 255, 165,   0 */
#define TFT_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define TFT_PINK        0xF81F

#define INVERT_ON		1
#define INVERT_OFF		0

#define PORTRAIT	0
#define LANDSCAPE	1
#define PORTRAIT_FLIP	2
#define LANDSCAPE_FLIP	3

#define LASTX	-1
#define LASTY	-2
#define CENTER	-3
#define RIGHT	-4
#define BOTTOM	-4

#define DEFAULT_FONT	0
#define DEJAVU18_FONT	1
#define DEJAVU24_FONT	2
#define UBUNTU16_FONT	3
#define COMIC24_FONT	4
#define MINYA24_FONT	5
#define TOONEY32_FONT	6
#define FONT_7SEG		7
#define USER_FONT		8

#define bitmapdatatype uint16_t *


int TFT_type;
uint16_t _width;
uint16_t _height;
uint16_t *tft_line;

void tft_set_defaults();
void tft_spi_disp_config(unsigned char sdi, unsigned char sdo, unsigned char sck, unsigned char cs, unsigned char dc);
void tft_spi_touch_config(unsigned char sdi, unsigned char sdo, unsigned char sck, unsigned char tcs);
int tft_spi_init(lua_State* L, uint8_t typ, uint8_t cs, uint8_t tcs);

void tft_cmd(const uint8_t cmd);
void tft_data(const uint8_t *data, int len);
void send_data(int x1, int y1, int x2, int y2, uint32_t len, uint16_t *buf);
void drawPixel(int16_t x, int16_t y, uint16_t color, uint8_t sel);
void TFT_pushColorRep(int x1, int y1, int x2, int y2, uint16_t data, uint32_t len);
int read_data(int x1, int y1, int x2, int y2, int len, uint8_t *buf);
uint16_t readPixel(int16_t x, int16_t y);
//void fill_tftline(uint16_t color, uint16_t len);

uint16_t touch_get_data(uint8_t type);

#endif  //LUA_USE_TFT

#endif
