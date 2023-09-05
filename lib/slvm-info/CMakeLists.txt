cmake_minimum_required(VERSION 3.10)

project(slvm-info CXX)

add_library(slvm-info INTERFACE)

target_include_directories(
  slvm-info INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:include>
  )

find_package(cbl REQUIRED)
find_package(config REQUIRED)
find_package(hedron REQUIRED)
find_package(x86 REQUIRED)

target_link_libraries(slvm-info INTERFACE cbl config hedron x86)

target_compile_options(slvm-info INTERFACE -Wall -Wextra)

install(
  TARGETS slvm-info
  EXPORT slvm-infoTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT slvm-infoTargets
  FILE slvm-infoTargets.cmake
  DESTINATION lib/cmake/slvm-info
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES slvm-infoConfig.cmake DESTINATION lib/cmake/slvm-info)

export(TARGETS slvm-info FILE slvm-infoTargets.cmake)
