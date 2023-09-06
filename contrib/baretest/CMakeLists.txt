cmake_minimum_required(VERSION 3.10)

project(baretest-contrib CXX)

add_library(baretest-contrib STATIC baretest/src/baretest.cpp baretest/src/print.cpp)

target_include_directories(
    baretest-contrib PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/baretest/include>
                      $<INSTALL_INTERFACE:include>
  )

target_link_libraries(baretest-contrib PUBLIC libcxx baretest-config)

install(
  TARGETS baretest-contrib
  EXPORT baretest-contribTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT baretest-contribTargets
  FILE baretest-contribTargets.cmake
  DESTINATION lib/cmake/baretest-contrib
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/baretest/include/ DESTINATION include/)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES baretest-contribConfig.cmake DESTINATION lib/cmake/baretest-contrib)

export(TARGETS baretest-contrib FILE baretest-contribTargets.cmake)
