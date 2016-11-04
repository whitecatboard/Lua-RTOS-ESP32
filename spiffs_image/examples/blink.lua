thread.start(function()
  pio.pin.setdir(pio.OUTPUT, pio.GPIO14)
  pio.pin.setpull(pio.NOPULL, pio.GPIO14)
  while true do
    pio.pin.setval(1, pio.GPIO14)
    tmr.delayms(100)
    pio.pin.setval(0, pio.GPIO14)
    tmr.delayms(100)
  end
end)