idf_build_get_property(python PYTHON)
idf_build_get_property(project_path PROJECT_DIR)

# Define BUILD_TIME flag, with the EPOCH time when Lua RTOS is build.
# This is used for set system time when RTC is not set.
if("${LUA_RTOS_BUILD_TIME}" STREQUAL "")
  execute_process(
      COMMAND python -c "from datetime import datetime;print(int(datetime.now().timestamp()))"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE LUA_RTOS_BUILD_TIME OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

# Define BUILD_COMMIT flag, with the commit hash number used in build.
# This is used in os.version Lua function for return the build commit.
if("${LUA_RTOS_BUILD_COMMIT}" STREQUAL "")
  execute_process(
      COMMAND git rev-parse HEAD
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE LUA_RTOS_BUILD_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

add_custom_target(gen-luartos-conf ALL
  COMMAND ${CMAKE_COMMAND} -E echo "#pragma once" > ${project_path}/build/config/build_time.h
  COMMAND ${CMAKE_COMMAND} -E echo "#define BUILD_TIME ${LUA_RTOS_BUILD_TIME}" >> ${project_path}/build/config/build_time.h
  COMMAND ${CMAKE_COMMAND} -E echo "#pragma once" > ${project_path}/build/config/build_commit.h
  COMMAND ${CMAKE_COMMAND} -E echo "#define BUILD_COMMIT \"${LUA_RTOS_BUILD_COMMIT}\"" >> ${project_path}/build/config/build_commit.h
  VERBATIM
)