/* Lua-RTOS-ESP32 TFT module
 *
 *  Author: LoBo (loboris@gmail.com, loboris.github)
 *
 *  Module supporting SPI TFT displays based on ILI9341 & ST7735 controllers
*/

#include "luartos.h"

#if CONFIG_LUA_RTOS_LUA_USE_TFT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "tft/tftspi.h"
#include "time.h"
#include "tjpgd.h"
#include <math.h>
#include "sys/status.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "error.h"

#include <drivers/spi.h>

static int disp_spi;

// Constants for ellipse function
#define TFT_ELLIPSE_UPPER_RIGHT 0x01
#define TFT_ELLIPSE_UPPER_LEFT  0x02
#define TFT_ELLIPSE_LOWER_LEFT 0x04
#define TFT_ELLIPSE_LOWER_RIGHT  0x08

// Constants for Arc function
// number representing the maximum angle (e.g. if 100, then if you pass in start=0 and end=50, you get a half circle)
// this can be changed with setArcParams function at runtime
#define DEFAULT_ARC_ANGLE_MAX 360
// rotational offset in degrees defining position of value 0 (-90 will put it at the top of circle)
// this can be changed with setAngleOffset function at runtime
#define DEFAULT_ANGLE_OFFSET -90

#define DEG_TO_RAD 0.01745329252
#define RAD_TO_DEG 57.295779513
#define deg_to_rad 0.01745329252 + 3.14159265359

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif
#if !defined(min)
#define min(A,B) ( (A) < (B) ? (A):(B))
#endif


typedef struct {
	uint8_t 	*font;
	uint8_t 	x_size;
	uint8_t 	y_size;
	uint8_t	    offset;
	uint16_t	numchars;
    uint8_t     bitmap;
	uint16_t    color;
} Font;

typedef struct {
      uint8_t charCode;
      int adjYOffset;
      int width;
      int height;
      int xOffset;
      int xDelta;
      uint16_t dataPtr;
} propFont;

typedef struct {
	uint16_t        x1;
	uint16_t        y1;
	uint16_t        x2;
	uint16_t        y2;
} dispWin_t;

static dispWin_t dispWin = {
  .x1 = 0,
  .y1 = 0,
  .x2 = 320,
  .y2 = 240,
};


extern uint8_t tft_DefaultFont[];
extern uint8_t tft_Dejavu18[];
extern uint8_t tft_Dejavu24[];
extern uint8_t tft_Ubuntu16[];
extern uint8_t tft_Comic24[];
extern uint8_t tft_minya24[];
extern uint8_t tft_tooney32[];

//static uint8_t tp_initialized = 0;	// touch panel initialized flag

static uint8_t *userfont = NULL;
static uint8_t orientation = PORTRAIT;	// screen orientation
static uint8_t rotation = 0;			// font rotation

static uint8_t	_transparent = 0;
static uint16_t	_fg = TFT_GREEN;
static uint16_t _bg = TFT_BLACK;
static uint8_t	_wrap = 0;				// character wrapping to new line
static int		TFT_X  = 0;
static int		TFT_Y  = 0;
static int		TFT_OFFSET  = 0;

static Font		cfont;
static propFont	fontChar;
static uint8_t	_forceFixed = 0;

uint32_t tp_calx = 7472920;
uint32_t tp_caly = 122224794;

static float _arcAngleMax = DEFAULT_ARC_ANGLE_MAX;
static float _angleOffset = DEFAULT_ANGLE_OFFSET;

// ================ Basics drawing functions ===================================
// Only functions which actually sends data to display
// All drawings are clipped to 'dispWin'

// draw color pixel on screen
//----------------------------------------------------------------------------
static void TFT_drawPixel(int16_t x, int16_t y, uint16_t color, uint8_t sel) {

  if ((x < dispWin.x1) || (y < dispWin.y1) || (x > dispWin.x2) || (y > dispWin.y2)) return;

  drawPixel(x, y, color, sel);
}

//---------------------------------------------------
static uint16_t TFT_readPixel(int16_t x, int16_t y) {

  if ((x < dispWin.x1) || (y < dispWin.y1) || (x > dispWin.x2) || (y > dispWin.y2)) return 0;

  return readPixel(x, y);
}

//------------------------------------------------------------------------------
static void TFT_drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
	// clipping
	if ((x < dispWin.x1) || (x > dispWin.x2) || (y > dispWin.y2)) return;
	if (y < dispWin.y1) {
		h -= (dispWin.y1 - y);
		y = dispWin.y1;
	}
	if (h < 0) h = 0;
	if ((y + h) > (dispWin.y2+1)) h = dispWin.y2 - y + 1;
	if (h == 0) h = 1;
	TFT_pushColorRep(x, y, x, y+h-1, color, (uint32_t)h);
}

//------------------------------------------------------------------------------
static void TFT_drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
	// clipping
	if ((y < dispWin.y1) || (x > dispWin.x2) || (y > dispWin.y2)) return;
	if (x < dispWin.x1) {
		w -= (dispWin.x1 - x);
		x = dispWin.x1;
	}
	if (w < 0) w = 0;
	if ((x + w) > (dispWin.x2+1)) w = dispWin.x2 - x + 1;
	if (w == 0) w = 1;

	TFT_pushColorRep(x, y, x+w-1, y, color, (uint32_t)w);
}

// fill a rectangle
//------------------------------------------------------------------------------------
static void TFT_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	// clipping
	if ((x >= dispWin.x2) || (y > dispWin.y2)) return;

	if (x < dispWin.x1) {
		w -= (dispWin.x1 - x);
		x = dispWin.x1;
	}
	if (y < dispWin.y1) {
		h -= (dispWin.y1 - y);
		y = dispWin.y1;
	}
	if (w < 0) w = 0;
	if (h < 0) h = 0;

	if ((x + w) > (dispWin.x2+1)) w = dispWin.x2 - x + 1;
	if ((y + h) > (dispWin.y2+1)) h = dispWin.y2 - y + 1;
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	TFT_pushColorRep(x, y, x+w-1, y+h-1, color, (uint32_t)(h*w));
}

//------------------------------------------
static void TFT_fillScreen(uint16_t color) {
	TFT_pushColorRep(0, 0, _width-1, _height-1, color, (uint32_t)(_height*_width));
}

// ^^^============= Basics drawing functions ================================^^^


// ================ Graphics drawing functions ==================================

//---------------------------------------------------------------------------------------
static void TFT_drawRect(uint16_t x1,uint16_t y1,uint16_t w,uint16_t h, uint16_t color) {
  TFT_drawFastHLine(x1,y1,w, color);
  TFT_drawFastVLine(x1+w-1,y1,h, color);
  TFT_drawFastHLine(x1,y1+h-1,w, color);
  TFT_drawFastVLine(x1,y1,h, color);
}

//-------------------------------------------------------------------------------------------------
static void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	spi_ll_select(disp_spi);
	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			TFT_drawPixel(x0 + x, y0 + y, color, 0);
			TFT_drawPixel(x0 + y, y0 + x, color, 0);
		}
		if (cornername & 0x2) {
			TFT_drawPixel(x0 + x, y0 - y, color, 0);
			TFT_drawPixel(x0 + y, y0 - x, color, 0);
		}
		if (cornername & 0x8) {
			TFT_drawPixel(x0 - y, y0 + x, color, 0);
			TFT_drawPixel(x0 - x, y0 + y, color, 0);
		}
		if (cornername & 0x1) {
			TFT_drawPixel(x0 - y, y0 - x, color, 0);
			TFT_drawPixel(x0 - x, y0 - y, color, 0);
		}
	}
	spi_ll_deselect(disp_spi);
}

// Used to do circles and roundrects
//----------------------------------------------------------------------------------------------------------------
static void fillCircleHelper(int16_t x0, int16_t y0, int16_t r,	uint8_t cornername, int16_t delta, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;
	int16_t ylm = x0 - r;

	while (x < y) {
		if (f >= 0) {
			if (cornername & 0x1) TFT_drawFastVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
			if (cornername & 0x2) TFT_drawFastVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
			ylm = x0 - y;
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if ((x0 - x) > ylm) {
			if (cornername & 0x1) TFT_drawFastVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
			if (cornername & 0x2) TFT_drawFastVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
		}
	}
}

// Draw a rounded rectangle
//-----------------------------------------------------------------------------------------------------
static void TFT_drawRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
	// smarter version
	TFT_drawFastHLine(x + r, y, w - 2 * r, color);			// Top
	TFT_drawFastHLine(x + r, y + h - 1, w - 2 * r, color);	// Bottom
	TFT_drawFastVLine(x, y + r, h - 2 * r, color);			// Left
	TFT_drawFastVLine(x + w - 1, y + r, h - 2 * r, color);	// Right

	// draw four corners
	drawCircleHelper(x + r, y + r, r, 1, color);
	drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

// Fill a rounded rectangle
//-----------------------------------------------------------------------------------------------------
static void TFT_fillRoundRect(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
	// smarter version
	TFT_fillRect(x + r, y, w - 2 * r, h, color);

	// draw four corners
	fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
	fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}



// Bresenham's algorithm - thx wikipedia - speed enhanced by Bodmer this uses
// the eficient FastH/V Line draw routine for segments of 2 pixels or more
//--------------------------------------------------------------------------------------
static void TFT_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  if (x0 == x1) {
	  if (y0 <= y1) TFT_drawFastVLine(x0, y0, y1-y0, color);
	  else TFT_drawFastVLine(x0, y1, y0-y1, color);
	  return;
  }
  if (y0 == y1) {
	  if (x0 <= x1) TFT_drawFastHLine(x0, y0, x1-x0, color);
	  else TFT_drawFastHLine(x1, y0, x0-x1, color);
	  return;
  }

  int steep = 0;
  if (abs(y1 - y0) > abs(x1 - x0)) steep = 1;
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx = x1 - x0, dy = abs(y1 - y0);;
  int16_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

  if (y0 < y1) ystep = 1;

  // Split into steep and not steep for FastH/V separation
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) TFT_drawPixel(y0, xs, color, 1);
        else TFT_drawFastVLine(y0, xs, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) TFT_drawFastVLine(y0, xs, dlen, color);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        err += dx;
        if (dlen == 1) TFT_drawPixel(xs, y0, color, 1);
        else TFT_drawFastHLine(xs, y0, dlen, color);
        dlen = 0; y0 += ystep; xs = x0 + 1;
      }
    }
    if (dlen) TFT_drawFastHLine(xs, y0, dlen, color);
  }
}

static void drawLineByAngle(int16_t x, int16_t y, int16_t angle, uint16_t length, uint16_t color)
{
	TFT_drawLine(
		x,
		y,
		x + length * cos((angle + _angleOffset) * DEG_TO_RAD),
		y + length * sin((angle + _angleOffset) * DEG_TO_RAD), color);
}


static void DrawLineByAngle(int16_t x, int16_t y, int16_t angle, uint16_t start, uint16_t length, uint16_t color)
{
	TFT_drawLine(
		x + start * cos((angle + _angleOffset) * DEG_TO_RAD),
		y + start * sin((angle + _angleOffset) * DEG_TO_RAD),
		x + (start + length) * cos((angle + _angleOffset) * DEG_TO_RAD),
		y + (start + length) * sin((angle + _angleOffset) * DEG_TO_RAD), color);
}


// Draw a triangle
//------------------------------------------------------------------------------
static void TFT_drawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
			     uint16_t x2, uint16_t y2, uint16_t color)
{
  TFT_drawLine(x0, y0, x1, y1, color);
  TFT_drawLine(x1, y1, x2, y2, color);
  TFT_drawLine(x2, y2, x0, y0, color);
}

// Fill a triangle
//-----------------------------------------------------------------------
static void TFT_fillTriangle(uint16_t x0, uint16_t y0,
				uint16_t x1, uint16_t y1,
				uint16_t x2, uint16_t y2, uint16_t color)
{
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    TFT_drawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
  int32_t
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    TFT_drawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    TFT_drawFastHLine(a, y, b-a+1, color);
  }
}

//----------------------------------------------------------------------------
static void TFT_drawCircle(int16_t x, int16_t y, int radius, uint16_t color) {
  int f = 1 - radius;
  int ddF_x = 1;
  int ddF_y = -2 * radius;
  int x1 = 0;
  int y1 = radius;

  spi_ll_select(disp_spi);
  TFT_drawPixel(x, y + radius, color, 0);
  TFT_drawPixel(x, y - radius, color, 0);
  TFT_drawPixel(x + radius, y, color, 0);
  TFT_drawPixel(x - radius, y, color, 0);
  while(x1 < y1) {
    if (f >= 0) {
      y1--;
      ddF_y += 2;
      f += ddF_y;
    }
    x1++;
    ddF_x += 2;
    f += ddF_x;
    TFT_drawPixel(x + x1, y + y1, color, 0);
    TFT_drawPixel(x - x1, y + y1, color, 0);
    TFT_drawPixel(x + x1, y - y1, color, 0);
    TFT_drawPixel(x - x1, y - y1, color, 0);
    TFT_drawPixel(x + y1, y + x1, color, 0);
    TFT_drawPixel(x - y1, y + x1, color, 0);
    TFT_drawPixel(x + y1, y - x1, color, 0);
    TFT_drawPixel(x - y1, y - x1, color, 0);
  }
  spi_ll_deselect(disp_spi);
}

//----------------------------------------------------------------------------
static void TFT_fillCircle(int16_t x, int16_t y, int radius, uint16_t color) {
	TFT_drawFastVLine(x, y-radius, 2*radius+1, color);
	fillCircleHelper(x, y, radius, 3, 0, color);
}

