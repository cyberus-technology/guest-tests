cmake_minimum_required(VERSION 3.10)

project(baretest-config CXX)

add_library(baretest-config STATIC baretest_config.cpp)

target_include_directories(
  baretest-config PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>
  )

target_link_libraries(baretest-config PUBLIC libsn sotest)

install(
  TARGETS baretest-config
  EXPORT baretest-configTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT baretest-configTargets
  FILE baretest-configTargets.cmake
  DESTINATION lib/cmake/baretest-config
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES baretest-configConfig.cmake DESTINATION lib/cmake/baretest-config)

export(TARGETS baretest-config FILE baretest-configTargets.cmake)
