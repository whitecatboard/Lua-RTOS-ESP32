function send()
  -- Update log level for show all log messages
  os.loglevel(os.LOG_ALL)
  
  -- Setup LoRa Wan, and send a confirmed packet
  lora.setup(lora.BAND868)
  lora.setAppEui("70B3D57ED0000740")
  lora.setAppKey("8CDB905A8073C80FCE188EBC674109FC")
  lora.setDevEui("70B3D57E00000000")
  lora.setDr(0)
  lora.setAdr(false)
  lora.join(lora.OTAA)
  lora.tx(true,1,pack.pack(25.3))
end

send()