//--------------------------------------------------------------------------------------------------------------------
static void TFT_draw_ellipse_section(uint16_t x, uint16_t y, uint16_t x0, uint16_t y0, uint16_t color, uint8_t option)
{
	spi_ll_select(disp_spi);
    // upper right
    if ( option & TFT_ELLIPSE_UPPER_RIGHT ) TFT_drawPixel(x0 + x, y0 - y, color, 0);
    // upper left
    if ( option & TFT_ELLIPSE_UPPER_LEFT ) TFT_drawPixel(x0 - x, y0 - y, color, 0);
    // lower right
    if ( option & TFT_ELLIPSE_LOWER_RIGHT ) TFT_drawPixel(x0 + x, y0 + y, color, 0);
    // lower left
    if ( option & TFT_ELLIPSE_LOWER_LEFT ) TFT_drawPixel(x0 - x, y0 + y, color, 0);
	spi_ll_deselect(disp_spi);
}

//--------------------------------------------------------------------------------------------------------------
static void TFT_draw_ellipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color, uint8_t option)
{
  uint16_t x, y;
  int32_t xchg, ychg;
  int32_t err;
  int32_t rxrx2;
  int32_t ryry2;
  int32_t stopx, stopy;

  rxrx2 = rx;
  rxrx2 *= rx;
  rxrx2 *= 2;

  ryry2 = ry;
  ryry2 *= ry;
  ryry2 *= 2;

  x = rx;
  y = 0;

  xchg = 1;
  xchg -= rx;
  xchg -= rx;
  xchg *= ry;
  xchg *= ry;

  ychg = rx;
  ychg *= rx;

  err = 0;

  stopx = ryry2;
  stopx *= rx;
  stopy = 0;

  while( stopx >= stopy )
  {
	TFT_draw_ellipse_section(x, y, x0, y0, color, option);
    y++;
    stopy += rxrx2;
    err += ychg;
    ychg += rxrx2;
    if ( 2*err+xchg > 0 )
    {
      x--;
      stopx -= ryry2;
      err += xchg;
      xchg += ryry2;
    }
  }

  x = 0;
  y = ry;

  xchg = ry;
  xchg *= ry;

  ychg = 1;
  ychg -= ry;
  ychg -= ry;
  ychg *= rx;
  ychg *= rx;

  err = 0;

  stopx = 0;

  stopy = rxrx2;
  stopy *= ry;


  while( stopx <= stopy )
  {
	TFT_draw_ellipse_section(x, y, x0, y0, color, option);
    x++;
    stopx += ryry2;
    err += xchg;
    xchg += ryry2;
    if ( 2*err+ychg > 0 )
    {
      y--;
      stopy -= rxrx2;
      err += ychg;
      ychg += rxrx2;
    }
  }

}

//---------------------------------------------------------------------------------------------------------------------------
static void TFT_draw_filled_ellipse_section(uint16_t x, uint16_t y, uint16_t x0, uint16_t y0, uint16_t color, uint8_t option)
{
    // upper right
    if ( option & TFT_ELLIPSE_UPPER_RIGHT ) TFT_drawFastVLine(x0+x, y0-y, y+1, color);
    // upper left
    if ( option & TFT_ELLIPSE_UPPER_LEFT ) TFT_drawFastVLine(x0-x, y0-y, y+1, color);
    // lower right
    if ( option & TFT_ELLIPSE_LOWER_RIGHT ) TFT_drawFastVLine(x0+x, y0, y+1, color);
    // lower left
    if ( option & TFT_ELLIPSE_LOWER_LEFT ) TFT_drawFastVLine(x0-x, y0, y+1, color);
}

//---------------------------------------------------------------------------------------------------------------------
static void TFT_draw_filled_ellipse(uint16_t x0, uint16_t y0, uint16_t rx, uint16_t ry, uint16_t color, uint8_t option)
{
  uint16_t x, y;
  int32_t xchg, ychg;
  int32_t err;
  int32_t rxrx2;
  int32_t ryry2;
  int32_t stopx, stopy;

  rxrx2 = rx;
  rxrx2 *= rx;
  rxrx2 *= 2;

  ryry2 = ry;
  ryry2 *= ry;
  ryry2 *= 2;

  x = rx;
  y = 0;

  xchg = 1;
  xchg -= rx;
  xchg -= rx;
  xchg *= ry;
  xchg *= ry;

  ychg = rx;
  ychg *= rx;

  err = 0;

  stopx = ryry2;
  stopx *= rx;
  stopy = 0;

  while( stopx >= stopy ) {
	TFT_draw_filled_ellipse_section(x, y, x0, y0, color, option);
    y++;
    stopy += rxrx2;
    err += ychg;
    ychg += rxrx2;
    if ( 2*err+xchg > 0 )
    {
      x--;
      stopx -= ryry2;
      err += xchg;
      xchg += ryry2;
    }
  }

  x = 0;
  y = ry;

  xchg = ry;
  xchg *= ry;

  ychg = 1;
  ychg -= ry;
  ychg -= ry;
  ychg *= rx;
  ychg *= rx;

  err = 0;

  stopx = 0;

  stopy = rxrx2;
  stopy *= ry;


  while( stopx <= stopy ) {
	TFT_draw_filled_ellipse_section(x, y, x0, y0, color, option);
    x++;
    stopx += ryry2;
    err += xchg;
    xchg += ryry2;
    if ( 2*err+ychg > 0 ) {
      y--;
      stopy -= rxrx2;
      err += ychg;
      ychg += rxrx2;
    }
  }
}

