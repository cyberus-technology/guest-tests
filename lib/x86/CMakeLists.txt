cmake_minimum_required(VERSION 3.10)

project(x86 CXX)

add_library(
  x86 STATIC arch.cpp segmentation.cpp virtual_msr.cpp virtual_msr_bus.cpp
  )

find_package(cbl REQUIRED)
find_package(compiler REQUIRED)
find_package(logger REQUIRED)
find_package(pprintpp REQUIRED)

target_compile_features(x86 PUBLIC cxx_std_17)

target_include_directories(
  x86 PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
             $<INSTALL_INTERFACE:include>
  )

target_link_libraries(
  x86
  PUBLIC cbl
  PRIVATE compiler logger pprintpp
  )

target_compile_options(x86 INTERFACE -Wall -Wextra)

install(
  TARGETS x86
  EXPORT x86Targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT x86Targets
  FILE x86Targets.cmake
  DESTINATION lib/cmake/x86
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make libraries discoverable via find_package we have to
# separate a *Config.cmake file and export the lib.
install(FILES x86Config.cmake DESTINATION lib/cmake/x86)

export(TARGETS x86 FILE x86Targets.cmake)
