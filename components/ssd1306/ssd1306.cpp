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

#define OLED_RESET 2
Adafruit_SSD1306 display(OLED_RESET);


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
void ssd1306_txtColor(int clr){ display.setTextColor(clr ? WHITE : BLACK); }
void ssd1306_txtCursor(int x, int y){ display.setCursor(x,y); }

void ssd1306_print(char* s){ display.print(s); }
void ssd1306_println(char* s){ display.println(s); }

void ssd1306_chr(uint8_t chr){ display.write(chr); }

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
