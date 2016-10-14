led = pio.GPIO12

pio.pin.setdir(pio.OUTPUT, led)

while true do
  pio.pin.sethigh(led)
  tmr.delayms(500)
  pio.pin.setlow(led)
  tmr.delayms(500)
end
