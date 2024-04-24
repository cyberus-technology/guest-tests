add_library(pprintpp INTERFACE)

target_include_directories(
  pprintpp INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS pprintpp
  EXPORT pprintppTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )
