add_library(libc-tiny INTERFACE)

target_include_directories(
  libc-tiny INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS libc-tiny
  EXPORT libc-tinyTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )
