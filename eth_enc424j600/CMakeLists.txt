if(CONFIG_SPI_ETH_CHIP_ENC424J600)
	set(srcs
	    "eth_mac_enc424j600.c"
	    "eth_phy_enc424j600.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

set(fragments
    "iram.lf"
)	   

idf_component_register(
	SRCS "${srcs}"
    PRIV_REQUIRES driver esp_eth sys
	LDFRAGMENTS "${fragments}"
    INCLUDE_DIRS "."
)