// Adapted from: Marek Buriak (https://github.com/marekburiak/ILI9341_due)
//---------------------------------------------------------------------------------------------------------------------------------------
static void TFT_fillArcOffsetted(uint16_t cx, uint16_t cy, uint16_t radius, uint16_t thickness, float start, float end, uint16_t color) {
	int16_t xmin = 65535, xmax = -32767, ymin = 32767, ymax = -32767;
	float cosStart, sinStart, cosEnd, sinEnd;
	float r, t;
	float startAngle, endAngle;

	startAngle = ((start / _arcAngleMax) * 360) + _angleOffset;
	endAngle = ((end / _arcAngleMax) * 360) + _angleOffset;

	while (startAngle < 0) startAngle += 360;
	while (endAngle < 0) endAngle += 360;
	while (startAngle > 360) startAngle -= 360;
	while (endAngle > 360) endAngle -= 360;

	if (startAngle > endAngle) {
		TFT_fillArcOffsetted(cx, cy, radius, thickness, ((startAngle) / (float)360) * _arcAngleMax, _arcAngleMax, color);
		TFT_fillArcOffsetted(cx, cy, radius, thickness, 0, ((endAngle) / (float)360) * _arcAngleMax, color);
	}
	else {
		// Calculate bounding box for the arc to be drawn
		cosStart = cos(startAngle * DEG_TO_RAD);
		sinStart = sin(startAngle * DEG_TO_RAD);
		cosEnd = cos(endAngle * DEG_TO_RAD);
		sinEnd = sin(endAngle * DEG_TO_RAD);

		r = radius;
		// Point 1: radius & startAngle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 2: radius & endAngle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		r = radius - thickness;
		// Point 3: radius-thickness & startAngle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 4: radius-thickness & endAngle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;


		// Corrections if arc crosses X or Y axis
		if ((startAngle < 90) && (endAngle > 90)) {
			ymax = radius;
		}

		if ((startAngle < 180) && (endAngle > 180)) {
			xmin = -radius;
		}

		if ((startAngle < 270) && (endAngle > 270)) {
			ymin = -radius;
		}

		// Slopes for the two sides of the arc
		float sslope = (float)cosStart / (float)sinStart;
		float eslope = (float)cosEnd / (float)sinEnd;

		if (endAngle == 360) eslope = -1000000;

		int ir2 = (radius - thickness) * (radius - thickness);
		int or2 = radius * radius;

		for (int x = xmin; x <= xmax; x++) {
			bool y1StartFound = false, y2StartFound = false;
			bool y1EndFound = false, y2EndSearching = false;
			int y1s = 0, y1e = 0, y2s = 0;
			for (int y = ymin; y <= ymax; y++)
			{
				int x2 = x * x;
				int y2 = y * y;

				if (
					(x2 + y2 < or2 && x2 + y2 >= ir2) && (
					(y > 0 && startAngle < 180 && x <= y * sslope) ||
					(y < 0 && startAngle > 180 && x >= y * sslope) ||
					(y < 0 && startAngle <= 180) ||
					(y == 0 && startAngle <= 180 && x < 0) ||
					(y == 0 && startAngle == 0 && x > 0)
					) && (
					(y > 0 && endAngle < 180 && x >= y * eslope) ||
					(y < 0 && endAngle > 180 && x <= y * eslope) ||
					(y > 0 && endAngle >= 180) ||
					(y == 0 && endAngle >= 180 && x < 0) ||
					(y == 0 && startAngle == 0 && x > 0)))
				{
					if (!y1StartFound)	//start of the higher line found
					{
						y1StartFound = true;
						y1s = y;
					}
					else if (y1EndFound && !y2StartFound) //start of the lower line found
					{
						y2StartFound = true;
						y2s = y;
						y += y1e - y1s - 1;	// calculate the most probable end of the lower line (in most cases the length of lower line is equal to length of upper line), in the next loop we will validate if the end of line is really there
						if (y > ymax - 1) // the most probable end of line 2 is beyond ymax so line 2 must be shorter, thus continue with pixel by pixel search
						{
							y = y2s;	// reset y and continue with pixel by pixel search
							y2EndSearching = true;
						}
					}
					else if (y2StartFound && !y2EndSearching)
					{
						// we validated that the probable end of the lower line has a pixel, continue with pixel by pixel search, in most cases next loop with confirm the end of lower line as it will not find a valid pixel
						y2EndSearching = true;
					}
				}
				else
				{
					if (y1StartFound && !y1EndFound) //higher line end found
					{
						y1EndFound = true;
						y1e = y - 1;
						TFT_drawFastVLine(cx + x, cy + y1s, y - y1s, color);
						if (y < 0) {
							y = abs(y); // skip the empty middle
						}
						else break;
					}
					else if (y2StartFound)
					{
						if (y2EndSearching) {
							// we found the end of the lower line after pixel by pixel search
							TFT_drawFastVLine(cx + x, cy + y2s, y - y2s, color);
							y2EndSearching = false;
							break;
						}
						else {
							// the expected end of the lower line is not there so the lower line must be shorter
							y = y2s;	// put the y back to the lower line start and go pixel by pixel to find the end
							y2EndSearching = true;
						}
					}
				}
			}
			if (y1StartFound && !y1EndFound) {
				y1e = ymax;
				TFT_drawFastVLine(cx + x, cy + y1s, y1e - y1s + 1, color);
			}
			else if (y2StartFound && y2EndSearching) {
				// we found start of lower line but we are still searching for the end
				// which we haven't found in the loop so the last pixel in a column must be the end
				TFT_drawFastVLine(cx + x, cy + y2s, ymax - y2s + 1, color);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
static void drawPolygon(int cx, int cy, int sides, int diameter, uint16_t color, uint8_t fill, int deg)
{
  sides = (sides > 2? sides : 3);		// This ensures the minimum side number is 3.
  int Xpoints[sides], Ypoints[sides];	// Set the arrays based on the number of sides entered
  int rads = 360 / sides;				// This equally spaces the points.

  for (int idx = 0; idx < sides; idx++) {
    Xpoints[idx] = cx + sin((float)(idx*rads + deg) * deg_to_rad) * diameter;
    Ypoints[idx] = cy + cos((float)(idx*rads + deg) * deg_to_rad) * diameter;
  }

  for(int idx = 0; idx < sides; idx++)	// draws the polygon on the screen.
  {
    if( (idx+1) < sides)
    	TFT_drawLine(Xpoints[idx],Ypoints[idx],Xpoints[idx+1],Ypoints[idx+1], color); // draw the lines
    else
    	TFT_drawLine(Xpoints[idx],Ypoints[idx],Xpoints[0],Ypoints[0], color); // finishes the last line to close up the polygon.
  }
  if(fill)
    for(int idx = 0; idx < sides; idx++)
    {
      if((idx+1) < sides)
    	  TFT_fillTriangle(cx,cy,Xpoints[idx],Ypoints[idx],Xpoints[idx+1],Ypoints[idx+1], color);
      else
    	  TFT_fillTriangle(cx,cy,Xpoints[idx],Ypoints[idx],Xpoints[0],Ypoints[0], color);
    }
}

// Similar to the Polygon function.
//-----------------------------------------------------------------------------------------
static void drawStar(int cx, int cy, int diameter, uint16_t color, bool fill, float factor)
{
  factor = constrain(factor, 1.0, 4.0);
  uint8_t sides = 5;
  uint8_t rads = 360 / sides;

  int Xpoints_O[sides], Ypoints_O[sides], Xpoints_I[sides], Ypoints_I[sides];//Xpoints_T[5], Ypoints_T[5];

  for(int idx = 0; idx < sides; idx++)
  {
	  // makes the outer points
    Xpoints_O[idx] = cx + sin((float)(idx*rads + 72) * deg_to_rad) * diameter;
    Ypoints_O[idx] = cy + cos((float)(idx*rads + 72) * deg_to_rad) * diameter;
    // makes the inner points
    Xpoints_I[idx] = cx + sin((float)(idx*rads + 36) * deg_to_rad) * ((float)(diameter)/factor);
    // 36 is half of 72, and this will allow the inner and outer points to line up like a triangle.
    Ypoints_I[idx] = cy + cos((float)(idx*rads + 36) * deg_to_rad) * ((float)(diameter)/factor);
  }

  for(int idx = 0; idx < sides; idx++)
  {
	if((idx+1) < sides)
	{
	  if(fill) // this part below should be self explanatory. It fills in the star.
	  {
		  TFT_fillTriangle(cx,cy,Xpoints_I[idx],Ypoints_I[idx],Xpoints_O[idx],Ypoints_O[idx], color);
		  TFT_fillTriangle(cx,cy,Xpoints_O[idx],Ypoints_O[idx],Xpoints_I[idx+1],Ypoints_I[idx+1], color);
	  }
	  else
	  {
		  TFT_drawLine(Xpoints_O[idx],Ypoints_O[idx],Xpoints_I[idx+1],Ypoints_I[idx+1], color);
		  TFT_drawLine(Xpoints_I[idx],Ypoints_I[idx],Xpoints_O[idx],Ypoints_O[idx], color);
	  }
	}
    else
    {
	  if(fill)
	  {
		  TFT_fillTriangle(cx,cy,Xpoints_I[0],Ypoints_I[0],Xpoints_O[idx],Ypoints_O[idx], color);
		  TFT_fillTriangle(cx,cy,Xpoints_O[idx],Ypoints_O[idx],Xpoints_I[idx],Ypoints_I[idx], color);
	  }
	  else
	  {
		  TFT_drawLine(Xpoints_O[idx],Ypoints_O[idx],Xpoints_I[idx],Ypoints_I[idx], color);
		  TFT_drawLine(Xpoints_I[0],Ypoints_I[0],Xpoints_O[idx],Ypoints_O[idx], color);
	  }
    }
  }
}


// ================ Font and string functions ==================================

// return max width of the proportional font
//--------------------------------
static uint8_t getMaxWidth(void) {
  uint16_t tempPtr = 4; // point at first char data
  uint8_t cc,cw,ch,w = 0;
  do
  {
    cc = cfont.font[tempPtr++];
    tempPtr++;
    cw = cfont.font[tempPtr++];
    ch = cfont.font[tempPtr++];
    tempPtr += 2;
    if (cc != 0xFF) {
      if (cw != 0) {
        if (cw > w) w = cw;
        // packed bits
        tempPtr += (((cw * ch)-1) / 8) + 1;
      }
    }
  } while (cc != 0xFF);

  return w;
}

//--------------------------------------------------------
static int load_file_font(const char * fontfile, int info)
{
	if (userfont != NULL) {
		free(userfont);
		userfont = NULL;
	}

    struct stat sb;

    // Open the file
    FILE *fhndl = fopen(fontfile, "r");
    if (!fhndl) {
    	syslog(LOG_ERR, "Error opening font file '%s'", fontfile);
		return 0;
    }

	// Get file size
    if (stat(fontfile, &sb) != 0) {
    	syslog(LOG_ERR, "Error getting font file size");
		return 0;
    }
	int fsize = sb.st_size;
	if (fsize < 30) {
		syslog(LOG_ERR, "Error getting font file size");
		return 0;
	}

	userfont = malloc(fsize+4);
	if (userfont == NULL) {
		syslog(LOG_ERR, "Font memory allocation error");
		fclose(fhndl);
		return 0;
	}

	int read = fread(userfont, 1, fsize, fhndl);

	fclose(fhndl);

	if (read != fsize) {
		syslog(LOG_ERR, "Font read error");
		free(userfont);
		userfont = NULL;
		return 0;
	}

	userfont[read] = 0;
	if (strstr((char *)(userfont+read-8), "RPH_font") == NULL) {
		syslog(LOG_ERR, "Font ID not found");
		free(userfont);
		userfont = NULL;
		return 0;
	}

	// Check size
	int size = 0;
	int numchar = 0;
	int width = userfont[0];
	int height = userfont[1];
	uint8_t first = 255;
	uint8_t last = 0;
	//int offst = 0;
	int pminwidth = 255;
	int pmaxwidth = 0;

	if (width != 0) {
		// Fixed font
		numchar = userfont[3];
		first = userfont[2];
		last = first + numchar - 1;
		size = ((width * height * numchar) / 8) + 4;
	}
	else {
		// Proportional font
		size = 4; // point at first char data
		uint8_t charCode;
		int charwidth;

		do {
		    charCode = userfont[size];
		    charwidth = userfont[size+2];

		    if (charCode != 0xFF) {
		    	numchar++;
		    	if (charwidth != 0) size += ((((charwidth * userfont[size+3])-1) / 8) + 7);
		    	else size += 6;

		    	if (info) {
	    			if (charwidth > pmaxwidth) pmaxwidth = charwidth;
	    			if (charwidth < pminwidth) pminwidth = charwidth;
	    			if (charCode < first) first = charCode;
	    			if (charCode > last) last = charCode;
	    		}
		    }
		    else size++;
		  } while ((size < (read-8)) && (charCode != 0xFF));
	}

	if (size != (read-8)) {
		syslog(LOG_ERR, "Font size error: found %d expected %d)", size, (read-8));
		free(userfont);
		userfont = NULL;
		return 0;
	}

	if (info) {
		if (width != 0) {
			syslog(LOG_INFO, "Fixed width font:\r\n  size: %d  width: %d  height: %d  characters: %d (%d~%d)",
					size, width, height, numchar, first, last);
		}
		else {
			syslog(LOG_INFO, "Proportional font:\r\n  size: %d  width: %d~%d  height: %d  characters: %d (%d~%d)\n",
					size, pminwidth, pmaxwidth, height, numchar, first, last);
		}
	}
	return 1;
}

//----------------------------------------------------------
static void TFT_setFont(uint8_t font, const char *font_file)
{
  cfont.font = NULL;

  if (font == FONT_7SEG) {
    cfont.bitmap = 2;
    cfont.x_size = 24;
    cfont.y_size = 6;
    cfont.offset = 0;
    cfont.color  = _fg;
  }
  else {
	  if (font == USER_FONT) {
		  if (load_file_font(font_file, 0) == 0) cfont.font = tft_DefaultFont;
		  else cfont.font = userfont;
	  }
	  else if (font == DEJAVU18_FONT) cfont.font = tft_Dejavu18;
	  else if (font == DEJAVU24_FONT) cfont.font = tft_Dejavu24;
	  else if (font == UBUNTU16_FONT) cfont.font = tft_Ubuntu16;
	  else if (font == COMIC24_FONT) cfont.font = tft_Comic24;
	  else if (font == MINYA24_FONT) cfont.font = tft_minya24;
	  else if (font == TOONEY32_FONT) cfont.font = tft_tooney32;
	  else cfont.font = tft_DefaultFont;

	  cfont.bitmap = 1;
	  cfont.x_size = cfont.font[0];
	  cfont.y_size = cfont.font[1];
	  cfont.offset = cfont.font[2];
	  if (cfont.x_size != 0) cfont.numchars = cfont.font[3];
	  else cfont.numchars = getMaxWidth();
  }
}

// private method to return the Glyph data for an individual character in the proportional font
//--------------------------------
static int getCharPtr(uint8_t c) {
  uint16_t tempPtr = 4; // point at first char data

  do {
    fontChar.charCode = cfont.font[tempPtr++];
    fontChar.adjYOffset = cfont.font[tempPtr++];
    fontChar.width = cfont.font[tempPtr++];
    fontChar.height = cfont.font[tempPtr++];
    fontChar.xOffset = cfont.font[tempPtr++];
    fontChar.xOffset = fontChar.xOffset < 0x80 ? fontChar.xOffset : (0x100 - fontChar.xOffset);
    fontChar.xDelta = cfont.font[tempPtr++];

    if (c != fontChar.charCode && fontChar.charCode != 0xFF) {
      if (fontChar.width != 0) {
        // packed bits
        tempPtr += (((fontChar.width * fontChar.height)-1) / 8) + 1;
      }
    }
  } while (c != fontChar.charCode && fontChar.charCode != 0xFF);

  fontChar.dataPtr = tempPtr;
  if (c == fontChar.charCode) {
    if (_forceFixed > 0) {
      // fix width & offset for forced fixed width
      fontChar.xDelta = cfont.numchars;
      fontChar.xOffset = (fontChar.xDelta - fontChar.width) / 2;
    }
  }

  if (fontChar.charCode != 0xFF) return 1;
  else return 0;
}

// print rotated proportional character
// character is already in fontChar
//--------------------------------------------------------------
static int rotatePropChar(int x, int y, int offset) {
  uint8_t ch = 0;
  double radian = rotation * 0.0175;
  float cos_radian = cos(radian);
  float sin_radian = sin(radian);

  uint8_t mask = 0x80;
  spi_ll_select(disp_spi);
  for (int j=0; j < fontChar.height; j++) {
    for (int i=0; i < fontChar.width; i++) {
      if (((i + (j*fontChar.width)) % 8) == 0) {
        mask = 0x80;
        ch = cfont.font[fontChar.dataPtr++];
      }

      int newX = (int)(x + (((offset + i) * cos_radian) - ((j+fontChar.adjYOffset)*sin_radian)));
      int newY = (int)(y + (((j+fontChar.adjYOffset) * cos_radian) + ((offset + i) * sin_radian)));

      if ((ch & mask) != 0) TFT_drawPixel(newX,newY,_fg, 0);
      else if (!_transparent) TFT_drawPixel(newX,newY,_bg, 0);

      mask >>= 1;
    }
  }
  spi_ll_deselect(disp_spi);

  return fontChar.xDelta+1;
}

// print non-rotated proportional character
// character is already in fontChar
//---------------------------------------------------------
static int printProportionalChar(int x, int y) {
  uint8_t i,j,ch=0;
  uint16_t cx,cy;

  // fill background if not transparent background
  if (!_transparent) {
    TFT_fillRect(x, y, fontChar.xDelta+1, cfont.y_size, _bg);
  }

  // draw Glyph
  uint8_t mask = 0x80;
  spi_ll_select(disp_spi);
  for (j=0; j < fontChar.height; j++) {
    for (i=0; i < fontChar.width; i++) {
      if (((i + (j*fontChar.width)) % 8) == 0) {
        mask = 0x80;
        ch = cfont.font[fontChar.dataPtr++];
      }

      if ((ch & mask) !=0) {
        cx = (uint16_t)(x+fontChar.xOffset+i);
        cy = (uint16_t)(y+j+fontChar.adjYOffset);
        TFT_drawPixel(cx, cy, _fg, 0);
      }
      mask >>= 1;
    }
  }
  spi_ll_deselect(disp_spi);

  return fontChar.xDelta;
}

// non-rotated fixed width character
//----------------------------------------------
static void printChar(uint8_t c, int x, int y) {
  uint8_t i,j,ch,fz,mask;
  uint16_t k,temp,cx,cy;

  // fz = bytes per char row
  fz = cfont.x_size/8;
  if (cfont.x_size % 8) fz++;

  // get char address
  temp = ((c-cfont.offset)*((fz)*cfont.y_size))+4;

  // fill background if not transparent background
  if (!_transparent) {
    TFT_fillRect(x, y, cfont.x_size, cfont.y_size, _bg);
  }

  spi_ll_select(disp_spi);
  for (j=0; j<cfont.y_size; j++) {
    for (k=0; k < fz; k++) {
      ch = cfont.font[temp+k];
      mask=0x80;
      for (i=0; i<8; i++) {
        if ((ch & mask) !=0) {
          cx = (uint16_t)(x+i+(k*8));
          cy = (uint16_t)(y+j);
          TFT_drawPixel(cx, cy, _fg, 0);
        }
        mask >>= 1;
      }
    }
    temp += (fz);
  }
  spi_ll_deselect(disp_spi);
}

// rotated fixed width character
//--------------------------------------------------------
static void rotateChar(uint8_t c, int x, int y, int pos) {
  uint8_t i,j,ch,fz,mask;
  uint16_t temp;
  int newx,newy;
  double radian = rotation*0.0175;
  float cos_radian = cos(radian);
  float sin_radian = sin(radian);
  int zz;

  if( cfont.x_size < 8 ) fz = cfont.x_size;
  else fz = cfont.x_size/8;
  temp=((c-cfont.offset)*((fz)*cfont.y_size))+4;

  spi_ll_select(disp_spi);
  for (j=0; j<cfont.y_size; j++) {
    for (zz=0; zz<(fz); zz++) {
      ch = cfont.font[temp+zz];
      mask = 0x80;
      for (i=0; i<8; i++) {
        newx=(int)(x+(((i+(zz*8)+(pos*cfont.x_size))*cos_radian)-((j)*sin_radian)));
        newy=(int)(y+(((j)*cos_radian)+((i+(zz*8)+(pos*cfont.x_size))*sin_radian)));

        if ((ch & mask) != 0) TFT_drawPixel(newx,newy,_fg, 0);
        else if (!_transparent) TFT_drawPixel(newx,newy,_bg, 0);
        mask >>= 1;
      }
    }
    temp+=(fz);
  }
  spi_ll_deselect(disp_spi);
  // calculate x,y for the next char
  TFT_X = (int)(x + ((pos+1) * cfont.x_size * cos_radian));
  TFT_Y = (int)(y + ((pos+1) * cfont.x_size * sin_radian));
}

// returns the string width in pixels. Useful for positions strings on the screen.
//----------------------------------
static int getStringWidth(char* str) {

  // is it 7-segment font?
  if (cfont.bitmap == 2) return ((2 * (2 * cfont.y_size + 1)) + cfont.x_size) * strlen(str);

  // is it a fixed width font?
  if (cfont.x_size != 0) return strlen(str) * cfont.x_size;
  else {
    // calculate the string width
    char* tempStrptr = str;
    int strWidth = 0;
    while (*tempStrptr != 0) {
      if (getCharPtr(*tempStrptr++)) strWidth += (fontChar.xDelta + 1);
    }
    return strWidth;
  }
}

//==============================================================================
/**
 * bit-encoded bar position of all digits' bcd segments
 *
 *                   6
 * 		  +-----+
 * 		3 |  .	| 2
 * 		  +--5--+
 * 		1 |  .	| 0
 * 		  +--.--+
 * 		     4
 */
static const uint16_t font_bcd[] = {
  0x200, // 0010 0000 0000  // -
  0x080, // 0000 1000 0000  // .
  0x06C, // 0100 0110 1100  // /, degree
  0x05f, // 0000 0101 1111, // 0
  0x005, // 0000 0000 0101, // 1
  0x076, // 0000 0111 0110, // 2
  0x075, // 0000 0111 0101, // 3
  0x02d, // 0000 0010 1101, // 4
  0x079, // 0000 0111 1001, // 5
  0x07b, // 0000 0111 1011, // 6
  0x045, // 0000 0100 0101, // 7
  0x07f, // 0000 0111 1111, // 8
  0x07d, // 0000 0111 1101  // 9
  0x900  // 1001 0000 0000  // :
};

//-------------------------------------------------------------------------------
static void barVert(int16_t x, int16_t y, int16_t w, int16_t l, uint16_t color) {
  TFT_fillTriangle(x+1, y+2*w, x+w, y+w+1, x+2*w-1, y+2*w, color);
  TFT_fillTriangle(x+1, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1, color);
  TFT_fillRect(x, y+2*w+1, 2*w+1, l, color);
  if ((cfont.offset) && (color != _bg)) {
    TFT_drawTriangle(x+1, y+2*w, x+w, y+w+1, x+2*w-1, y+2*w, cfont.color);
    TFT_drawTriangle(x+1, y+2*w+l+1, x+w, y+3*w+l, x+2*w-1, y+2*w+l+1, cfont.color);
    TFT_drawRect(x, y+2*w+1, 2*w+1, l, cfont.color);
  }
}

//------------------------------------------------------------------------------
static void barHor(int16_t x, int16_t y, int16_t w, int16_t l, uint16_t color) {
  TFT_fillTriangle(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, color);
  TFT_fillTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, color);
  TFT_fillRect(x+2*w+1, y, l, 2*w+1, color);
  if ((cfont.offset) && (color != _bg)) {
    TFT_drawTriangle(x+2*w, y+2*w-1, x+w+1, y+w, x+2*w, y+1, cfont.color);
    TFT_drawTriangle(x+2*w+l+1, y+2*w-1, x+3*w+l, y+w, x+2*w+l+1, y+1, cfont.color);
    TFT_drawRect(x+2*w+1, y, l, 2*w+1, cfont.color);
  }
}

//------------------------------------------------------------------------------------------------
static void TFT_draw7seg(int16_t x, int16_t y, int8_t num, int16_t w, int16_t l, uint16_t color) {
  /* TODO: clipping */
  if (num < 0x2D || num > 0x3A) return;

  int16_t c = font_bcd[num-0x2D];
  int16_t d = 2*w+l+1;

  //if (!_transparent) TFT_fillRect(x, y, (2 * (2 * w + 1)) + l, (3 * (2 * w + 1)) + (2 * l), _bg);

  if (!(c & 0x001)) barVert(x+d, y+d, w, l, _bg);
  if (!(c & 0x002)) barVert(x,   y+d, w, l, _bg);
  if (!(c & 0x004)) barVert(x+d, y, w, l, _bg);
  if (!(c & 0x008)) barVert(x,   y, w, l, _bg);
  if (!(c & 0x010)) barHor(x, y+2*d, w, l, _bg);
  if (!(c & 0x020)) barHor(x, y+d, w, l, _bg);
  if (!(c & 0x040)) barHor(x, y, w, l, _bg);

  //if (!(c & 0x080)) TFT_fillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, _bg);
  if (!(c & 0x100)) TFT_fillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, _bg);
  if (!(c & 0x800)) TFT_fillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, _bg);
  //if (!(c & 0x200)) TFT_fillRect(x+2*w+1, y+d, l, 2*w+1, _bg);

  if (c & 0x001) barVert(x+d, y+d, w, l, color);               // down right
  if (c & 0x002) barVert(x,   y+d, w, l, color);               // down left
  if (c & 0x004) barVert(x+d, y, w, l, color);                 // up right
  if (c & 0x008) barVert(x,   y, w, l, color);                 // up left
  if (c & 0x010) barHor(x, y+2*d, w, l, color);                // down
  if (c & 0x020) barHor(x, y+d, w, l, color);                  // middle
  if (c & 0x040) barHor(x, y, w, l, color);                    // up

  if (c & 0x080) {
    TFT_fillRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, color);         // low point
    if (cfont.offset) TFT_drawRect(x+(d/2), y+2*d, 2*w+1, 2*w+1, cfont.color);
  }
  if (c & 0x100) {
    TFT_fillRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, color);       // down middle point
    if (cfont.offset) TFT_drawRect(x+(d/2), y+d+2*w+1, 2*w+1, l/2, cfont.color);
  }
  if (c & 0x800) {
    TFT_fillRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, color); // up middle point
    if (cfont.offset) TFT_drawRect(x+(d/2), y+(2*w)+1+(l/2), 2*w+1, l/2, cfont.color);
  }
  if (c & 0x200) {
    TFT_fillRect(x+2*w+1, y+d, l, 2*w+1, color);               // middle, minus
    if (cfont.offset) TFT_drawRect(x+2*w+1, y+d, l, 2*w+1, cfont.color);
  }
}
//==============================================================================


//---------------------------------------------
static void TFT_print(char *st, int x, int y) {
  int stl, i, tmpw, tmph, fh;
  uint8_t ch;

  if (cfont.bitmap == 0) return; // wrong font selected

  // for rotated string x cannot be RIGHT or CENTER
  if ((rotation != 0) && ((x < -2) || (y < -2))) return;

  stl = strlen(st); // number of characters in string to print

  // set CENTER or RIGHT possition
  tmpw = getStringWidth(st);
  fh = cfont.y_size; // font height
  if ((cfont.x_size != 0) && (cfont.bitmap == 2)) {
    // 7-segment font
    fh = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);  // character height
  }

  if (x==RIGHT) x = dispWin.x2 - tmpw - 1;
  if (x==CENTER) x = (dispWin.x2 - tmpw - 1)/2;
  if (y==BOTTOM) y = dispWin.y2 - fh - 1;
  if (y==CENTER) y = (dispWin.y2 - (fh/2) - 1)/2;
  if (x < dispWin.x1) x = dispWin.x1;
  if (y < dispWin.y1) y = dispWin.y1;

  TFT_X = x;
  TFT_Y = y;
  int offset = TFT_OFFSET;


  tmph = cfont.y_size; // font height
  // for non-proportional fonts, char width is the same for all chars
  if (cfont.x_size != 0) {
    if (cfont.bitmap == 2) { // 7-segment font
      tmpw = (2 * (2 * cfont.y_size + 1)) + cfont.x_size;        // character width
      tmph = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);  // character height
    }
    else tmpw = cfont.x_size;
  }
  if ((TFT_Y + tmph - 1) > dispWin.y2) return;

  // adjust y position


  for (i=0; i<stl; i++) {
    ch = *st++; // get char

    if (cfont.x_size == 0) {
      // for proportional font get char width
      if (getCharPtr(ch)) tmpw = fontChar.xDelta;
    }

    if (ch == 0x0D) { // === '\r', erase to eol ====
      if ((!_transparent) && (rotation==0)) TFT_fillRect(TFT_X, TFT_Y,  dispWin.x2+1-TFT_X, tmph, _bg);
    }

    else if (ch == 0x0A) { // ==== '\n', new line ====
      if (cfont.bitmap == 1) {
        TFT_Y += tmph;
        if (TFT_Y > (dispWin.y2-tmph)) break;
        TFT_X = dispWin.x1;
      }
    }

    else { // ==== other characters ====
      // check if character can be displayed in the current line
      if ((TFT_X+tmpw) > (dispWin.x2+1)) {
        if (_wrap == 0) break;
        TFT_Y += tmph;
        if (TFT_Y > (dispWin.y2-tmph)) break;
        TFT_X = dispWin.x1;
      }

      // Let's print the character
      if (cfont.x_size == 0) {
        // == proportional font
        if (rotation==0) {
          TFT_X += printProportionalChar(TFT_X, TFT_Y)+1;
        }
        else {
          offset += rotatePropChar(x, y, offset);
          TFT_OFFSET = offset;
        }
      }
      // == fixed font
      else {
        if (cfont.bitmap == 1) {
          if ((ch < cfont.offset) || ((ch-cfont.offset) > cfont.numchars)) ch = cfont.offset;
          if (rotation==0) {
            printChar(ch, TFT_X, TFT_Y);
            TFT_X += tmpw;
          }
          else rotateChar(ch, x, y, i);
        }
        else if (cfont.bitmap == 2) { // 7-seg font
          TFT_draw7seg(TFT_X, TFT_Y, ch, cfont.y_size, cfont.x_size, _fg);
          TFT_X += (tmpw + 2);
        }
      }
    }
  }
}

//========================================
static int compile_font_file(lua_State* L)
{
	char outfile[128] = {'\0'};
	size_t len;
    struct stat sb;

	const char *fontfile = luaL_checklstring( L, 1, &len );

	// check here that filename end with ".c".
	if ((len < 3) || (len > 125) || (strcmp(fontfile + len - 2, ".c") != 0)) return luaL_error(L, "not a .c file");

	sprintf(outfile, "%s", fontfile);
	sprintf(outfile+strlen(outfile)-1, "fon");

	// Open the source file
    if (stat(fontfile, &sb) != 0) {
    	return luaL_error(L, "Error opening source file '%s'", fontfile);
    }
    // Open the file
    FILE *ffd = fopen(fontfile, "r");
    if (!ffd) {
    	return luaL_error(L, "Error opening source file '%s'", fontfile);
    }

	// Open the font file
    FILE *ffd_out= fopen(outfile, "w");
	if (!ffd_out) {
		fclose(ffd);
		return luaL_error(L, "error opening destination file");
	}

	// Get file size
	int fsize = sb.st_size;
	if (fsize <= 0) {
		fclose(ffd);
		fclose(ffd_out);
		return luaL_error(L, "source file size error");
	}

	char *sourcebuf = malloc(fsize+4);
	if (sourcebuf == NULL) {
		fclose(ffd);
		fclose(ffd_out);
		return luaL_error(L, "memory allocation error");
	}
	char *fbuf = sourcebuf;

	int rdsize = fread(fbuf, 1, fsize, ffd);
	fclose(ffd);

	if (rdsize != fsize) {
		free(fbuf);
		fclose(ffd_out);
		return luaL_error(L, "error reading from source file");
	}

	*(fbuf+rdsize) = '\0';

	fbuf = strchr(fbuf, '{');			// beginning of font data
	char *fend = strstr(fbuf, "};");	// end of font data

	if ((fbuf == NULL) || (fend == NULL) || ((fend-fbuf) < 22)) {
		free(fbuf);
		fclose(ffd_out);
		return luaL_error(L, "wrong source file format");
	}

	fbuf++;
	*fend = '\0';
	char hexstr[5] = {'\0'};
	int lastline = 0;

	fbuf = strstr(fbuf, "0x");
	int size = 0;
	char *nextline;
	char *numptr;

	int bptr = 0;

	while ((fbuf != NULL) && (fbuf < fend) && (lastline == 0)) {
		nextline = strchr(fbuf, '\n'); // beginning of the next line
		if (nextline == NULL) {
			nextline = fend-1;
			lastline++;
		}
		else nextline++;

		while (fbuf < nextline) {
			numptr = strstr(fbuf, "0x");
			if ((numptr == NULL) || ((fbuf+4) > nextline)) numptr = strstr(fbuf, "0X");
			if ((numptr != NULL) && ((numptr+4) <= nextline)) {
				fbuf = numptr;
				if (bptr >= 128) {
					// buffer full, write to file
                    if (fwrite(outfile, 1, 128, ffd_out) != 128) goto error;
					bptr = 0;
					size += 128;
				}
				memcpy(hexstr, fbuf, 4);
				hexstr[4] = 0;
				outfile[bptr++] = (uint8_t)strtol(hexstr, NULL, 0);
				fbuf += 4;
			}
			else fbuf = nextline;
		}
		fbuf = nextline;
	}

	if (bptr > 0) {
		size += bptr;
        if (fwrite(outfile, 1, bptr, ffd_out) != bptr) goto error;
	}

	// write font ID
	sprintf(outfile, "RPH_font");
    if (fwrite(outfile, 1, 8, ffd_out) != 8) goto error;

	fclose(ffd_out);

	free(sourcebuf);

	// === Test compiled font ===
	sprintf(outfile, "%s", fontfile);
	sprintf(outfile+strlen(outfile)-1, "fon");

	uint8_t *uf = userfont; // save userfont pointer
	userfont = NULL;
	if (load_file_font(outfile, 1) == 0) printf("Error compiling file!\n");
	else {
		free(userfont);
		printf("File compiled successfully.\n");
	}
	userfont = uf; // restore userfont

	return 0;

error:
	fclose(ffd_out);
	free(sourcebuf);
	return luaL_error(L, "error writing to destination file");
}


// ================ Service functions ==========================================

