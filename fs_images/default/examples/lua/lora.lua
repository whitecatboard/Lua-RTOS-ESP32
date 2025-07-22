function send()
  -- Update log level for show all log messages
  os.loglevel(os.LOG_ALL)
  
  -- Setup LoRa Wan, and send a confirmed packet
  lora.attach(lora.BAND868)
  lora.setAppEui("70B3D57EF0001E32")
  lora.setAppKey("796B86B4C9DC0252FA32457B1315E284")
  lora.setDr(0)
  lora.setAdr(false)
  lora.join(lora.OTAA)
  lora.tx(true,1,pack.pack(25.3))
end

send()
