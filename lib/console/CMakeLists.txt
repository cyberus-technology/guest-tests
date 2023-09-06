cmake_minimum_required(VERSION 3.10)

project(console CXX)

add_library(console INTERFACE)

target_include_directories(
  console INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>
  )

target_compile_options(console INTERFACE -Wall -Wextra)

install(
  TARGETS console
  EXPORT consoleTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT consoleTargets
  FILE consoleTargets.cmake
  DESTINATION lib/cmake/console
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES consoleConfig.cmake DESTINATION lib/cmake/console)

export(TARGETS console FILE consoleTargets.cmake)
