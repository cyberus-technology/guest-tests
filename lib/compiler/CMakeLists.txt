cmake_minimum_required(VERSION 3.10)

project(compiler CXX)

add_library(compiler INTERFACE)

target_include_directories(
  compiler INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include>
  )

target_compile_options(compiler INTERFACE -Wall -Wextra)

install(
  TARGETS compiler
  EXPORT compilerTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT compilerTargets
  FILE compilerTargets.cmake
  DESTINATION lib/cmake/compiler
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES compilerConfig.cmake DESTINATION lib/cmake/compiler)

export(TARGETS compiler FILE compilerTargets.cmake)