// Change the screen rotation.
// Input: m new rotation value (0 to 3)
//--------------------------------------
static void TFT_setRotation(uint8_t m) {
  uint8_t rotation = m & 3; // can't be higher than 3
  uint8_t send = 1;
  uint8_t madctl = 0;

  if (m > 3) madctl = (m & 0xF8); // for testing, manually set MADCTL register
  else {
	  orientation = m;
	  if (TFT_type == 0) {
		if ((rotation & 1)) {
			_width  = ST7735_HEIGHT;
			_height = ST7735_WIDTH;
		}
		else {
			_width  = ST7735_WIDTH;
			_height = ST7735_HEIGHT;
		}
		switch (rotation) {
		  case PORTRAIT:
			madctl = (MADCTL_MX | MADCTL_MY | MADCTL_RGB);
			break;
		  case LANDSCAPE:
			madctl = (MADCTL_MY | MADCTL_MV | MADCTL_RGB);
			break;
		  case PORTRAIT_FLIP:
			madctl = (MADCTL_RGB);
			break;
		  case LANDSCAPE_FLIP:
			madctl = (MADCTL_MX | MADCTL_MV | MADCTL_RGB);
			break;
		}
	  }
	  else if (TFT_type == 1) {
		if ((rotation & 1)) {
			_width  = ILI9341_HEIGHT;
			_height = ILI9341_WIDTH;
		}
		else {
			_width  = ILI9341_WIDTH;
			_height = ILI9341_HEIGHT;
		}
		switch (rotation) {
		  case PORTRAIT:
			madctl = (MADCTL_MX | MADCTL_BGR);
			break;
		  case LANDSCAPE:
			madctl = (MADCTL_MV | MADCTL_BGR);
			break;
		  case PORTRAIT_FLIP:
			madctl = (MADCTL_MY | MADCTL_BGR);
			break;
		  case LANDSCAPE_FLIP:
			madctl = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
			break;
		}
	  }
	  /*
	  else if (TFT_type == 2) {
		_width  = XADOW_WIDTH;
		_height = XADOW_HEIGHT;
		switch (rotation) {
		  case PORTRAIT:
			madctl = 0;
			break;
		  case LANDSCAPE:
			madctl = (MADCTL_MV | MADCTL_MX);
			break;
		  case PORTRAIT_FLIP:
			madctl = 0;
			rotation = PORTRAIT;
			break;
		  case LANDSCAPE_FLIP:
			madctl = (MADCTL_MV | MADCTL_MX);
			rotation = LANDSCAPE;
			break;
		}
	  }
	  */
	  else send = 0;
  }

  if (send) {
	  tft_cmd(TFT_MADCTL);
	  tft_data(&madctl, 1);
  }

  dispWin.x1 = 0;
  dispWin.y1 = 0;
  dispWin.x2 = _width-1;
  dispWin.y2 = _height-1;

}

// Send the command to invert all of the colors.
// Input: i 0 to disable inversion; non-zero to enable inversion
//-------------------------------------------------
static void TFT_invertDisplay(const uint8_t mode) {
  if ( mode == INVERT_ON ) tft_cmd(TFT_INVONN);
  else tft_cmd(TFT_INVOFF);
}

//--------------------------------------------------
static uint8_t checkParam(uint8_t n, lua_State* L) {
  if (lua_gettop(L) < n) {
	syslog(LOG_ERR, "not enough parameters" );
    return 1;
  }
  return 0;
}

/**
 * Converts the components of a color, as specified by the HSB
 * model, to an equivalent set of values for the default RGB model.
 * The _sat and _brightnesscomponents
 * should be floating-point values between zero and one (numbers in the range 0.0-1.0)
 * The _hue component can be any floating-point number.  The floor of this number is
 * subtracted from it to create a fraction between 0 and 1.
 * This fractional number is then multiplied by 360 to produce the hue
 * angle in the HSB color model.
 * The integer that is returned by HSBtoRGB encodes the
 * value of a color in bits 0-15 of an integer value
*/
//-------------------------------------------------------------------
static uint16_t HSBtoRGB(float _hue, float _sat, float _brightness) {
 float red = 0.0;
 float green = 0.0;
 float blue = 0.0;

 if (_sat == 0.0) {
   red = _brightness;
   green = _brightness;
   blue = _brightness;
 } else {
   if (_hue == 360.0) {
     _hue = 0;
   }

   int slice = (int)(_hue / 60.0);
   float hue_frac = (_hue / 60.0) - slice;

   float aa = _brightness * (1.0 - _sat);
   float bb = _brightness * (1.0 - _sat * hue_frac);
   float cc = _brightness * (1.0 - _sat * (1.0 - hue_frac));

   switch(slice) {
     case 0:
         red = _brightness;
         green = cc;
         blue = aa;
         break;
     case 1:
         red = bb;
         green = _brightness;
         blue = aa;
         break;
     case 2:
         red = aa;
         green = _brightness;
         blue = cc;
         break;
     case 3:
         red = aa;
         green = bb;
         blue = _brightness;
         break;
     case 4:
         red = cc;
         green = aa;
         blue = _brightness;
         break;
     case 5:
         red = _brightness;
         green = aa;
         blue = bb;
         break;
     default:
         red = 0.0;
         green = 0.0;
         blue = 0.0;
         break;
   }
 }

 uint8_t ired = (uint8_t)(red * 31.0);
 uint8_t igreen = (uint8_t)(green * 63.0);
 uint8_t iblue = (uint8_t)(blue * 31.0);

 return (uint16_t)((ired << 11) | (igreen << 5) | (iblue & 0x001F));
}

//-------------------------------------------------
static uint16_t getColor(lua_State* L, uint8_t n) {
  if( lua_istable( L, n ) ) {
    uint8_t i;
    uint8_t cl[3];
    uint8_t datalen = lua_rawlen( L, n );
    if (datalen < 3) return _fg;

    for( i = 0; i < 3; i++ )
    {
      lua_rawgeti( L, n, i + 1 );
      cl[i] = ( int )luaL_checkinteger( L, -1 );
      lua_pop( L, 1 );
    }
    if (cl[0] > 0x1F) cl[0] = 0x1F;
    if (cl[1] > 0x3F) cl[1] = 0x3F;
    if (cl[2] > 0x1F) cl[2] = 0x1F;
    return (cl[0] << 11) | (cl[1] << 5) | cl[2];
  }
  else {
    return luaL_checkinteger( L, n );
  }
}

//-------------------------------
static int _check(lua_State* L) {
	if (TFT_type < 0) {
		return luaL_error( L, "Display not initialized" );
	}
	return 0;
}

//--------------------------
static void _initvar(void) {
  rotation = 0;
  _wrap = 0;
  _transparent = 0;
  _forceFixed = 0;
  dispWin.x2 = _width-1;
  dispWin.y2 = _height-1;
  dispWin.x1 = 0;
  dispWin.y1 = 0;
}



// ================ JPG SUPPORT ================================================
// User defined device identifier
typedef struct {
	FILE *fhndl;		// File handler for input function
    uint16_t x;			// image top left point X position
    uint16_t y;			// image top left point Y position
    uint8_t *membuff;	// memory buffer containing the image
    uint32_t bufsize;	// size of the memory buffer
    uint32_t bufptr;	// memory buffer current possition
} JPGIODEV;


// User defined call-back function to input JPEG data from file
//---------------------
static UINT tjd_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	int rb = 0;
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	if (buff) {	// Read nd bytes from the input strem
		rb = fread(buff, 1, nd, dev->fhndl);
		return rb;	// Returns actual number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		if (fseek(dev->fhndl, nd, SEEK_CUR) >= 0) return nd;
		else return 0;
	}
}

// User defined call-back function to input JPEG data from memory buffer
//-------------------------
static UINT tjd_buf_input (
	JDEC* jd,		// Decompression object
	BYTE* buff,		// Pointer to the read buffer (NULL:skip)
	UINT nd			// Number of bytes to read/skip from input stream
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;
	if (!dev->membuff) return 0;
	if (dev->bufptr >= (dev->bufsize + 2)) return 0; // end of stream

	if ((dev->bufptr + nd) > (dev->bufsize + 2)) nd = (dev->bufsize + 2) - dev->bufptr;

	if (buff) {	// Read nd bytes from the input strem
		memcpy(buff, dev->membuff + dev->bufptr, nd);
		dev->bufptr += nd;
		return nd;	// Returns number of bytes read
	}
	else {	// Remove nd bytes from the input stream
		dev->bufptr += nd;
		return nd;
	}
}

// User defined call-back function to output RGB bitmap
//----------------------
static UINT tjd_output (
	JDEC* jd,		// Decompression object of current session
	void* bitmap,	// Bitmap data to be output
	JRECT* rect		// Rectangular region to output
)
{
	// Device identifier for the session (5th argument of jd_prepare function)
	JPGIODEV *dev = (JPGIODEV*)jd->device;

	// ** Put the rectangular into the display device **
	uint16_t x;
	uint16_t y;
	BYTE *src = (BYTE*)bitmap;
	uint16_t left = rect->left + dev->x;
	uint16_t top = rect->top + dev->y;
	uint16_t right = rect->right + dev->x;
	uint16_t bottom = rect->bottom + dev->y;
	uint16_t color;

	if ((left >= _width) || (top >= _height)) return 1;	// out of screen area, return

	int len = ((right-left+1) * (bottom-top+1));		// calculate length of data

	if ((len > 0) && (len <= (TFT_LINEBUF_MAX_SIZE))) {
		uint16_t bufidx = 0;

	    for (y = top; y <= bottom; y++) {
		    for (x = left; x <= right; x++) {
		    	// Clip to display area
		    	if ((x < _width) && (y < _height)) {
		    		color = (uint16_t)*src++;
		    		color |= ((uint16_t)*src++) << 8;
		    		tft_line[bufidx++] = color;
		    	}
		    	else src += 2;
		    }
	    }
	    if (right > _width) right = _width-1;
	    if (bottom > _height) bottom = _height-1;
	    send_data(left, top, right, bottom, bufidx, tft_line);
	}
	else {
		syslog(LOG_ERR, "max data size exceded: %d (%d,%d,%d,%d)", len, left,top,right,bottom);
		return 0;  // stop decompression
	}

	return 1;	// Continue to decompression
}

#if CONFIG_LUA_RTOS_LUA_USE_CAM
extern uint8_t *cam_get_image(FILE *fhndl, int *err, uint32_t *bytes_read, uint8_t capture);
#endif

// tft.jpgimage(X, Y, scale, file_name [, from_cam]
//=======================================
static int ltft_jpg_image( lua_State* L )
{
	_check(L);

	const char *fname;
	size_t len;
	JPGIODEV dev;
    struct stat sb;
    uint8_t dbg = 0;

	int x = luaL_checkinteger( L, 1 );
	int y = luaL_checkinteger( L, 2 );
	int maxscale = luaL_checkinteger( L, 3 );
	if ((maxscale < 0) || (maxscale > 3)) maxscale = 3;

	fname = luaL_checklstring( L, 4, &len );

    if (strlen(fname) == 0) return 0;

    if (lua_gettop(L) > 4) {
    	if (luaL_checkinteger( L, 5 ) != 0) dbg = 1;
    }

    if (!tft_line) {
        return luaL_error(L, "Line buffer not allocated");
    }

	#if CONFIG_LUA_RTOS_LUA_USE_CAM
    if ((strcmp(fname, "CAM") == 0) || (strcmp(fname, "cam") == 0)) {
    	// image from camera
    	dev.fhndl = NULL;
    	int err = 0;
        dev.membuff = cam_get_image(NULL, &err, &dev.bufsize, 1);
        if (err != 0) {
            //return luaL_error(L, "Camera error %d", err);
        	return 0;
        }
        dev.bufptr = 2;
    }
    else {
    	// image from file
        dev.membuff = NULL;
        dev.bufsize = 0;

        if (stat(fname, &sb) != 0) {
            return luaL_error(L, strerror(errno));
        }

        dev.fhndl = fopen(fname, "r");
        if (!dev.fhndl) {
            return luaL_error(L, strerror(errno));
        }
    }
	#else
	// image from file
    dev.membuff = NULL;
    dev.bufsize = 0;

    if (stat(fname, &sb) != 0) {
        return luaL_error(L, strerror(errno));
    }

    dev.fhndl = fopen(fname, "r");
    if (!dev.fhndl) {
        return luaL_error(L, strerror(errno));
    }
	#endif

	char *work;				// Pointer to the working buffer (must be 4-byte aligned)
	UINT sz_work = 3800;	// Size of the working buffer (must be power of 2)
	JDEC jd;				// Decompression object (70 bytes)
	JRESULT rc;
	BYTE scale = 0;
	uint8_t radj = 0;
	uint8_t badj = 0;

	if ((x < 0) && (x != CENTER) && (x != RIGHT)) x = 0;
	if ((y < 0) && (y != CENTER) && (y != BOTTOM)) y = 0;
	if (x > (_width-5)) x = _width - 5;
	if (y > (_height-5)) y = _height - 5;

	work = malloc(sz_work);
	if (work) {
		if (dev.membuff) rc = jd_prepare(&jd, tjd_buf_input, (void *)work, sz_work, &dev);
		else rc = jd_prepare(&jd, tjd_input, (void *)work, sz_work, &dev);
		if (rc == JDR_OK) {
			if (x == CENTER) {
				x = _width - (jd.width >> scale);
				if (x < 0) {
					if (maxscale) {
						for (scale = 0; scale <= maxscale; scale++) {
							if (((jd.width >> scale) <= (_width)) && ((jd.height >> scale) <= (_height))) break;
							if (scale == maxscale) break;
						}
						x = _width - (jd.width >> scale);
						if (x < 0) x = 0;
						else x >>= 1;
						maxscale = 0;
					}
					else x = 0;
				}
				else x >>= 1;
			}
			if (y == CENTER) {
				y = _height - (jd.height >> scale);
				if (y < 0) {
					if (maxscale) {
						for (scale = 0; scale <= maxscale; scale++) {
							if (((jd.width >> scale) <= (_width)) && ((jd.height >> scale) <= (_height))) break;
							if (scale == maxscale) break;
						}
						y = _height - (jd.height >> scale);
						if (y < 0) y = 0;
						else y >>= 1;
						maxscale = 0;
					}
					else y = 0;
				}
				else y >>= 1;
			}
			if (x == RIGHT) {
				x = 0;
				radj = 1;
			}
			if (y == BOTTOM) {
				y = 0;
				badj = 1;
			}
			// Determine scale factor
			if (maxscale) {
				for (scale = 0; scale <= maxscale; scale++) {
					if (((jd.width >> scale) <= (_width-x)) && ((jd.height >> scale) <= (_height-y))) break;
					if (scale == maxscale) break;
				}
			}
			if (dbg) printf("Image dimensions: %dx%d, scale: %d, bytes used: %d\r\n", jd.width, jd.height, scale, jd.sz_pool);

			if (radj) {
				x = _width - (jd.width >> scale);
				if (x < 0) x = 0;
			}
			if (badj) {
				y = _height - (jd.height >> scale);
				if (y < 0) y = 0;
			}
			dev.x = x;
			dev.y = y;
			// Start to decompress the JPEG file
			rc = jd_decomp(&jd, tjd_output, scale);
			if (rc != JDR_OK) {
				if (dbg) printf("jpg decompression error %d\r\n", rc);
			}
		}
		else {
			if (dbg) printf("jpg prepare error %d\r\n", rc);
		}

		free(work);  // free work buffer
	}
	else {
		if (dbg) printf("work buffer allocation error\r\n");
	}

    if (dev.fhndl) fclose(dev.fhndl);  // close input file
    if (dev.membuff) free(dev.membuff);

    return 0;
}

