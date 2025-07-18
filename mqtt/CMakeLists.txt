if(CONFIG_LUA_RTOS_LUA_USE_MQTT)
	set(srcs
        "Clients.c"
        "LinkedList.c"
        "Log.c"
        "Messages.c"
        "MQTTAsync.c"
        "MQTTPacket.c"
        "MQTTPacketOut.c"
        "MQTTPersistence.c"
        "MQTTPersistenceDefault.c"
        "MQTTProtocolClient.c"
        "MQTTProtocolOut.c"
        "OsWrapper.c"
        "Socket.c"
        "SocketBuffer.c"
        "SSLSocket.c"
        "Thread.c"
        "utf-8.c"
	)
	
	set(include_dirs ".")
else()
	set(srcs "")
	set(include_dirs "")
endif()

idf_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${include_dirs}"
	PRIV_REQUIRES pthread openssl sys
)

target_compile_options(${COMPONENT_LIB} PRIVATE -DNOSTACKTRACE)
target_compile_options(${COMPONENT_LIB} PRIVATE -DOPENSSL)
target_compile_options(${COMPONENT_LIB} PRIVATE -DHIGH_PERFORMANCE)
