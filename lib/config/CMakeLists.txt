cmake_minimum_required(VERSION 3.10)

project(config CXX)

find_package(x86 REQUIRED)

add_library(config INTERFACE)

target_include_directories(
  config INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>
  )

target_compile_options(config INTERFACE -Wall -Wextra)

target_link_libraries(config INTERFACE x86)

install(
  TARGETS config
  EXPORT configTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT configTargets
  FILE configTargets.cmake
  DESTINATION lib/cmake/config
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES configConfig.cmake DESTINATION lib/cmake/config)

export(TARGETS config FILE configTargets.cmake)
