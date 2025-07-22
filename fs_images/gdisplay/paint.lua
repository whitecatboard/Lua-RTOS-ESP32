--[[
Simple paint program to demonstrate display & touch on ILI9341 based displays
Author: LoBo, loboris@gmail.com (https://github.com/loboris)
To load the program execute 'dofile("paint.lua")' or 'require("paint")'
To run the program:
  set the display orientation (optional), 'paint.orient=gdisplay.LANDSCAPE_FLIP'
  execute 'paint.run()' to start the program
Program can also be run in the thread:
  set the display orientation (optional), 'paint.orient=gdisplay.LANDSCAPE_FLIP'
  'paint_th = thread.start(paint.run)'
]]--


paint = {
	orient = gdisplay.PORTRAIT_FLIP,
	running = false
}

-- ------------------------
-- Wait for touch event
-- ------------------------
function paint.wait(tpwait)
	local touch
	while true do
		touch, _, _ = gdisplay.gettouch()
		if ((touch > 0) and (tpwait == 1)) or ((touch == 0) and (tpwait == 0)) then
			break
		end
		tmr.sleepms(10)
	end
end

-- ------------------------------------
-- Display selection bars and some info
-- ------------------------------------
function paint.paint_info()
	local dispx, dispy, dx, dy, dw, dh, fp, i

	gdisplay.setorient(paint.orient)
	gdisplay.setfont(gdisplay.FONT_DEFAULT)
	gdisplay.setrot(0)
	gdisplay.setfixed(0)
	gdisplay.clear(gdisplay.BLACK)
	dispx, dispy = gdisplay.getscreensize()
	dx = dispx // 8
	dw,dh = gdisplay.getfontsize()
	dy = dispy - dh - 5
	fp = math.ceil((dx // 2) - (dw // 2))

	-- draw color bar, orange selected
	gdisplay.rect(dx*0,0,dx-2,18,gdisplay.BLACK,gdisplay.BLACK)
	gdisplay.rect(dx*1,0,dx-2,18,gdisplay.WHITE,gdisplay.WHITE)
	gdisplay.rect(dx*2,0,dx-2,18,gdisplay.RED,gdisplay.RED)
	gdisplay.rect(dx*3,0,dx-2,18,gdisplay.GREEN,gdisplay.GREEN)
	gdisplay.rect(dx*4,0,dx-2,18,gdisplay.BLUE,gdisplay.BLUE)
	gdisplay.rect(dx*5,0,dx-2,18,gdisplay.YELLOW,gdisplay.YELLOW)
	gdisplay.rect(dx*6,0,dx-2,18,gdisplay.CYAN,gdisplay.CYAN)
	gdisplay.rect(dx*7,0,dx-2,18,gdisplay.ORANGE,gdisplay.ORANGE)

	gdisplay.rect(dx*7+2,2,dx-6,14,gdisplay.WHITE, gdisplay.ORANGE)
	gdisplay.rect(dx*7+3,3,dx-8,12,gdisplay.BLACK, gdisplay.ORANGE)

	for i=1,7,1 do
		gdisplay.line(dx*i-1,0,dx*i-1,18,gdisplay.LIGHTGREY)
		gdisplay.line(dx*i-2,0,dx*i-2,18,gdisplay.LIGHTGREY)
	end
	gdisplay.line(0,18,dispx,18,gdisplay.LIGHTGREY)
	
	-- draw functions bar, size 2 selected
	gdisplay.rect(dx*0,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
	gdisplay.rect(dx*1,dy,dx-2,dispy-dy,gdisplay.YELLOW)
	gdisplay.rect(dx*2,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
	gdisplay.rect(dx*3,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
	gdisplay.rect(dx*4,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
	gdisplay.rect(dx*5,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
	gdisplay.rect(dx*6,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
	gdisplay.rect(dx*7,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)

	-- write info
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.write(dx*0+fp,dy+3,"2")
	gdisplay.write(dx*1+fp,dy+3,"4")
	gdisplay.write(dx*2+fp,dy+3,"6")
	gdisplay.write(dx*3+fp,dy+3,"8")
	gdisplay.write(dx*4+fp-3,dy+3,"10")
	--gdisplay.write(dx*5+fp,dy+3,"S")
	gdisplay.circle(dx*5+((dx-2) // 2), dy+((dispy-dy) // 2), (dispy-dy) // 2 - 2, gdisplay.LIGHTGREY, gdisplay.LIGHTGREY)
	gdisplay.write(dx*6+fp,dy+3,"C")
	gdisplay.write(dx*7+fp,dy+3,"R")

	gdisplay.setcolor(gdisplay.YELLOW)
	gdisplay.write(60,40,"S")
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.write(gdisplay.LASTX,gdisplay.LASTY," change shape")
	gdisplay.circle(60+(dw // 2), 40+(dh // 2), dh // 2 + 1, gdisplay.LIGHTGREY, gdisplay.LIGHTGREY)

	gdisplay.setcolor(gdisplay.YELLOW)
	gdisplay.write(60,dh*2+40,"C")
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.write(gdisplay.LASTX,gdisplay.LASTY," Clear screen")

	gdisplay.setcolor(gdisplay.YELLOW)
	gdisplay.write(60,dh*4+40,"R")
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.write(gdisplay.LASTX,gdisplay.LASTY," Return, exit program")

	gdisplay.setcolor(gdisplay.YELLOW)
	gdisplay.write(60,dh*6+40,"2,4,6,8")
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.write(gdisplay.LASTX,gdisplay.LASTY," draw size")

	gdisplay.setcolor(gdisplay.ORANGE)
	gdisplay.setfont(gdisplay.FONT_UBUNTU16)
	gdisplay.write(gdisplay.CENTER,dh*10+40,"Touch screen to start")
	gdisplay.setfont(gdisplay.FONT_DEFAULT)

	paint.wait(0)
	paint.wait(1)
	paint.wait(0)

	gdisplay.rect(0,20,dispx,dy-20,gdisplay.BLACK,gdisplay.BLACK)

	return dx, dy
end

-- -----------------
-- Paint main loop
-- -----------------
function paint.run()
	if paint.running then
		print("Already running")
		return
	end

	local dispx, dispy, dx, dy, x, y, touch, lx, lx, first, drw, dodrw, color, lastc, lastr

	paint.running = true

	dx, dy = paint.paint_info()
	dispx, dispy = gdisplay.getscreensize()

	first = true
	drw = 1
	color = gdisplay.ORANGE
	lastc = dx*7
	r = 4
	lastr = dx

	while true do
		tmr.sleepms(10)
		-- get touch status and coordinates
		touch, x, y = gdisplay.gettouch()
		if touch > 0 then
			if first and ((y < 20) or (y > dy)) then
				-- color or functions bar touched
				-- check if after 100 ms we are on the same position
				lx = x
				ly = y
				tmr.sleepms(100)
				paint.wait(1)
				if (math.abs(x-lx) < 4) and (math.abs(y-ly) < 4) then
					if (y < 20) then
						-- === first touch, upper bar touched: color select ===
						gdisplay.rect(lastc,0,dx-2,18,color,color)
						if x > (dx*7) then
							color = gdisplay.ORANGE
							lastc = dx*7
						elseif x > (dx*6) then
							color = gdisplay.CYAN
							lastc = dx*6
						elseif x > (dx*5) then
							color = gdisplay.YELLOW
							lastc = dx*5
						elseif x > (dx*4) then
							color = gdisplay.BLUE
							lastc = dx*4
						elseif x > (dx*3) then
							color = gdisplay.GREEN
							lastc = dx*3
						elseif x > (dx*2) then
							color = gdisplay.RED
							lastc = dx*2
						elseif x > dx then
							color = gdisplay.WHITE
							lastc = dx
						elseif x > 1 then
							color = gdisplay.BLACK
							lastc = 0
						end
						gdisplay.rect(lastc+2,2,dx-6,14,gdisplay.WHITE,color)
						gdisplay.rect(lastc+3,3,dx-8,12,gdisplay.BLACK,color)

						-- wait for touch release
						paint.wait(0)
						first = true

					elseif (y > dy) then
						-- === first touch, lower bar touched: change size, r, erase, shape select, return ===
						if x < (dx*5) then
							gdisplay.rect(lastr,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY)
						end
						if x > (dx*7) then
							break
						elseif x > (dx*6) then
							-- clear drawing area
							gdisplay.rect(0,20,dispx,dy-20,gdisplay.BLACK,gdisplay.BLACK)
						elseif x > (dx*5) then
							-- change drawing shape
							drw = drw + 1
							if drw > 4 then
								drw = 1
							end
							gdisplay.rect(dx*5,dy,dx-2,dispy-dy,gdisplay.LIGHTGREY, gdisplay.BLACK)
							if drw == 1 then
								gdisplay.circle(dx*5+((dx-2) // 2), dy+((dispy-dy) // 2), (dispy-dy) // 2 - 2, gdisplay.LIGHTGREY, gdisplay.LIGHTGREY)
							elseif drw == 3 then
								gdisplay.rect(dx*5+6, dy+2, dx-14, dispy-dy-4, gdisplay.LIGHTGREY, gdisplay.LIGHTGREY)
							elseif drw == 2 then
								gdisplay.circle(dx*5+((dx-2) // 2), dy+((dispy-dy) // 2), (dispy-dy) // 2 - 2, gdisplay.YELLOW, gdisplay.DARKGREY)
							elseif drw == 4 then
								gdisplay.rect(dx*5+6, dy+2, dx-14, dispy-dy-4, gdisplay.YELLOW, gdisplay.DARKGREY)
							end
						-- drawing size
						elseif x > (dx*4) then
							r = 10
							lastr = dx*4
						elseif x > (dx*3) then
							r = 8
							lastr = dx*3
						elseif x > (dx*2) then
							r = 6
							lastr = dx*2
						elseif x > dx then
							r = 4
							lastr = dx
						elseif x > 0 then
							r = 2
							lastr = 0
						end
						if x < (dx*5) then
							gdisplay.rect(lastr,dy,dx-2,dispy-dy,gdisplay.YELLOW)
						end
						-- wait for touch release
						paint.wait(0)
						first = true
					end
				end
			elseif (x > r) and (y > (r+20)) and (y < (dy-r)) then
				-- === touch on drawing area, draw shape ===
				dodrw = true
				if first == false then
					-- it is NOT first touch, draw only if coordinates changed, but no more than 5 pixels
					if ((x == lx) and (y == ly)) or (math.abs(x-lx) > 5) or (math.abs(y-ly) > 5) then
						dodrw = false
					end
				end
				
				if dodrw == true then
					-- draw with active shape
					if drw == 1 then
						gdisplay.circle(x, y, r, color, color)
					elseif drw == 3 then
						gdisplay.rect(x-r, y-r, r*2, r*2, color, color)
					elseif drw == 2 then
						gdisplay.circle(x, y, r, color, gdisplay.DARKGREY)
					elseif drw == 4 then
						gdisplay.rect(x-r, y-r, r*2, r*2, color, gdisplay.DARKGREY)
					end
				end
				-- save touched coordinates
				lx = x
				ly = y
				first = false
			end
		else
			-- not touched
			first = true
			paint.wait(1)
		end
	end

	gdisplay.rect(0,dy,dispx,dispy-dy,gdisplay.YELLOW,gdisplay.BLACK)
	gdisplay.setcolor(gdisplay.CYAN)
	gdisplay.write(gdisplay.CENTER, dy+3, "FINISHED")

	paint.running = false
end

gdisplay.init(gdisplay.ILI9341,gdisplay.LANDSCAPE_FLIP)
paint.run()