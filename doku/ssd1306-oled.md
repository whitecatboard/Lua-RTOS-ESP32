
#Main SSD1306 graphic interface
main Lua object is **ssd1306**. all graphic interfaces are members of **ssd1306**. examples:
```lua
ssd1306.cls()
ssd1306.line(0,0, 127,63)
ssd1306.textPos(10,22)
ssd1306.print("Hello World! ", 123.4, nil)
```

#Members of ssd1306 object

#general ops

##cls()
clear screen

##show()
show internal buffer with graphic data on the ssd1306 oled screen

##autoshow(is_auto)
select between 

* show after each graphical operation
* or show manually after a couple of operations

##invert(is_invert)
invert of all picture on the screen or not

* is_invert -- *true* - invert, *false* - not invert

##rotation(rot)
set rotation of graphical operation on the screen

* rot -- [0..3] - rotation. 0, 90, 180, 270 degrees

#graphical ops

##point(x, y, clr)
point
* x,y -- coords of point
* clr -- color of point. 1 - white, 0 - black

##line(x0,y0, x1,y1, clr)
line
* x0,y0 -- begin of the line
* x1,y1 -- end of the line
* clr -- color. 1 or 0

##rect(x0,y0, x1,y1, clr)
regular rectangle
* x0,y0 -- begin of the rect
* x1,y1 -- end of the rect
* clr -- color. 1 or 0

##rectFill(x0,y0, x1,y1, clr)
filled rectangle
* x0,y0 -- begin of the rect
* x1,y1 -- end of the rect
* clr -- color. 1 or 0

##rectRound(x0,y0, x1,y1, r, clr)
rectangle with rounded corners
* x0,y0 -- begin of the rect
* x1,y1 -- end of the rect
* r -- radius of roundings of corners
* clr -- color. 1 or 0

##rectRoundFill(x0,y0, x1,y1, r, clr)
filled rectangle with rounded corners
* x0,y0 -- begin of the rect
* x1,y1 -- end of the rect
* r -- radius of roundings of corners
* clr -- color. 1 or 0

##circle(x,y, r, clr)
circle
* x,y -- center of the circle
* r -- radius of the circle
* clr -- color. 1 or 0

##circleFill(x,y, r, clr)
filled circle
* x,y -- center of the circle
* r -- radius of the circle
* clr -- color. 1 or 0

##triangle(x1,y1, x2,y2, x3,y3, clr)
triangle
* x1,y1 -- 1st corner
* x2,y2 -- 2nd corner
* x3,y3 -- 3rd corner
* clr -- color. 1 or 0

##triangleFill(x1,y1, x2,y2, x3,y3, clr)
filled triangle
* x1,y1 -- 1st corner
* x2,y2 -- 2nd corner
* x3,y3 -- 3rd corner
* clr -- color. 1 or 0

#bitmap ops

##bitmap(x,y, w,h, {..point_colors..}, clr)
bitmap
* x,y -- left-top corner of the picture
* w,h -- width-height of the picture
* point_colors -- a table with integers. each integer contain 8bits (a byte) - colors of 8 points in the line. next integer - next 8 points and so on. 
* clr -- color. 1 or 0
```lua
ssd1306.bitmap(20,5, 8,4, {8, 20, 34, 20}, 1)
```

#text ops

##textColor(clr, bgnd)
set color for text ops
* clr -- color of text. 1 or 0
* bgnd -- color of background. 1 or 0

##textSize(scale)
set text size scale
scale -- scale of text. 1, 2 or 3

##textFont(font, is_bold, is_italic, size)
set font for text ops
* font -- 0, 1, 2 - 3 types of fonts. if size > 0 fonts are regular, if size == 0 fots are special 
* is_bold -- 1 - bold font, 0 - regular font
* is_italic -- 1 - italic font, 0 - regular font
* size -- 9, 12, 18, 24 - sizes of general fonts, 0 - for special fonts

##textPos(x,y)
set starting pos for text ops
* x,y -- text starting position

##x0,y0, w,h = textGetBounds(str, x,y)
get the bounding rectangle of the sent text using current font
* str -- text to calculate its bounds
* x,y -- starting pos for calculation
* x0,y0 -- calculated left-top corner
* w,h -- calculated width and height

##print(out)
print data
* out - dta for print. may be string, number or other (nil for example). or ',' separated list of different data.

##chr(char_code)
print caracter with char_code into current text position

##chr(x,y, char_code, clr, bgnd, scale)
more complex single character output
* x,y -- pos to output
* char_code -- code of character to output
* clr -- color of character
* bgnd -- color of background
* scale -- 1,2,3 - scale of character
