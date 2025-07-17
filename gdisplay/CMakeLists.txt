if(CONFIG_LUA_RTOS_LUA_USE_GDISPLAY)
	set(srcs
	    "gdisplay.c"
		"fonts/bcd.c"
		"fonts/comic24.c"
		"fonts/DefaultFont.c"
		"fonts/DejaVuSans18.c"
		"fonts/DejaVuSans24.c"
		"fonts/font.c"
		"fonts/Lcd.c"
		"fonts/minya24.c"
		"fonts/tooney32.c"
		"fonts/Ubuntu16.c"
		"image/bmp.c"
		"image/image.c"
		"image/jpg.c"
		"image/tjpgd.c"   
		"primitives/arc.c"
		"primitives/circle.c"
		"primitives/ellipse.c"
		"primitives/line.c"
		"primitives/polygon.c"
		"primitives/rect.c"
		"primitives/star.c"
		"primitives/triangle.c"	
	    "qrcodegen/qrcodegen.c"
	)
		
	set(include_dirs 
	    "."
	    "fonts"
	    "image"
	    "primitives"
	    "qrcodegen"
	)
else()
	set(srcs "")
	set(include_dirs "")
endif()

idf_component_register(
	SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES sys
)
