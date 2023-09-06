cmake_minimum_required(VERSION 3.10)

project(console-serial CXX)

add_library(console-serial STATIC console_serial.cpp)

find_package(console REQUIRED)
find_package(libcxx REQUIRED)
find_package(logger REQUIRED)

target_include_directories(
  console-serial PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                        $<INSTALL_INTERFACE:include>
  )

target_compile_options(console-serial PRIVATE -Wall -Wextra)

target_link_libraries(console-serial PUBLIC cbl console logger libcxx)

install(
  TARGETS console-serial
  EXPORT console-serialTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT console-serialTargets
  FILE console-serialTargets.cmake
  DESTINATION lib/cmake/console-serial
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/serial DESTINATION include/serial)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES console-serialConfig.cmake DESTINATION lib/cmake/console-serial)

export(TARGETS console-serial FILE console-serialTargets.cmake)
