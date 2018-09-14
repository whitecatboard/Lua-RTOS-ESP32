-- Touch pannel calibration for ILI9341 based displays

if dispType == nil then
    dispType = tft.ILI9341
end

tft.init(dispType,tft.LANDSCAPE)
if tft.gettype() < 0 then
    print("LCD not initialized")
    return
end

-- === SELECT THE RIGHT tp CS !! ===
-- tft.set_touch_cs(2)

local dispx, dispy
dispx = 320
dispy = 240

local rx = {}
local ry = {}

-- ------------------------------------
local function drawCrossHair(x, y, clr)
  tft.rect(x-10, y-10, 20, 20, clr)
  tft.line(x-5, y, x+5, y, clr)
  tft.line(x, y-5, x, y+5, clr)
end

-- ------------------------------
local function readCoordinates()
  local x = 0
  local y = 0
  local touch
  local sx = 0
  local sy = 0
  local n = 0
  
  tft.setcolor(tft.CYAN)

  while n < 8 do
    tft.write(tft.CENTER, 110, " PRESS ")
    -- wait for touch
    touch, x, y = tft.getrawtouch()
    while touch <= 0 do
        touch, x, y = tft.getrawtouch()
    end
    -- wait for release
    while touch > 0 do
        if n == 8 then
            tft.write(tft.CENTER, 110, "RELEASE")
        end
        touch, x, y = tft.getrawtouch()
        if (touch > 0) and (n < 256) then
            sx = sx + x
            sy = sy + y
            n = n + 1
        end
    end
  end

  return (sx / n), (sy / n)
end

-- ------------------------------
local function calibrate(x, y, i)
  drawCrossHair(x,y, tft.YELLOW)
  rx[i], ry[i] = readCoordinates()
  drawCrossHair(x,y,tft.GREEN)
end

-- ---------------------
local function tpcalib()
  local px,py,clx,crx,cty,cby,calx,caly

  tft.setfont(tft.FONT_DEFAULT)
  tft.setfixed(0)
  tft.setrot(0)
  tft.write(tft.CENTER,40,"Touch yellow point and release")
  tft.write(tft.CENTER,60,"Repeat for all 8 calibration points")
  
  tft.setfixed(1)
  drawCrossHair(dispx-11, 10, tft.WHITE)
  drawCrossHair(dispx/2, 10, tft.WHITE)
  drawCrossHair(10, 10, tft.WHITE)
  drawCrossHair(dispx-11, dispy/2, tft.WHITE)
  drawCrossHair(10, dispy/2, tft.WHITE)
  drawCrossHair(dispx-11, dispy-11, tft.WHITE)
  drawCrossHair(dispx/2, dispy-11, tft.WHITE)
  drawCrossHair(10, dispy-11, tft.WHITE)

  calibrate(10, 10, 1)
  calibrate(10, dispy/2, 2)
  calibrate(10, dispy-11, 3)
  calibrate(dispx/2, 10, 4)
  calibrate(dispx/2, dispy-11, 5)
  calibrate(dispx-11, 10, 6)
  calibrate(dispx-11, dispy/2, 7)
  calibrate(dispx-11, dispy-11, 8)

  px = math.abs((((rx[4]+rx[5]+rx[8]) / 3) - ((rx[1]+rx[1]+rx[3]) / 3)) / (dispy-20))  -- LANDSCAPE
  clx = (((rx[1]+rx[2]+rx[3])/3))  -- LANDSCAPE
  crx = (((rx[6]+rx[7]+rx[8])/3))  -- LANDSCAPE

  if (clx < crx) then
    clx = clx - (px*10)
    crx = crx + (px*10)
  else
    clx = clx + (px*10)
    crx = crx - (px*10)
  end

  py = math.abs((((ry[1]+ry[4]+ry[6])/3) - ((ry[3]+ry[5]+ry[8])/3))/(dispx-20))  -- LANDSCAPE
  cty = (((ry[1]+ry[4]+ry[6])/3))  -- LANDSCAPE
  cby = (((ry[3]+ry[5]+ry[8])/3))  -- LANDSCAPE

  if (cty < cby) then
    cty = cty - (py*10)
    cby = cby + (py*10)
  else
    cty = cty + (py*10)
    cby = cby - (py*10)
  end

  calx = (math.ceil(clx) * 65536) + math.ceil(crx)
  caly = (math.ceil(cty) * 65536) + math.ceil(cby)

  tft.setcal(calx, caly)

  nvs.write("touch","calx", calx)
  nvs.write("touch","caly", caly)

  tft.clear(0)
  tft.write(tft.CENTER,40,"Calibration completed")
  tft.write(tft.CENTER,60,"and saved to system parameters")
  print("Calibration completed and saved to system parameters")
end

-- run calibration function
tpcalib()