//==================================
static int tft_image( lua_State* L )
{
  _check(L);

  if (checkParam(5, L)) return 0;

  const char *fname;
  FILE *fhndl = 0;
  struct stat sb;
  uint32_t xrd = 0;
  size_t len;

  int x = luaL_checkinteger( L, 1 );
  int y = luaL_checkinteger( L, 2 );
  int xsize = luaL_checkinteger( L, 3 );
  int ysize = luaL_checkinteger( L, 4 );
  fname = luaL_checklstring( L, 5, &len );

  if (strlen(fname) == 0) return 0;

  if (x == CENTER) x = (_width - xsize) / 2;
  else if (x == RIGHT) x = (_width - xsize);
  if (x < 0) x = 0;

  if (y == CENTER) y = (_height - ysize) / 2;
  else if (y == BOTTOM) y = (_height - ysize);
  if (y < 0) y = 0;

  // crop to disply width
  int xend;
  if ((x+xsize) > _width) xend = _width-1;
  else xend = x+xsize-1;
  int disp_xsize = xend-x+1;
  if ((disp_xsize <= 1) || (y >= _height)) {
	  syslog(LOG_ERR, "image out of screen.");
	  return 0;
  }

  if (stat(fname, &sb) != 0) {
      return luaL_error(L, strerror(errno));
  }

  if (!tft_line) {
      return luaL_error(L, "Line buffer not allocated");
  }

  fhndl = fopen(fname, "r");
  if (!fhndl) {
      return luaL_error(L, strerror(errno));
  }

  disp_xsize *= 2;
  do { // read 1 image line from file and send to display
	xrd = fread(tft_line, 2, xsize, fhndl);  // read line from file
	if (xrd != xsize) {
		syslog(LOG_ERR, "Error reading line: %d", xrd);
		break;
	}

    send_data(x, y, xend, y, xsize, tft_line);

	y++;
	if (y >= _height) break;
	ysize--;

  } while (ysize > 0);

  fclose(fhndl);

  return 0;
}

//=====================================
static int tft_bmpimage( lua_State* L )
{
	_check(L);

	if (checkParam(3, L)) return 0;

	const char *fname;
	char *basename;
	FILE *fhndl = 0;
	struct stat sb;
	uint8_t *buf = NULL;
	uint32_t xrd = 0;
	size_t len;

	int x = luaL_checkinteger( L, 1 );
	int y = luaL_checkinteger( L, 2 );
	fname = luaL_checklstring( L, 3, &len );

	if (strlen(fname) == 0) return 0;

	basename = strrchr(fname, '/');
	if (basename == NULL) basename = (char *)fname;
	else basename++;
	if (strlen(basename) == 0) return 0;

	if (stat(fname, &sb) != 0) {
		return luaL_error(L, strerror(errno));
	}

    if (!tft_line) {
	    return luaL_error(L, "Line buffer not allocated");
    }

    // Allocate buffer for reading one line of display data
    // 3 * TFT_MAX_DISP_SIZE
    buf = malloc(TFT_MAX_DISP_SIZE*3);
    if (!buf) {
	    return luaL_error(L, "File buffer allocation error");
    }

    fhndl = fopen(fname, "r");
	if (!fhndl) {
		free(buf);
		return luaL_error(L, strerror(errno));
	}

    xrd = fread(buf, 1, 54, fhndl);  // read header
	if (xrd != 54) {
exithd:
		free(buf);
		fclose(fhndl);
		syslog(LOG_ERR, "Error reading header");
		return 0;
	}

	uint16_t wtemp;
	uint32_t temp;
	uint32_t offset;
	uint32_t xsize;
	uint32_t ysize;

	// Check image header
	if ((buf[0] != 'B') || (buf[1] != 'M')) goto exithd;

	memcpy(&offset, buf+10, 4);
	memcpy(&temp, buf+14, 4);
	if (temp != 40) goto exithd;
	memcpy(&wtemp, buf+26, 2);
	if (wtemp != 1) goto exithd;
	memcpy(&wtemp, buf+28, 2);
	if (wtemp != 24) goto exithd;
	memcpy(&temp, buf+30, 4);
	if (temp != 0) goto exithd;

	memcpy(&xsize, buf+18, 4);
	memcpy(&ysize, buf+22, 4);

	// Adjust position
	if (x == CENTER) x = (_width - xsize) / 2;
	else if (x == RIGHT) x = (_width - xsize);
	if (x < 0) x = 0;

	if (y == CENTER) y = (_height - ysize) / 2;
	else if (y == BOTTOM) y = (_height - ysize);
	if (y < 0) y = 0;

	// Crop to display width
	int xend;
	if ((x+xsize) > _width) xend = _width-1;
	else xend = x+xsize-1;
	int disp_xsize = xend-x+1;
	if ((disp_xsize <= 1) || (y >= _height)) {
		syslog(LOG_ERR, "image out of screen.");
		goto exit;
	}

	int i;
	while (ysize > 0) {
		// Position at line start
		// ** BMP images are stored in file from LAST to FIRST line
		//    so we have to read from the end line first

		if (fseek(fhndl, offset+((ysize-1)*(xsize*3)), SEEK_SET) != 0) break;

		// ** read one image line from file and send to display **
		// read only the part of image line which can be shown on screen
		xrd = fread(buf, 1, disp_xsize*3, fhndl);  // read line from file
		if (xrd != (disp_xsize*3)) {
			syslog(LOG_ERR, "Error reading line: %d (%d)", y, xrd);
			break;
		}
		// Convert colors to RGB565 format and place to line buffer
		uint8_t *linebuf = (uint8_t *)tft_line;
		for (i=0;i < xrd;i += 3) {
			// get RGB888 and convert to RGB565
			// BMP BYTES ORDER: B8-G8-R8 !!
			*linebuf    = buf[i+2] & 0xF8;			// R5
			*linebuf++ |= buf[i+1] >> 5;			// G6 Hi
			*linebuf    = (buf[i+1] << 3) & 0xE0;	// G6 Lo
			*linebuf++ |= buf[i] >> 3;				// B5
		}
	    send_data(x, y, xend, y, disp_xsize, tft_line);

		y++;	// next image line
		if (y >= _height) break;
		ysize--;
	}

exit:
	free(buf);
	fclose(fhndl);

	return 0;
}


//====================================
static int ltft_init( lua_State* L ) {
    uint8_t typ = luaL_checkinteger( L, 1);
    int orient = luaL_optinteger( L, 2, LANDSCAPE);
    uint8_t cs = luaL_optinteger( L, 3, CONFIG_LUA_RTOS_TFT_CS);
    uint8_t tcs = luaL_optinteger( L, 4, CONFIG_LUA_RTOS_TFT_TP_CS);

    TFT_setFont(DEFAULT_FONT, NULL);
    _fg = TFT_GREEN;
    _bg = TFT_BLACK;

    if (typ < 3) TFT_type = 0;							// ST7735 type display
    else if ((typ == 3) || (typ == 4)) TFT_type = 1;	// ILI7341 type display
    else {
        return luaL_error( L, "unsupported display chipset" );
    }

    disp_spi = tft_spi_init(L, typ, cs, tcs);

    TFT_setRotation(orient % 4);
    TFT_fillScreen(TFT_BLACK);
    _initvar();

	return 0;
}

//==================================
static int tft_gettype(lua_State *L)
{
    lua_pushinteger( L, TFT_type);
    return 1;
}

//=========================================
static int tft_set_brightness(lua_State *L)
{
    return 0;
}

//======================================
static int tft_setorient( lua_State* L )
{
	_check(L);

	orientation = luaL_checkinteger( L, 1 );
	TFT_setRotation(orientation);
	TFT_fillScreen(_bg);

	return 0;
}

//==================================
static int tft_clear( lua_State* L )
{
	_check(L);

	uint16_t color = TFT_BLACK;

	if (lua_gettop(L) > 0) color = getColor( L, 1 );

	TFT_pushColorRep(0, 0, _width-1, _height-1, color, (uint32_t)(_height*_width));

	_bg = color;
	_initvar();

	return 0;
}

//===================================
static int tft_invert( lua_State* L )
{
	_check(L);

	uint16_t inv = luaL_checkinteger( L, 1 );
	TFT_invertDisplay(inv);
	return 0;
}

//====================================
static int tft_setwrap( lua_State* L )
{
	_wrap = luaL_checkinteger( L, 1 );
	return 0;
}

//======================================
static int tft_settransp( lua_State* L )
{
	_transparent = luaL_checkinteger( L, 1 );
	return 0;
}

//===================================
static int tft_setrot( lua_State* L )
{
	rotation = luaL_checkinteger( L, 1 );
	return 0;
}

//===================================
static int tft_setfixed( lua_State* L )
{
	_forceFixed = luaL_checkinteger( L, 1 );
	return 0;
}

//====================================
static int tft_setfont( lua_State* L )
{
  if (checkParam(1, L)) return 0;

  uint8_t fnt = DEFAULT_FONT;
  size_t fnlen = 0;
  const char* fname = NULL;

  if (lua_type(L, 1) == LUA_TNUMBER) {
	  fnt = luaL_checkinteger( L, 1 );
  }
  else if (lua_type(L, 1) == LUA_TSTRING) {
	  fnt = USER_FONT;
	  fname = lua_tolstring(L, -1, &fnlen);
  }

  TFT_setFont(fnt, fname);

  if (fnt == FONT_7SEG) {
    if (lua_gettop(L) > 2) {
      uint8_t l = luaL_checkinteger( L, 2 );
      uint8_t w = luaL_checkinteger( L, 3 );
      if (l < 6) l = 6;
      if (l > 40) l = 40;
      if (w < 1) w = 1;
      if (w > (l/2)) w = l/2;
      if (w > 12) w = 12;
      cfont.x_size = l;
      cfont.y_size = w;
      cfont.offset = 0;
      cfont.color  = _fg;
      if (lua_gettop(L) > 3) {
        if (w > 1) {
          cfont.offset = 1;
          cfont.color  = getColor( L, 4 );
        }
      }
    }
    else {  // default size
      cfont.x_size = 12;
      cfont.y_size = 2;
    }
  }
  return 0;
}

//========================================
static int tft_getfontsize( lua_State* L )
{
  if (cfont.bitmap == 1) {
    if (cfont.x_size != 0) lua_pushinteger( L, cfont.x_size );
    else lua_pushinteger( L, getMaxWidth() );
    lua_pushinteger( L, cfont.y_size );
  }
  else if (cfont.bitmap == 2) {
    lua_pushinteger( L, (2 * (2 * cfont.y_size + 1)) + cfont.x_size );
    lua_pushinteger( L, (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size) );
  }
  else {
    lua_pushinteger( L, 0);
    lua_pushinteger( L, 0);
  }
  return 2;
}

//==========================================
static int tft_getscreensize( lua_State* L )
{
	lua_pushinteger( L, _width);
	lua_pushinteger( L, _height);
	return 2;
}

//==========================================
static int tft_getfontheight( lua_State* L )
{
  if (cfont.bitmap == 1) {
	// Bitmap font
    lua_pushinteger( L, cfont.y_size );
  }
  else if (cfont.bitmap == 2) {
	// 7-segment font
    lua_pushinteger( L, (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size) );
  }
  else {
    lua_pushinteger( L, 0);
  }
  return 1;
}

//===============================
static int tft_on( lua_State* L )
{
	_check(L);
	tft_cmd(TFT_DISPON);
	return 0;
}

//================================
static int tft_off( lua_State* L )
{
	_check(L);
	tft_cmd(TFT_DISPOFF);
	return 0;
}

//=====================================
static int tft_setcolor( lua_State* L )
{
	if (checkParam(1, L)) return 0;

	_fg = getColor( L, 1 );
	if (lua_gettop(L) > 1) _bg = getColor( L, 2 );
	return 0;
}

//=======================================
static int tft_setclipwin( lua_State* L )
{
	if (checkParam(4, L)) return 0;

	dispWin.x1 = luaL_checkinteger( L, 1 );
	dispWin.y1 = luaL_checkinteger( L, 2 );
	dispWin.x2 = luaL_checkinteger( L, 3 );
	dispWin.y2 = luaL_checkinteger( L, 4 );

	if (dispWin.x2 >= _width) dispWin.x2 = _width-1;
	if (dispWin.y2 >= _height) dispWin.y2 = _height-1;
	if (dispWin.x1 > dispWin.x2) dispWin.x1 = dispWin.x2;
	if (dispWin.y1 > dispWin.y2) dispWin.y1 = dispWin.y2;

	return 0;
}

//=========================================
static int tft_resetclipwin( lua_State* L )
{
	dispWin.x2 = _width-1;
	dispWin.y2 = _height-1;
	dispWin.x1 = 0;
	dispWin.y1 = 0;

	return 0;
}

//=====================================
static int tft_HSBtoRGB( lua_State* L )
{
	float hue = luaL_checknumber(L, 1);
	float sat = luaL_checknumber(L, 2);
	float bri = luaL_checknumber(L, 3);

	lua_pushinteger(L, HSBtoRGB(hue, sat, bri));

	return 1;
}

