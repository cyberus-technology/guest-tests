cmake_minimum_required(VERSION 3.10)

project(first-fit-heap-contrib CXX)

add_library(first-fit-heap-contrib INTERFACE)

target_include_directories(
    first-fit-heap-contrib INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/first-fit-heap/include>
                      $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS first-fit-heap-contrib
  EXPORT first-fit-heap-contribTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT first-fit-heap-contribTargets
  FILE first-fit-heap-contribTargets.cmake
  DESTINATION lib/cmake/first-fit-heap-contrib
  )

install(FILES first-fit-heap/include/heap.hpp first-fit-heap/include/heap_config.hpp
        DESTINATION include
  )

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES first-fit-heap-contribConfig.cmake DESTINATION lib/cmake/first-fit-heap-contrib)

export(TARGETS first-fit-heap-contrib FILE first-fit-heap-contribTargets.cmake)
