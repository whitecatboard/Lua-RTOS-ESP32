cmake_policy(SET CMP0057 NEW)

set(project_path ${CMAKE_SOURCE_DIR})

include("${project_path}/build/config/sdkconfig.cmake")

function(fs_prepare)
  # Set partition to use
  set(fs_partition storage)

  # Don't include this components
  set(fs_exclude_components "romfs_image" "cmake")

  # Set the root path
  if("${FS_ROOT_PATH}" STREQUAL "")
    if("${BOARD_FILE_SYSTEM}" STREQUAL "")
      set(FS_ROOT_PATH "${project_path}/fs_images/default")
    else()		
      set(FS_ROOT_PATH "${project_path}/fs_images/${BOARD_FILE_SYSTEM}")
    endif()
  endif()
 
  if (NOT EXISTS "${FS_ROOT_PATH}")
    message("${FS_ROOT_PATH} doesn't exist")
  endif()
  
  # Set the search path
  if("${FS_SEARCH_PATH}" STREQUAL "")
    set(FS_SEARCH_PATH "${project_path}" "${project_path}/lua/modules")
  endif()

  foreach(cd ${FS_SEARCH_PATH})
    if (NOT EXISTS "${cd}")
      message("${cd} doesn't exist")
    endif()
  endforeach()

  # Get all file system fragments found in FS_SEARCH_PATH
  set (fs_fragments)
  
  foreach(cd ${FS_SEARCH_PATH})
    # Get all directories that contains a fs.cmake file
    file(GLOB_RECURSE fs_cmake_files LIST_DIRECTORIES false "${cd}/fs.cmake")
  
    # Process each fs.cmake file 
    foreach(fs_cmake_file ${fs_cmake_files})
      # Get component path
      get_filename_component(component_path ${fs_cmake_file} DIRECTORY)
    
      # Get component name
      get_filename_component(component_name ${component_path} NAME)
    
      # Process component if not excluded
      if (NOT ${component_name} IN_LIST fs_exclude_components) 
        include(${fs_cmake_file})

        if(NOT ${COMPONENT_ADD_FS} STREQUAL "")
          # Include fragment      
          foreach(add_folder ${COMPONENT_ADD_FS})
            list(APPEND fs_fragments "${component_path}/${add_folder}")
          endforeach()
        endif()     	
      endif()    
    endforeach()  
  endforeach()
  
  # Remove duplicates and sort
  list(APPEND fs_fragments "${FS_ROOT_PATH}")
  list(REMOVE_DUPLICATES fs_fragments)
  list(SORT fs_fragments)

  # Copy to tmp-fs folder  
  file(REMOVE_RECURSE ${project_path}/build/tmp-fs)
  make_directory(${project_path}/build/tmp-fs)
  
  foreach(fs_fragment ${fs_fragments})
    file(GLOB_RECURSE fragment_files LIST_DIRECTORIES  false "${fs_fragment}/*")
    foreach(fragment_file ${fragment_files})
      get_filename_component(fragment_path ${fragment_file} DIRECTORY)
      string(REPLACE ${fs_fragment}  "" fragment_path ${fragment_path})      
      make_directory(${project_path}/build/tmp-fs${fragment_path})            
      file(COPY ${fragment_file} DESTINATION ${project_path}/build/tmp-fs${fragment_path})
    endforeach()
  endforeach()  
endfunction()

fs_prepare()