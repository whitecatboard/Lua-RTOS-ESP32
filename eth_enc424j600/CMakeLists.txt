set(fragments
    "iram.lf"
)	   

idf_component_register(
	SRCS "eth_mac_enc424j600.c"
          "eth_phy_enc424j600.c"
    PRIV_REQUIRES driver esp_eth sys
	LDFRAGMENTS "${fragments}"
    INCLUDE_DIRS "."
)