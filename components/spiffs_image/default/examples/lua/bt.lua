bt.attach(bt.mode.BLE)

advData = "02".. -- Length	Flags. CSS v5, Part A, 1.3
					"01".. -- Flags data type value
					"06".. -- Flags data GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
					"03".. -- Length	Complete list of 16-bit Service UUIDs. Ibid. ยง 1.1
					"03".. -- Complete list of 16-bit Service UUIDs data type value
					"AA".. -- 16-bit Eddystone UUID 0xAA LSB
					"FE".. -- 16-bit Eddystone UUID 0xFE MSB
					"17".. -- Length	Service Data. Ibid. ยง 1.11 23 bytes from this point
					"16".. -- Service Data data type value
					"AA".. -- 16-bit Eddystone UUID 0xAA LSB
					"FE".. -- 16-bit Eddystone UUID 0xFE MSB
					"00".. -- Frame Type Eddystone UUID Value = 0x00
					"ED".. -- Ranging Data	Calibrated Tx power at 0 m  -60dBm meter + 41dBm = -19dBm
					"3F".. -- 10-byte Namespace uuid generated
					"3B".. -- 10-byte Namespace uuid generated
					"51".. -- 10-byte Namespace uuid generated
					"B6".. -- 10-byte Namespace uuid generated
					"0B".. -- 10-byte Namespace uuid generated
					"88".. -- 10-byte Namespace uuid generated
					"EF".. -- 10-byte Namespace uuid generated
					"99".. -- 10-byte Namespace uuid generated
					"49".. -- 10-byte Namespace uuid generated
					"E5".. -- 10-byte Namespace uuid generated
					"CF".. -- 6-byte Instance MAC address
					"48".. -- 6-byte Instance MAC address
					"41".. -- 6-byte Instance MAC address
					"85".. -- 6-byte Instance MAC address
					"AE".. -- 6-byte Instance MAC address
					"0B".. -- 6-byte Instance MAC address
					"00".. -- RFU Reserved for future use, must be0x00
					"00"   -- RFU Reserved for future use, must be0x00

bt.advertise(160, 160, bt.adv.ADV_IND, bt.ownaddr.Public, bt.peeraddr.Public, "808182838485", bt.chann.All, bt.filter.ConnAllScanAll, advData)
