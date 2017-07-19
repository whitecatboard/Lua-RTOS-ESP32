--[[
Demo program demonstrating the capabities of the tft module
Author:	LoBo (https://github/loboris)
Date:	07/02/2017
To load the program execute 'dofile("tftdemo.lua")' or 'require("tftdemo")'
Usage EXAMPLES:
tftdemo.circleDemo(6,1)
tftdemo.fullDemo(6, 1)
=== Also can be run in thread: ===
demo_th = thread.start(tftdemo.thfullDemo)
Touching the screen will exit the program. (only for ILI9341, if touch panel present)
To disable touch checking set 'gdisplay.demo.touch=false' (may run faster)
--]]


-- create fontdemo table and set some defaults
tftdemo = {
	dispType = gdisplay.ILI9341, -- or gdisplay.ST7735B, gdisplay.ST7735, gdisplay.ST7735G
	maxx = 240,
	maxy = 320,
	miny = 12,
	touch = false,
	-- fonts used in this demo
	fontnames = {
		gdisplay.FONT_DEFAULT,
		gdisplay.FONT_7SEG,
		gdisplay.FONT_UBUNTU16,
		gdisplay.FONT_COMIC24,
		gdisplay.FONT_TOONEY32,
		gdisplay.FONT_MINYA24
	},
	-- images used in this demo {file_name, width, height}
	-- width & height are needed only for raw images
	images = {
		jpg = {"tiger240.jpg", 0, 0},
		bmp = {"tiger.bmp", 0, 0}
	}
}

-- Init the display
function tftdemo.init()
	if gdisplay.gettype() < 0 then
		gdisplay.init(tftdemo.dispType, gdisplay.LANDSCAPE_FLIP)
		if gdisplay.gettype() < 0 then
			print("LCD not initialized")
			return false
		end
		if gdisplay.gettype() ~= gdisplay.ILI9341 then
			tftdemo.touch = false
		end
	end
	return true
end

-- Check if the display is touched
function tftdemo.touched()
	if tftdemo.touch == false then
		return false
	else
		local tch
		tch,_,_ = gdisplay.gettouch()
		if tch <= 0 then
			return false
		else
			return true
		end
	end
end

-- print display header
function tftdemo.header(tx, setclip)
	math.randomseed(os.time())
	-- adjust screen dimensions (depends on used display and orientation
	tftdemo.maxx, tftdemo.maxy = gdisplay.getscreensize()
	gdisplay.clear()
	gdisplay.setcolor(gdisplay.CYAN)
	if tftdemo.maxx < 240 then
		gdisplay.setfont("/@font/SmallFont.fon")
	else
		gdisplay.setfont(gdisplay.FONT_DEFAULT)
	end
	tftdemo.miny = gdisplay.getfontheight() + 5
	gdisplay.rect(0,0,tftdemo.maxx-1,tftdemo.miny-1,gdisplay.OLIVE,{8,16,8})
	gdisplay.settransp(1)
	gdisplay.write(gdisplay.CENTER,2,tx)
	gdisplay.settransp(0)

	if setclip then
		gdisplay.setclipwin(0, tftdemo.miny, tftdemo.maxx, tftdemo.maxy)
	end
end

-- Display some fonts
function tftdemo.dispFont(sec)
	tftdemo.header("DISPLAY FONTS", false)

	local tx
	if tftdemo.maxx < 240 then
		tx = "ESP-Lua"
	else
		tx = "Hi from LoBo"
	end
	local starty = tftdemo.miny + 4

	local x,y
	local n = os.clock() + sec
	while os.clock() < n do
		y = starty
		x = 0
		local i,j
		i = 0
		while y < tftdemo.maxy do
			if i == 0 then 
				x = 0
			elseif i == 1 then 
				x = gdisplay.CENTER
			elseif i == 2 then
				x = gdisplay.RIGHT
			end
			i = i + 1
			if i > 2 then
				i = 0
			end
			
			for j=1, #tftdemo.fontnames, 1 do
				gdisplay.setcolor(math.random(0xFFFF))
				if tftdemo.fontnames[j] == gdisplay.FONT_7SEG then
					gdisplay.setfont(tftdemo.fontnames[j], 8, 1)
					gdisplay.write(x,y,"-12.45/")
				else
					gdisplay.setfont(tftdemo.fontnames[j])
					gdisplay.write(x,y,tx)
				end
				y = y + 2 + gdisplay.getfontheight()
				if y > (tftdemo.maxy-gdisplay.getfontheight()) then
					y = tftdemo.maxy
				end
			end
		end
		if tftdemo.touched() then
			break
		end
	end
end

-- Display random fonts
function tftdemo.fontDemo(sec, rot)
	local tx = "FONTS"
	if rot > 0 then
		tx = "ROTATED "..tx
	end
	tftdemo.header(tx, true)

	tx = "ESP32-Lua"
	local x, y, color, i, l, w
	local n = os.clock() + sec
	while os.clock() < n do
		if rot == 1 then
			gdisplay.setrot(math.floor(math.random(359)/5)*5);
		end
		for i=1, #tftdemo.fontnames, 1 do
			if (rot == 0) or (i ~= 1) then
				gdisplay.setcolor(math.random(0xFFFF))
				x = math.random(tftdemo.maxx-8)
				if tftdemo.fontnames[j] == gdisplay.FONT_7SEG then
					gdisplay.setfont(tftdemo.fontnames[i])
					y = math.random(tftdemo.miny, tftdemo.maxy-gdisplay.getfontheight())
					gdisplay.write(x,y,tx)
				else
					l = math.random(6,20)
					w = math.random(1,l // 3)
					gdisplay.setfont(tftdemo.fontnames[i], l, w)
					y = math.random(tftdemo.miny, tftdemo.maxy-gdisplay.getfontheight())
					gdisplay.write(x,y,"-12.45/")
				end
			end
		end
		if tftdemo.touched() then break end
	end
	gdisplay.resetclipwin()
	gdisplay.setrot(0)
end

-- Display random lines
function tftdemo.lineDemo(sec)
	tftdemo.header("LINE DEMO", true)

	local x1, x2, y1, y2, color
	local n = os.clock() + sec
	while os.clock() < n do
		x1 = math.random(tftdemo.maxx-4)
		y1 = math.random(tftdemo.miny, tftdemo.maxy-4)
		x2 = math.random(tftdemo.maxx-1)
		y2 = math.random(tftdemo.miny, tftdemo.maxy-1)
		color = math.random(0xFFFF)
		gdisplay.line(x1,y1,x2,y2,color)
		if tftdemo.touched() then break end
	end;
	gdisplay.resetclipwin()
end;

-- Display random circles
function tftdemo.circleDemo(sec,dofill)
	local tx = "CIRCLE"
	if dofill > 0 then
		tx = "FILLED "..tx
	end
	tftdemo.header(tx, true)

	local x, y, r, color, fill
	local n = os.clock() + sec
	while os.clock() < n do
		x = math.random(4, tftdemo.maxx-2)
		y = math.random(tftdemo.miny+2, tftdemo.maxy-2)
		if x < y then
			r = math.random(2, x)
		else
			r = math.random(2, y)
		end
		color = math.random(0xFFFF)
		if dofill > 0 then
			fill = math.random(0xFFFF)
			gdisplay.circle(x,y,r,color,fill)
		else
			gdisplay.circle(x,y,r,color)
		end
		if tftdemo.touched() then break end
	end
	gdisplay.resetclipwin()
end;

-- Display random ellipses
function tftdemo.ellipseDemo(sec,dofill)
	local tx = "ELLIPSE"
	if dofill > 0 then
		tx = "FILLED "..tx
	end
	tftdemo.header(tx, true)

	local x, y, rx, ry, color, fill
	local n = os.clock() + sec
	while os.clock() < n do
		x = math.random(4, tftdemo.maxx-2)
		y = math.random(tftdemo.miny+2, tftdemo.maxy-2)
		if x < y then
			rx = math.random(2, x)
		else
			rx = math.random(2, y)
		end
		if x < y then
			ry = math.random(2, x)
		else
			ry = math.random(2, y)
		end
		color = math.random(0xFFFF)
		if dofill > 0 then
			fill = math.random(0xFFFF)
			gdisplay.ellipse(x,y,rx,ry,color,fill)
		else
			gdisplay.ellipse(x,y,rx,ry,color)
		end
		if tftdemo.touched() then break end
	end;
	gdisplay.resetclipwin()
end;

-- Display random rectangles
function tftdemo.rectDemo(sec,dofill)
	local tx = "RECTANGLE"
	if dofill > 0 then
		tx = "FILLED "..tx
	end
	tftdemo.header(tx, true)

	local x, y, w, h, color, fill
	local n = os.clock() + sec
	while os.clock() < n do
		x = math.random(4, tftdemo.maxx-2)
		y = math.random(tftdemo.miny, tftdemo.maxy-2)
		w = math.random(2, tftdemo.maxx-x)
		h = math.random(2, tftdemo.maxy-y)
		color = math.random(0xFFFF)
		if dofill > 0 then
			fill = math.random(0xFFFF)
			gdisplay.rect(x,y,w,h,color,fill)
		else
			gdisplay.rect(x,y,w,h,color)
		end
		if tftdemo.touched() then break end
	end
	gdisplay.resetclipwin()
end

-- Display random rounded rectangles
function tftdemo.roundrectDemo(sec,dofill)
	local tx = "ROUND RECT"
	if dofill > 0 then
		tx = "FILLED "..tx
	end
	tftdemo.header(tx, true)

	local x, y, w, h, r, color, fill
	local n = os.clock() + sec
	while os.clock() < n do
		x = math.random(2, tftdemo.maxx-18)
		y = math.random(tftdemo.miny, tftdemo.maxy-18)
		w = math.random(12, tftdemo.maxx-x)
		h = math.random(12, tftdemo.maxy-y)
		if w > h then
			r = math.random(2, h // 2)
		else
			r = math.random(2, w // 2)
		end
		color = math.random(0xFFFF)
		if dofill > 0 then
			fill = math.random(0xFFFF)
			gdisplay.roundrect(x,y,w,h,r,color,fill)
		else
			gdisplay.roundrect(x,y,w,h,r,color)
		end
		if tftdemo.touched() then break end
	end
	gdisplay.resetclipwin()
end

--[[ Display random poligons
function tftdemo.polyDemo(sec,dofill)
	local tx = "ROUND RECT"
	if dofill > 0 then
		tx = "FILLED "..tx
	end
	tftdemo.header(tx, true)
	local x, y, sid, r, rot, color, fill
	local n = os.clock() + sec
	while os.clock() < n do
		x = math.random(2, tftdemo.maxx-16)
		y = math.random(tftdemo.miny, tftdemo.maxy-16)
		sid = math.random(5, 16)
		rot = math.random(0, 359)
		if x > y then
			r = math.random(8, tftdemo.maxy-y)
		else
			r = math.random(8, tftdemo.maxx-x)
		end
		color = math.random(0xFFFF)
		if dofill > 0 then
			fill = math.random(0xFFFF)
			gdisplay.poly(x,y,sid,r,rot,color,fill)
		else
			gdisplay.poly(x,y,sid,r,rot,color)
		end
		if tftdemo.touched() then break end
	end
	gdisplay.resetclipwin()
end
--]]

-- Display random triangles
function tftdemo.triangleDemo(sec,dofill)
	local tx = "TRIANGLE"
	if dofill > 0 then
		tx = "FILLED "..tx
	end
	tftdemo.header(tx, true)

	local x1, y1, x2, y2, x3, y3, color, fill
	local n = os.clock() + sec
	while os.clock() < n do
		x1 = math.random(4, tftdemo.maxx-2)
		y1 = math.random(tftdemo.miny, tftdemo.maxy-2)
		x2 = math.random(4, tftdemo.maxx-2)
		y2 = math.random(tftdemo.miny,tftdemo.maxy-2)
		x3 = math.random(4, tftdemo.maxx-2)
		y3 = math.random(tftdemo.miny, tftdemo.maxy-2)
		color = math.random(0xFFFF)
		if dofill > 0 then
			fill = math.random(0xFFFF)
			gdisplay.triangle(x1,y1,x2,y2,x3,y3,color,fill)
		else
			gdisplay.triangle(x1,y1,x2,y2,x3,y3,color)
		end
		if tftdemo.touched() then break end
	end;
	gdisplay.resetclipwin()
end;

-- Display random pixels
function tftdemo.pixelDemo(sec)
	tftdemo.header("PUTPIXEL", true)

	local x, y, color
	local n = os.clock() + sec
	while os.clock() < n do
		x = math.random(tftdemo.maxx-1)
		y = math.random(tftdemo.miny, tftdemo.maxy-1)
		color = math.random(0xFFFF)
		gdisplay.putpixel(x,y,color)
		if tftdemo.touched() then break end
	end;
	gdisplay.resetclipwin()
end

-- Display imaged in supported formats
function tftdemo.imageDemo(sec)
	tftdemo.header("JPG IMAGE")
	if os.exists(tftdemo.images["jpg"][1]) then
		gdisplay.image(0,tftdemo.miny+2,tftdemo.images["jpg"][1])
	else
		gdisplay.write(gdisplay.CENTER,gdisplay.CENTER,"Image not found")
	end
	if tftdemo.touched() then return end
	tmr.delay(2)

	tftdemo.header("BMP IMAGE")
	if os.exists(tftdemo.images["bmp"][1]) then
		gdisplay.image(0, tftdemo.miny+2, tftdemo.images["bmp"][1])
	else
		gdisplay.write(gdisplay.CENTER,gdisplay.CENTER,"Image not found")
	end
	if tftdemo.touched() then return end
	tmr.delay(2)
end

-- Display introductory screen
function tftdemo.intro(sec)
	tftdemo.maxx, tftdemo.maxy = gdisplay.getscreensize()
	local inc = 360 / tftdemo.maxy
	local i, x, y, w, h
	-- display rainbow colors
	for i=0,tftdemo.maxy-1,1 do
		gdisplay.line(0,i,tftdemo.maxx-1,i,gdisplay.hsb2rgb(i*inc,1,1))
	end
	-- add some text
	gdisplay.setrot(0);
	gdisplay.setfont("/@font/DotMatrix_M.fon")
	gdisplay.settransp(1)

	x, y, w, h =gdisplay.stringpos(gdisplay.CENTER, gdisplay.CENTER, "ESP32-Lua")
	gdisplay.setcolor(gdisplay.DARKGREY)
	gdisplay.write(gdisplay.CENTER, gdisplay.CENTER,"ESP32-Lua")
	gdisplay.setcolor(gdisplay.BLACK)
	gdisplay.write(x-1, y-1,"ESP32-Lua")
	
	gdisplay.setfont(gdisplay.FONT_COMIC24)
	x, _, _, _ =gdisplay.stringpos(gdisplay.CENTER, gdisplay.CENTER, "TFT Demo")
	gdisplay.setcolor(gdisplay.DARKGREY)
	gdisplay.write(x, y+h+12,"TFT Demo")
	gdisplay.setcolor(gdisplay.BLACK)
	gdisplay.write(x-1, y+h+11,"TFT Demo")

	if tftdemo.touched() then return end
	tmr.delay(sec)
end

function tftdemo.lcdDemo(sec, orient)
	gdisplay.setorient(orient)

	tftdemo.intro(sec)
	if tftdemo.touched() then return end
	tftdemo.dispFont(sec)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.fontDemo(sec,0)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.fontDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.lineDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.circleDemo(sec,0)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.circleDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.ellipseDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.rectDemo(sec,0)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.rectDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.roundrectDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.triangleDemo(sec,0)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.triangleDemo(sec,1)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.pixelDemo(sec, orient)
	if tftdemo.touched() then return end
	tmr.delay(2)
	tftdemo.imageDemo(sec)
	if tftdemo.touched() then return end
	tmr.delay(1)
end

function tftdemo.fullDemo(sec, rpt)
	local orientation = 0
	while rpt > 0 do
		gdisplay.setrot(0);
		gdisplay.setcolor(gdisplay.CYAN)
		gdisplay.setfont(gdisplay.FONT_DEFAULT)

		tftdemo.lcdDemo(sec, orientation)
		if tftdemo.touched() then break end

		orientation = (orientation + 1) & 3
		rpt = rpt - 1
	end
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.setfont(gdisplay.FONT_UBUNTU16)
	gdisplay.write(gdisplay.CENTER,tftdemo.maxy-gdisplay.getfontheight() - 4,"That's all folks!")
	gdisplay.setrot(0);
	gdisplay.settransp(0)
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.setfont(gdisplay.FONT_DEFAULT)
end

-- FUNCTION TO BE RUN IN THREAD!
function tftdemo.thfullDemo()
	local orientation = gdisplay.LANDSCAPE_FLIP
	local sec = 5
	while true do
		gdisplay.setrot(0);
		gdisplay.setcolor(gdisplay.CYAN)
		gdisplay.setfont(gdisplay.FONT_DEFAULT)

		tftdemo.lcdDemo(sec, orientation)
		if tftdemo.touched() then break end

		orientation = (orientation + 1) & 3
	end
end

if tftdemo.init() then
	demo_th = thread.start(tftdemo.thfullDemo)
end