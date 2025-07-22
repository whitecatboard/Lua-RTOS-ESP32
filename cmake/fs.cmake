idf_build_get_property(project_path PROJECT_DIR)

partition_table_get_partition_info(FS_BASE_ADDR "--partition-name storage" "offset")
partition_table_get_partition_info(FS_SIZE "--partition-name storage" "size")

if("${CONFIG_LUA_RTOS_USE_SPIFFS}" STREQUAL "y")
  set(fs_type "spiffs")
elseif("${CONFIG_LUA_RTOS_USE_LFS}" STREQUAL "y")  
  set(fs_type "lfs")
endif()

add_custom_target(fs-prepare
  COMMAND ${CMAKE_COMMAND}
  		  -DFS_ROOT_PATH=${FS_ROOT_PATH} 
  		  -DBOARD_FILE_SYSTEM=${BOARD_FILE_SYSTEM} 
          -P "${project_path}/cmake/fs-prepare.cmake"
  WORKING_DIRECTORY ${project_path}
  VERBATIM
)

add_custom_command(OUTPUT ${project_path}/build/${fs_type}_image.img
  COMMAND ${project_path}/mkspiffs/src/mkspiffs.exe 
          -c ${project_path}/build/tmp-fs
          -b ${CONFIG_LUA_RTOS_SPIFFS_LOG_BLOCK_SIZE}
          -p ${CONFIG_LUA_RTOS_SPIFFS_LOG_PAGE_SIZE}
          -s ${FS_SIZE} ${project_path}/build/spiffs_image.img
          
  COMMAND ${ESPTOOLPY} write_flash ${FS_BASE_ADDR} ${project_path}/build/${fs_type}_image.img
  
  VERBATIM
)
 
add_custom_target(flashfs
  DEPENDS gen-part
  DEPENDS fs-prepare             
  DEPENDS ${project_path}/build/${fs_type}_image.img
  COMMAND ${CMAKE_COMMAND} -E remove ${project_path}/build/${fs_type}_image.img
)
 