//=====================================
static int tft_putpixel( lua_State* L )
{
	_check(L);
	if (checkParam(2, L)) return 0;

	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t color = _fg;

	if (lua_gettop(L) > 2) color = getColor( L, 3 );

	TFT_drawPixel(x, y, color, 1);

	return 0;
}

//=====================================
static int tft_getpixel( lua_State* L )
{
	_check(L);
	if (checkParam(2, L)) return 0;

	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t color = TFT_readPixel(x,y);

	lua_pushinteger(L, color);

	return 1;
}

//======================================
static int tft_getwindow( lua_State* L )
{
	_check(L);
	if (checkParam(4, L)) return 0;

    int out_type = 0;
    luaL_Buffer b;
    char hbuf[8];

    if ((lua_gettop(L) > 4) && (lua_isstring(L, 4))) {
        const char* sarg;
        size_t sarglen;
        sarg = luaL_checklstring(L, 5, &sarglen);
        if (sarglen == 2) {
        	if (strstr(sarg, "*h") != NULL) out_type = 1;
        	else if (strstr(sarg, "*t") != NULL) out_type = 2;
        }
    }

    int16_t x = luaL_checkinteger( L, 1 );
	int16_t y = luaL_checkinteger( L, 2 );
	int w = luaL_checkinteger( L, 3 );
	int h = luaL_checkinteger( L, 3 );
	uint8_t f = 0;
	if (w < 1) w = 1;
	if (w > _width) w = _width;
	if (h < 1) h = 1;
	if (h > _height) h = _height;
	if ((x + w) > _width) w = _width - x;
	if ((y + h) > _height) h = _height - h;

	int len = w*h;
	if ((y < 0) || (y > (_height-1))) f= 1;
	else if ((x < 0) || (x > (_width-1))) f= 1;
	else if (len > (TFT_LINEBUF_MAX_SIZE)) f = 1;
	if (f) {
        return luaL_error( L, "wrong coordinates or size > %d", TFT_LINEBUF_MAX_SIZE );
	}

	int err = read_data(x, y, x+w, y+h, len, (uint8_t *)tft_line);
    if (err < 0) {
        return luaL_error( L, "Error reading display data (%d)", err );
    }

    if (out_type < 2) luaL_buffinit(L, &b);
    else lua_newtable(L);

	if (out_type == 0) {
		luaL_addlstring(&b, (const char *)tft_line, len);
	}
	else {
		for (int i = 0; i < len; i++) {
			if (out_type == 1) {
				sprintf(hbuf, "%04x;", tft_line[i]);
				luaL_addstring(&b, hbuf);
			}
			else {
				lua_pushinteger( L, tft_line[i]);
				lua_rawseti(L, -2, i+1);
			}
		}
	}

    if (out_type < 2) luaL_pushresult(&b);

    return 1;
}

//=====================================
static int tft_drawline( lua_State* L )
{
	_check(L);
	if (checkParam(4, L)) return 0;

	uint16_t color = _fg;
	if (lua_gettop(L) > 4) color = getColor( L, 5 );
	uint16_t x0 = luaL_checkinteger( L, 1 );
	uint16_t y0 = luaL_checkinteger( L, 2 );
	uint16_t x1 = luaL_checkinteger( L, 3 );
	uint16_t y1 = luaL_checkinteger( L, 4 );
	TFT_drawLine(x0,y0,x1,y1,color);

	return 0;
}

//============================================
static int tft_drawlineByAngle( lua_State* L )
{
	_check(L);
	if (checkParam(4, L)) return 0;

	uint16_t color = _fg;
	uint16_t start = 0;
	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t len = luaL_checkinteger( L, 3 );
	uint16_t angle = luaL_checkinteger( L, 4 );

	if (lua_gettop(L) > 4) color = getColor( L, 5 );
	if (lua_gettop(L) > 5) start = luaL_checkinteger( L, 6 );
	if (start >= len) start = len-1;

	if (start == 0) drawLineByAngle(x, y, angle, len, color);
	else DrawLineByAngle(x, y, angle, start, len, color);

	return 0;
}

