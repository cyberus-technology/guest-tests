cmake_minimum_required(VERSION 3.10)

project(emulator ASM CXX)

add_library(emulator STATIC emulator.cpp native.S)

find_package(decoder REQUIRED)
find_package(x86 REQUIRED)

target_compile_features(emulator PUBLIC cxx_std_17)

target_include_directories(
  emulator PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                  $<INSTALL_INTERFACE:include>
  )

target_compile_options(emulator PRIVATE -Wall -Wextra)

target_link_libraries(emulator PUBLIC decoder x86)

install(
  TARGETS emulator
  EXPORT emulatorTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT emulatorTargets
  FILE emulatorTargets.cmake
  DESTINATION lib/cmake/emulator
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/emulator
        DESTINATION include/emulator
  )

# In order to make libraries discoverable via find_package we have to separate
# a *Config.cmake file and export the lib.
install(FILES emulatorConfig.cmake DESTINATION lib/cmake/emulator)

export(TARGETS emulator FILE emulatorTargets.cmake)
