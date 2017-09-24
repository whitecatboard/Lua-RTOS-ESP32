/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

//#define SSD1306_128_64 1

#include <Adafruit_SSD1306.h>

#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBoldOblique9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerifItalic12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMonoOblique12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <Fonts/FreeSerifItalic18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMonoOblique18pt7b.h>
#include <Fonts/FreeSansBoldOblique12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifItalic24pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoOblique24pt7b.h>
#include <Fonts/FreeSansBoldOblique18pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoOblique9pt7b.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/Org_01.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/Picopixel.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSansOblique12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/TomThumb.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSansOblique18pt7b.h>
#include <Fonts/FreeSerifBoldItalic12pt7b.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansOblique24pt7b.h>
#include <Fonts/FreeSerifBoldItalic18pt7b.h>
#include <Fonts/FreeMonoBoldOblique18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>
#include <Fonts/FreeSerifBoldItalic24pt7b.h>
#include <Fonts/FreeMonoBoldOblique24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifBoldItalic9pt7b.h>



const GFXfont* fonts_9[] PROGMEM = {
	&FreeMono9pt7b,
	&FreeMonoBold9pt7b,
	&FreeMonoOblique9pt7b,
	&FreeMonoBoldOblique9pt7b,
	
	&FreeSans9pt7b,
	&FreeSansBold9pt7b,
	&FreeSansOblique9pt7b,
	&FreeSansBoldOblique9pt7b,
	
	&FreeSerif9pt7b,
	&FreeSerifBold9pt7b,
	&FreeSerifItalic9pt7b,
	&FreeSerifBoldItalic9pt7b
};
const GFXfont* fonts_12[] PROGMEM = {
	&FreeMono12pt7b,
	&FreeMonoBold12pt7b,
	&FreeMonoOblique12pt7b,
	&FreeMonoBoldOblique12pt7b,
	
	&FreeSans12pt7b,
	&FreeSansBold12pt7b,
	&FreeSansOblique12pt7b,
	&FreeSansBoldOblique12pt7b,
	
	&FreeSerif12pt7b,
	&FreeSerifBold12pt7b,
	&FreeSerifItalic12pt7b,
	&FreeSerifBoldItalic12pt7b
};
const GFXfont* fonts_18[] PROGMEM = {
	&FreeMono18pt7b,
	&FreeMonoBold18pt7b,
	&FreeMonoOblique18pt7b,
	&FreeMonoBoldOblique18pt7b,
	
	&FreeSans18pt7b,
	&FreeSansBold18pt7b,
	&FreeSansOblique18pt7b,
	&FreeSansBoldOblique18pt7b,
	
	&FreeSerif18pt7b,
	&FreeSerifBold18pt7b,
	&FreeSerifItalic18pt7b,
	&FreeSerifBoldItalic18pt7b
};
const GFXfont* fonts_24[] PROGMEM = {
	&FreeMono24pt7b,
	&FreeMonoBold24pt7b,
	&FreeMonoOblique24pt7b,
	&FreeMonoBoldOblique24pt7b,
	
	&FreeSans24pt7b,
	&FreeSansBold24pt7b,
	&FreeSansOblique24pt7b,
	&FreeSansBoldOblique24pt7b,
	
	&FreeSerif24pt7b,
	&FreeSerifBold24pt7b,
	&FreeSerifItalic24pt7b,
	&FreeSerifBoldItalic24pt7b
};

const GFXfont* fonts_s[] PROGMEM = {
	&Org_01,
	&Picopixel,
	&TomThumb
	};
	
#define MAX_FONTS 3


#define OLED_RESET 2
Adafruit_SSD1306 display(OLED_RESET);

/*
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };
*/

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

extern "C" {
	
#include "ssd1306.h"


void testscrolltext(void) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10,0);
  display.clearDisplay();
  display.println("scroll");
  display.display();
  delay(1);
 
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);    
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
}



void ssd1306_clear(){ display.clearDisplay(); }
void ssd1306_show(){ display.display(); }
void ssd1306_invert(bool b){ display.invertDisplay(b); }
void ssd1306_rotation(int r){ display.setRotation(r); }

void ssd1306_pixel(int x, int y, int c){ display.drawPixel(x, y, c ? WHITE : BLACK); }
void ssd1306_line(int xb, int yb, int xe, int ye, int c){ 
	display.drawLine(xb, yb, xe, ye, c==0 ? BLACK : WHITE); 
}
void ssd1306_rect(int xb, int yb, int xe, int ye, int c){ 
	display.drawRect(xb, yb, xe, ye, c==0 ? BLACK : WHITE); 
}
void ssd1306_rectFill(int xb, int yb, int xe, int ye, int c){ 
	display.fillRect(xb, yb, xe, ye, c==0 ? BLACK : WHITE); 
}
void ssd1306_roundRect(int xb, int yb, int xe, int ye, int r, int c){ 
	display.drawRoundRect(xb, yb, xe, ye, r, c==0 ? BLACK : WHITE); 
}
void ssd1306_roundRectFill(int xb, int yb, int xe, int ye, int r, int c){ 
	display.fillRoundRect(xb, yb, xe, ye, r, c==0 ? BLACK : WHITE); 
}
void ssd1306_circle(int x, int y, int r, int c){ 
	display.drawCircle(x, y, r, c==0 ? BLACK : WHITE); 
}
void ssd1306_circleFill(int x, int y, int r, int c){ 
	display.fillCircle(x, y, r, c==0 ? BLACK : WHITE); 
}
void ssd1306_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int c){ 
	display.drawTriangle(x1, y1, x2, y2, x3, y3, c==0 ? BLACK : WHITE); 
}
void ssd1306_triangleFill(int x1, int y1, int x2, int y2, int x3, int y3, int c){ 
	display.fillTriangle(x1, y1, x2, y2, x3, y3, c==0 ? BLACK : WHITE); 
}

void ssd1306_txtSize(float sz){ display.setTextSize(sz); }
void ssd1306_txtColor(int clr, int bg){ 
	display.setTextColor(clr ? WHITE : BLACK, bg ? WHITE : BLACK); 
}
void ssd1306_txtFont(int n, bool b, bool i, int sz){ 
	if(n < 0 || n >= MAX_FONTS) n = 0;
	int tn = n*4 + (b ? 1 : 0) + (i ? 2 : 0);
	
	const GFXfont** tfonts;
	if(sz <= 0) tfonts = fonts_s;
	else if(sz < 11) tfonts = fonts_9;
	else if(sz < 17) tfonts = fonts_12;
	else if(sz < 23) tfonts = fonts_18;
	else /*if(sz < 29)*/ tfonts = fonts_24;
	//else tfonts = fonts_9;
	display.setFont( tfonts[ (sz <= 0 ? n : tn) ] ); 
}
void ssd1306_txtCursor(int x, int y){ display.setCursor(x,y); }
void ssd1306_txtGetBnd(char* pstr, int x, int y, int* px1, int* py1, int* pw, int* ph){ 
	display.getTextBounds(
					pstr, 
					x,y, 
					(int16_t*)px1,(int16_t*)py1, 
					(uint16_t*)pw,(uint16_t*)ph
					); 
}

void ssd1306_print(char* s){ display.print(s); }
void ssd1306_println(char* s){ display.println(s); }

void ssd1306_chr(uint8_t chr){ display.write(chr); }
void ssd1306_chrPos(int x, int y, uint8_t chr, int clr, int bg, int size){ 
	display.drawChar(x, y, chr, clr, bg, size); 
}

void ssd1306_bitmap(int x, int y, int w, int h, const unsigned char* bmp, int clr){ 
	display.drawBitmap(x, y, bmp, w, h, clr); 
}


void ssd1306_init()   {
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
//  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
//  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
  
  display.setTextColor(WHITE);
  display.setTextSize(1);
}

} //extern "C"