//=================================
static int tft_rect( lua_State* L )
{
	_check(L);
	if (checkParam(5, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 5) fillcolor = getColor( L, 6 );
	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t w = luaL_checkinteger( L, 3 );
	uint16_t h = luaL_checkinteger( L, 4 );
	uint16_t color = getColor( L, 5 );
	if (lua_gettop(L) > 5) TFT_fillRect(x,y,w,h,fillcolor);
	if (fillcolor != color) TFT_drawRect(x,y,w,h,color);

	return 0;
}

//static void rounded_Square(int cx, int cy, int h, int w, float radius, uint16_t color, uint8_t fill)
//======================================
static int tft_roundrect( lua_State* L )
{
	_check(L);
	if (checkParam(6, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 6) fillcolor = getColor( L, 7 );
	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t w = luaL_checkinteger( L, 3 );
	uint16_t h = luaL_checkinteger( L, 4 );
	float r = luaL_checknumber( L, 5 );
	uint16_t color = getColor( L, 6 );
	if (lua_gettop(L) > 6) TFT_fillRoundRect(x,y,w,h,r,fillcolor);
	if (fillcolor != color) TFT_drawRoundRect(x,y,w,h,r,color);

	return 0;
}

//=================================
static int tft_circle( lua_State* L )
{
	_check(L);
	if (checkParam(4, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 4) fillcolor = getColor( L, 5 );
	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t r = luaL_checkinteger( L, 3 );
	uint16_t color = getColor( L, 4 );
	if (lua_gettop(L) > 4) TFT_fillCircle(x,y,r,fillcolor);
	if (fillcolor != color) TFT_drawCircle(x,y,r,color);

	return 0;
}

//====================================
static int tft_ellipse( lua_State* L )
{
	_check(L);
	if (checkParam(5, L)) return 0;

	uint16_t fillcolor = _bg;
	uint8_t opt = 15;

	if (lua_gettop(L) > 5) fillcolor = getColor( L, 6 );
	if (lua_gettop(L) > 6) opt = getColor( L, 7 ) & 0x0F;

	uint16_t x = luaL_checkinteger( L, 1 );
	uint16_t y = luaL_checkinteger( L, 2 );
	uint16_t rx = luaL_checkinteger( L, 3 );
	uint16_t ry = luaL_checkinteger( L, 4 );
	uint16_t color = getColor( L, 5 );

	if (lua_gettop(L) > 5) TFT_draw_filled_ellipse(x, y, rx, ry, fillcolor, opt);
	if (fillcolor != color) TFT_draw_ellipse(x, y, rx, ry, color, opt);

	return 0;
}

//================================
static int tft_arc( lua_State* L )
{
	_check(L);
	if (checkParam(7, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 7) fillcolor = getColor( L, 8 );
	uint16_t cx = luaL_checkinteger( L, 1 );
	uint16_t cy = luaL_checkinteger( L, 2 );
	uint16_t r = luaL_checkinteger( L, 3 );
	uint16_t th = luaL_checkinteger( L, 4 );
	if (th < 1) th = 1;
	if (th > r) th = r;
	float start = luaL_checknumber( L, 5 );
	float end = luaL_checknumber( L, 6 );
	uint16_t color = getColor( L, 7 );

	if (lua_gettop(L) > 7) {
		TFT_fillArcOffsetted(cx, cy, r, th, start, end, fillcolor);
		if (fillcolor != color) {
			TFT_fillArcOffsetted(cx, cy, r, 1, start, end, color);
			TFT_fillArcOffsetted(cx, cy, r-th, 1, start, end, color);
			DrawLineByAngle(cx, cy, start, r-th, th, color);
			DrawLineByAngle(cx, cy, end, r-th, th, color);
		}
	}
	else {
		TFT_fillArcOffsetted(cx, cy, r, th, start, end, color);
	}

	return 0;
}

//static void drawPolygon(int cx, int cy, int sides, int diameter, uint16_t color, bool fill, float deg)
//=================================
static int tft_poly( lua_State* L )
{
	_check(L);
	if (checkParam(6, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 6) fillcolor = getColor( L, 7 );
	uint16_t cx = luaL_checkinteger( L, 1 );
	uint16_t cy = luaL_checkinteger( L, 2 );
	uint16_t sid = luaL_checkinteger( L, 3 );
	uint16_t r = luaL_checkinteger( L, 4 );
	int rot = luaL_checknumber( L, 5 );
	uint16_t color = getColor( L, 6 );

	if (lua_gettop(L) > 6) drawPolygon(cx, cy, sid, r, fillcolor, 1, rot);
	if (fillcolor != color) drawPolygon(cx, cy, sid, r, color, 0, rot);

	return 0;
}

//static void drawStar(int cx, int cy, int diameter, uint16_t color, bool fill, float factor)
//=================================
static int tft_star( lua_State* L )
{
	_check(L);
	if (checkParam(5, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 5) fillcolor = getColor( L, 6 );
	uint16_t cx = luaL_checkinteger( L, 1 );
	uint16_t cy = luaL_checkinteger( L, 2 );
	uint16_t r = luaL_checkinteger( L, 3 );
	float fact = luaL_checknumber( L, 4 );
	uint16_t color = getColor( L, 5 );

	if (lua_gettop(L) > 5) drawStar(cx, cy, r, fillcolor, 1, fact);
	if (fillcolor != color) drawStar(cx, cy, r, color, 0, fact);

	return 0;
}

//=====================================
static int tft_triangle( lua_State* L )
{
	_check(L);
	if (checkParam(7, L)) return 0;

	uint16_t fillcolor = _bg;
	if (lua_gettop(L) > 7) fillcolor = getColor( L, 8 );
	uint16_t x0 = luaL_checkinteger( L, 1 );
	uint16_t y0 = luaL_checkinteger( L, 2 );
	uint16_t x1 = luaL_checkinteger( L, 3 );
	uint16_t y1 = luaL_checkinteger( L, 4 );
	uint16_t x2 = luaL_checkinteger( L, 5 );
	uint16_t y2 = luaL_checkinteger( L, 6 );
	uint16_t color = getColor( L, 7 );
	if (lua_gettop(L) > 7) TFT_fillTriangle(x0,y0,x1,y1,x2,y2,fillcolor);
	if (fillcolor != color) TFT_drawTriangle(x0,y0,x1,y1,x2,y2,color);

	return 0;
}

//=====================================
static int tft_writepos( lua_State* L )
{
  _check(L);
  if (checkParam(3, L)) return 0;

  const char* buf;
  size_t len;
  int w, h;

  if (cfont.bitmap == 0) goto errexit;

  int x = luaL_checkinteger( L, 1 );
  // for rotated string x cannot be RIGHT or CENTER
  if ((rotation != 0) && (x < -2)) goto errexit;

  int y = luaL_checkinteger( L, 2 );
  if ((x != LASTX) || (y != LASTY)) TFT_OFFSET = 0;
  if (x == LASTX) x = TFT_X;
  if (y == LASTY) y = TFT_Y;

  luaL_checktype( L, 3, LUA_TSTRING );
  buf = lua_tolstring( L, 3, &len );

  w = getStringWidth((char*)buf);
  h = cfont.y_size; // font height
  if ((cfont.x_size != 0) && (cfont.bitmap == 2)) {
    // 7-segment font
    h = (3 * (2 * cfont.y_size + 1)) + (2 * cfont.x_size);  // character height
  }

  if (x==RIGHT) x = dispWin.x2 - w - 1;
  if (x==CENTER) x = (dispWin.x2 - w - 1)/2;
  if (y==BOTTOM) y = dispWin.y2 - h - 1;
  if (y==CENTER) y = (dispWin.y2 - (h/2) - 1)/2;
  if (x < dispWin.x1) x = dispWin.x1;
  if (y < dispWin.y1) y = dispWin.y1;

  if ((y + h - 1) > dispWin.y2) goto errexit;

  lua_pushinteger(L, x);
  lua_pushinteger(L, y);
  lua_pushinteger(L, w);
  lua_pushinteger(L, h);

  return 4;

errexit:
	lua_pushnil(L);
	return 1;
}

//tft.write(x,y,string|intnum|{floatnum,dec},...)
//==================================
static int tft_write( lua_State* L )
{
  _check(L);
  if (checkParam(3, L)) return 0;

  const char* buf;
  char tmps[16];
  size_t len;
  uint8_t numdec = 0;
  uint8_t argn = 0;
  float fnum;

  int x = luaL_checkinteger( L, 1 );
  int y = luaL_checkinteger( L, 2 );
  if ((x != LASTX) || (y != LASTY)) TFT_OFFSET = 0;
  if (x == LASTX) x = TFT_X;
  if (y == LASTY) y = TFT_Y;

  for( argn = 3; argn <= lua_gettop( L ); argn++ )
  {
    if ( lua_type( L, argn ) == LUA_TNUMBER )
    { // write integer number
      len = lua_tointeger( L, argn );
      sprintf(tmps,"%d",len);
      TFT_print(&tmps[0], x, y);
      x = TFT_X;
      y = TFT_Y;
    }
    else if ( lua_type( L, argn ) == LUA_TTABLE ) {
      if (lua_rawlen( L, argn ) == 2) {
        lua_rawgeti( L, argn, 1 );
        fnum = luaL_checknumber( L, -1 );
        lua_pop( L, 1 );
        lua_rawgeti( L, argn, 2 );
        numdec = ( int )luaL_checkinteger( L, -1 );
        lua_pop( L, 1 );
        sprintf(tmps,"%.*f",numdec, fnum);
        TFT_print(&tmps[0], x, y);
        x = TFT_X;
        y = TFT_Y;
      }
    }
    else if ( lua_type( L, argn ) == LUA_TSTRING )
    { // write string
      luaL_checktype( L, argn, LUA_TSTRING );
      buf = lua_tolstring( L, argn, &len );
      TFT_print((char*)buf, x, y);
      x = TFT_X;
      y = TFT_Y;
    }
  }

  return 0;
}

//--------------------------------------
static int tft_set_speed(lua_State *L) {
	int speed, cspeed;

	spi_ll_get_speed(disp_spi, (uint32_t *)&cspeed);
	cspeed = cspeed / 1000;

	if (lua_gettop(L) > 0) {
		speed = luaL_checkinteger(L, 1);

		if (speed < 1000) speed = 1000;
		else if (speed > 40000) speed = 40000;

		if (speed != cspeed) {
			spi_ll_set_speed(disp_spi, speed * 1000);

			speed = speed / 1000;
		}
	} else {
		speed = cspeed;
	}

	lua_pushinteger(L, speed);

	return 1;
}

//--------------------------------------------
static int tft_set_angleOffset(lua_State *L) {

	if (lua_gettop(L) > 0) {
		float angle = luaL_checknumber(L, 1);
		if (angle < -360.0) angle = -360.0;
		if (angle > 360.0) angle = 360.0;
		_angleOffset = angle;
	}
	lua_pushnumber(L, _angleOffset);

	return 1;
}

// ============= Touch panel functions =========================================

//-----------------------------------------------
static int tp_get_data(uint8_t type, int samples)
{
	int n, result, val = 0;
	uint32_t i = 0;
	uint32_t vbuf[18];
	uint32_t minval, maxval, dif;

    if (samples < 3) samples = 1;
    if (samples > 18) samples = 18;

    // one dummy read
    result = touch_get_data(type);

    // read data
	while (i < 10) {
    	minval = 5000;
    	maxval = 0;
		// get values
		for (n=0;n<samples;n++) {
		    result = touch_get_data(type);
			if (result < 0) break;

			vbuf[n] = result;
			if (result < minval) minval = result;
			if (result > maxval) maxval = result;
		}
		if (result < 0) break;
		dif = maxval - minval;
		if (dif < 40) break;
		i++;
    }
	if (result < 0) return -1;

	if (samples > 2) {
		// remove one min value
		for (n = 0; n < samples; n++) {
			if (vbuf[n] == minval) {
				vbuf[n] = 5000;
				break;
			}
		}
		// remove one max value
		for (n = 0; n < samples; n++) {
			if (vbuf[n] == maxval) {
				vbuf[n] = 5000;
				break;
			}
		}
		for (n = 0; n < samples; n++) {
			if (vbuf[n] < 5000) val += vbuf[n];
		}
		val /= (samples-2);
	}
	else val = vbuf[0];

    return val;
}

//====================================
static int tft_get_touch(lua_State *L)
{
	int result = -1;
    int X=0, Y=0, Z=0;

	if (TFT_type == 999) {
		// touch available only on ILI9341
		lua_pushinteger(L, -1);
		lua_pushinteger(L, X);
		lua_pushinteger(L, Y);
		return 3;
	}

    result = tp_get_data(0xB0, 3);
	if (result > 50)  {
		// tp pressed
		Z = result;

		result = tp_get_data(0xD0, 10);
		if (result >= 0) {
			X = result;

			result = tp_get_data(0x90, 10);
			if (result >= 0) Y = result;
		}
	}

	if (result >= 0) lua_pushinteger(L, Z);
	else lua_pushinteger(L, result);
	lua_pushinteger(L, X);
	lua_pushinteger(L, Y);

	return 3;
}

//=====================================
static int tft_read_touch(lua_State *L)
{
	int result = -1;
    int32_t X=0, Y=0, tmp;

	if (TFT_type != 1) {
		// touch available only on ILI9341
		lua_pushinteger(L, -1);
		lua_pushinteger(L, X);
		lua_pushinteger(L, Y);
		return 3;
	}

    result = tp_get_data(0xB0, 3);
	if (result > 50)  {
		// tp pressed
		result = tp_get_data(0xD0, 10);
		if (result >= 0) {
			X = result;

			result = tp_get_data(0x90, 10);
			if (result >= 0) Y = result;
		}
	}

	if (result <= 50) {
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		lua_pushinteger(L, 0);
		return 3;
	}

	int xleft   = (tp_calx >> 16) & 0x3FFF;
	int xright  = tp_calx & 0x3FFF;
	int ytop    = (tp_caly >> 16) & 0x3FFF;
	int ybottom = tp_caly & 0x3FFF;

	if (((xright - xleft) != 0) && ((ybottom - ytop) != 0)) {
		X = ((X - xleft) * 320) / (xright - xleft);
		Y = ((Y - ytop) * 240) / (ybottom - ytop);
	}
	else {
		lua_pushinteger(L, 0);
		lua_pushinteger(L, X);
		lua_pushinteger(L, Y);
		return 3;
	}

	if (X < 0) X = 0;
	if (X > 319) X = 319;
	if (Y < 0) Y = 0;
	if (Y > 239) Y = 239;

	switch (orientation) {
		case PORTRAIT:
			tmp = X;
			X = 240 - Y - 1;
			Y = tmp;
			break;
		case PORTRAIT_FLIP:
			tmp = X;
			X = Y;
			Y = 320 - tmp - 1;
			break;
		case LANDSCAPE_FLIP:
			X = 320 - X - 1;
			Y = 240 - Y - 1;
			break;
	}

	lua_pushinteger(L, result);
	lua_pushinteger(L, X);
	lua_pushinteger(L, Y);
	return 3;
}

//==================================
static int tft_set_cal(lua_State *L)
{
    tp_calx = luaL_checkinteger(L, 1);
    tp_caly = luaL_checkinteger(L, 2);
    return 0;
}

// =============================================================================

#include "modules.h"

static const LUA_REG_TYPE tft_map[] = {
    { LSTRKEY( "init" ),			LFUNCVAL( ltft_init ) },
	{ LSTRKEY( "clear" ),			LFUNCVAL( tft_clear )},
	{ LSTRKEY( "on" ),				LFUNCVAL( tft_on )},
	{ LSTRKEY( "off" ),				LFUNCVAL( tft_off )},
	{ LSTRKEY( "setfont" ),			LFUNCVAL( tft_setfont )},
	{ LSTRKEY( "compilefont" ),		LFUNCVAL( compile_font_file )},
	{ LSTRKEY( "getscreensize" ),	LFUNCVAL( tft_getscreensize )},
	{ LSTRKEY( "getfontsize" ),		LFUNCVAL( tft_getfontsize )},
	{ LSTRKEY( "getfontheight" ),	LFUNCVAL( tft_getfontheight )},
	{ LSTRKEY( "gettype" ),			LFUNCVAL( tft_gettype )},
	{ LSTRKEY( "setrot" ),			LFUNCVAL( tft_setrot )},
	{ LSTRKEY( "setorient" ),		LFUNCVAL( tft_setorient )},
	{ LSTRKEY( "setcolor" ),		LFUNCVAL( tft_setcolor )},
	{ LSTRKEY( "settransp" ),		LFUNCVAL( tft_settransp )},
	{ LSTRKEY( "setfixed" ),		LFUNCVAL( tft_setfixed )},
	{ LSTRKEY( "setwrap" ),			LFUNCVAL( tft_setwrap )},
	{ LSTRKEY( "setangleoffset" ),	LFUNCVAL( tft_set_angleOffset )},
	{ LSTRKEY( "setclipwin" ),		LFUNCVAL( tft_setclipwin )},
	{ LSTRKEY( "resetclipwin" ),	LFUNCVAL( tft_resetclipwin )},
	{ LSTRKEY( "invert" ),			LFUNCVAL( tft_invert )},
	{ LSTRKEY( "putpixel" ),		LFUNCVAL( tft_putpixel )},
	{ LSTRKEY( "getpixel" ),		LFUNCVAL( tft_getpixel )},
	{ LSTRKEY( "getline" ),			LFUNCVAL( tft_getwindow )},
	{ LSTRKEY( "line" ),			LFUNCVAL( tft_drawline )},
	{ LSTRKEY( "linebyangle" ),		LFUNCVAL( tft_drawlineByAngle )},
	{ LSTRKEY( "rect" ),			LFUNCVAL( tft_rect )},
	{ LSTRKEY( "roundrect" ),		LFUNCVAL( tft_roundrect )},
	{ LSTRKEY( "circle" ),			LFUNCVAL( tft_circle )},
	{ LSTRKEY( "ellipse" ),			LFUNCVAL( tft_ellipse )},
	{ LSTRKEY( "arc" ),				LFUNCVAL( tft_arc )},
	{ LSTRKEY( "poly" ),			LFUNCVAL( tft_poly )},
	{ LSTRKEY( "star" ),			LFUNCVAL( tft_star )},
	{ LSTRKEY( "triangle" ),		LFUNCVAL( tft_triangle )},
	{ LSTRKEY( "write" ),			LFUNCVAL( tft_write )},
	{ LSTRKEY( "stringpos" ),		LFUNCVAL( tft_writepos )},
	{ LSTRKEY( "image" ),			LFUNCVAL( tft_image )},
	{ LSTRKEY( "jpgimage" ),		LFUNCVAL( ltft_jpg_image )},
	{ LSTRKEY( "bmpimage" ),		LFUNCVAL( tft_bmpimage )},
	{ LSTRKEY( "hsb2rgb" ),			LFUNCVAL( tft_HSBtoRGB )},
	{ LSTRKEY( "setbrightness" ),	LFUNCVAL( tft_set_brightness )},
	{ LSTRKEY( "gettouch" ),		LFUNCVAL( tft_read_touch )},
	{ LSTRKEY( "getrawtouch" ),		LFUNCVAL( tft_get_touch )},
	{ LSTRKEY( "setcal" ),			LFUNCVAL( tft_set_cal )},
	{ LSTRKEY( "setspeed" ),		LFUNCVAL( tft_set_speed )},

	// Constant definitions
	{ LSTRKEY( "PORTRAIT" ),       LINTVAL( PORTRAIT ) },
	{ LSTRKEY( "PORTRAIT_FLIP" ),  LINTVAL( PORTRAIT_FLIP ) },
	{ LSTRKEY( "LANDSCAPE" ),      LINTVAL( LANDSCAPE ) },
	{ LSTRKEY( "LANDSCAPE_FLIP" ), LINTVAL( LANDSCAPE_FLIP ) },
	{ LSTRKEY( "CENTER" ),         LINTVAL( CENTER ) },
	{ LSTRKEY( "RIGHT" ),          LINTVAL( RIGHT ) },
	{ LSTRKEY( "BOTTOM" ),         LINTVAL( BOTTOM ) },
	{ LSTRKEY( "LASTX" ),          LINTVAL( LASTX ) },
	{ LSTRKEY( "LASTY" ),          LINTVAL( LASTY ) },
	{ LSTRKEY( "BLACK" ),          LINTVAL( TFT_BLACK ) },
	{ LSTRKEY( "NAVY" ),           LINTVAL( TFT_NAVY ) },
	{ LSTRKEY( "DARKGREEN" ),      LINTVAL( TFT_DARKGREEN ) },
	{ LSTRKEY( "DARKCYAN" ),       LINTVAL( TFT_DARKCYAN ) },
	{ LSTRKEY( "MAROON" ),         LINTVAL( TFT_MAROON ) },
	{ LSTRKEY( "PURPLE" ),         LINTVAL( TFT_PURPLE ) },
	{ LSTRKEY( "OLIVE" ),          LINTVAL( TFT_OLIVE ) },
	{ LSTRKEY( "LIGHTGREY" ),      LINTVAL( TFT_LIGHTGREY ) },
	{ LSTRKEY( "DARKGREY" ),       LINTVAL( TFT_DARKGREY ) },
	{ LSTRKEY( "BLUE" ),           LINTVAL( TFT_BLUE ) },
	{ LSTRKEY( "GREEN" ),          LINTVAL( TFT_GREEN ) },
	{ LSTRKEY( "CYAN" ),           LINTVAL( TFT_CYAN ) },
	{ LSTRKEY( "RED" ),            LINTVAL( TFT_RED ) },
	{ LSTRKEY( "MAGENTA" ),        LINTVAL( TFT_MAGENTA ) },
	{ LSTRKEY( "YELLOW" ),         LINTVAL( TFT_YELLOW ) },
	{ LSTRKEY( "WHITE" ),          LINTVAL( TFT_WHITE ) },
	{ LSTRKEY( "ORANGE" ),         LINTVAL( TFT_ORANGE ) },
	{ LSTRKEY( "GREENYELLOW" ),    LINTVAL( TFT_GREENYELLOW ) },
	{ LSTRKEY( "PINK" ),           LINTVAL( TFT_PINK ) },
	{ LSTRKEY( "FONT_DEFAULT" ),   LINTVAL( DEFAULT_FONT ) },
	{ LSTRKEY( "FONT_DEJAVU18" ),  LINTVAL( DEJAVU18_FONT ) },
	{ LSTRKEY( "FONT_DEJAVU24" ),  LINTVAL( DEJAVU24_FONT ) },
	{ LSTRKEY( "FONT_UBUNTU16" ),  LINTVAL( UBUNTU16_FONT ) },
	{ LSTRKEY( "FONT_COMIC24" ),   LINTVAL( COMIC24_FONT ) },
	{ LSTRKEY( "FONT_TOONEY32" ),  LINTVAL( TOONEY32_FONT ) },
	{ LSTRKEY( "FONT_MINYA24" ),   LINTVAL( MINYA24_FONT ) },
	{ LSTRKEY( "FONT_7SEG" ),      LINTVAL( FONT_7SEG ) },
	{ LSTRKEY( "ST7735" ),         LINTVAL( 0 ) },
	{ LSTRKEY( "ST7735B" ),        LINTVAL( 1 ) },
	{ LSTRKEY( "ST7735G" ),        LINTVAL( 2 ) },
	{ LSTRKEY( "ILI9341" ),        LINTVAL( 3 ) },

	{ LNILKEY, LNILVAL }
};

int luaopen_tft(lua_State* L) {
    return 0;
}

MODULE_REGISTER_MAPPED(TFT, tft, tft_map, luaopen_tft);

#endif
