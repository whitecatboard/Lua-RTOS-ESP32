-- Attach stepper
_stepperGPIO14 = stepper.attach(pio.GPIO26, pio.GPIO14)

-- Set a callback when a Lora Frame is received
lora.whenReceived(function(_port, _payload)
	print("Paquet rebut")
	print("  port:".. _port)
	print("  payload: ".._payload)
	
	valor, _payload = pack.unpack(_payload, false)
	print("  valor: "..valor)
	
	if valor == 1 then
		_stepperGPIO14:move(0.25, 60, 2)
		stepper.start(_stepperGPIO14)
	elseif valor == 0 then
		_stepperGPIO14:move(-0.25, 60, 2)
		stepper.start(_stepperGPIO14)
    end
end)

while true do
    -- Setup Lora
    lora.setup(lora.BAND868)
    lora.setDevAddr("26011728")
    lora.setNwksKey("5DD8013B282A725F51B1227F311EEFB1")
    lora.setAppsKey("9F09A4C74BAEDCB06669A4AC1D7ACB68")
    lora.setDr(5)
    lora.setReTx(0)
    
    -- Send a frame, for enable downlink
    print("Enviant ...")
    lora.tx(false, 2, '00')

    -- Wait 5 seconds
    tmr.delay(5)
end