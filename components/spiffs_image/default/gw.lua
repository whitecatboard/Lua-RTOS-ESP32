-- net.wf.setup(net.wf.mode.STA, "Xarxa Wi-Fi de JAUME","castellbisbal123")
net.wf.setup(net.wf.mode.STA, "CITILAB","wifi@citilab")

net.wf.start()
net.service.sntp.start()
net.service.sntp.stop()

lora.gw.start()
