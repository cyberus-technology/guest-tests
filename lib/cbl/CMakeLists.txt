cmake_minimum_required(VERSION 3.10)

project(cbl CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(cbl INTERFACE)

find_package(compiler REQUIRED)
find_package(libcxx REQUIRED)
find_package(logger REQUIRED)
find_package(math REQUIRED)
find_package(memory REQUIRED)

target_include_directories(
  cbl INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_compile_options(cbl INTERFACE -Wall -Wextra)

target_link_libraries(cbl INTERFACE compiler logger math memory libcxx)

install(
  TARGETS cbl
  EXPORT cblTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT cblTargets
  FILE cblTargets.cmake
  DESTINATION lib/cmake/cbl
  )

install(DIRECTORY
        ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION include)

install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/include/algorithm.hpp
        DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES cblConfig.cmake DESTINATION lib/cmake/cbl)

export(TARGETS cbl FILE cblTargets.cmake)
