add_library(first-fit-heap INTERFACE)

target_include_directories(
  first-fit-heap
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS first-fit-heap
  EXPORT first-fit-heapTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )
