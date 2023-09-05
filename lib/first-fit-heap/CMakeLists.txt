cmake_minimum_required(VERSION 3.10)

project(first-fit-heap CXX)

add_library(first-fit-heap INTERFACE)

find_package(libsn REQUIRED)

target_include_directories(
  first-fit-heap INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
  )

target_compile_definitions(
        first-fit-heap INTERFACE HEAP_FREESTANDING HEAP_ASSERT
)

target_compile_options(
        first-fit-heap INTERFACE -Wno-unused-parameter
)

target_link_libraries(first-fit-heap INTERFACE first-fit-heap-contrib libsn)

install(
  TARGETS first-fit-heap
  EXPORT first-fit-heapTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT first-fit-heapTargets
  FILE first-fit-heapTargets.cmake
  DESTINATION lib/cmake/first-fit-heap
  )

install(FILES include/heap_freestanding.hpp DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES first-fit-heapConfig.cmake DESTINATION lib/cmake/first-fit-heap)

export(TARGETS first-fit-heap FILE first-fit-heapTargets.cmake)
