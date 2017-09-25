
	
void ssd1306_clear();
void ssd1306_show();
void ssd1306_invert(bool b);
void ssd1306_rotation(int r);
void ssd1306_dim(bool is_dim);
void ssd1306_startscroll(int from, int to, int dir);
void ssd1306_stopscroll();

void ssd1306_pixel(int x, int y, int c);
void ssd1306_line(int xb, int yb, int xe, int ye, int c);
void ssd1306_rect(int xb, int yb, int xe, int ye, int c);
void ssd1306_rectFill(int xb, int yb, int xe, int ye, int c);
void ssd1306_roundRect(int xb, int yb, int xe, int ye, int r, int c);
void ssd1306_roundRectFill(int xb, int yb, int xe, int ye, int r, int c);
void ssd1306_circle(int x, int y, int r, int c);
void ssd1306_circleFill(int x, int y, int r, int c);
void ssd1306_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int c);
void ssd1306_triangleFill(int x1, int y1, int x2, int y2, int x3, int y3, int c);

void ssd1306_txtSize(float sz);
void ssd1306_txtColor(int clr, int bg);
void ssd1306_txtCursor(int x, int y);
void ssd1306_txtFont(int n, bool b, bool i, int sz);
void ssd1306_txtGetBnd(char* pstr, int x, int y, int* px1, int* py1, int* pw, int* ph);
void ssd1306_print(char* s);
void ssd1306_println(char* s);
void ssd1306_chr(uint8_t chr);
void ssd1306_chrPos(int x, int y, uint8_t chr, int clr, int bg, int size);

void ssd1306_bitmap(int x, int y, int w, int h, const unsigned char* bmp, int clr);

void ssd1306_init();

