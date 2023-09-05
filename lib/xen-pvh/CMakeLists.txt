cmake_minimum_required(VERSION 3.10)

project(xen-pvh CXX)

add_library(xen-pvh INTERFACE)

target_include_directories(
  xen-pvh INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>
  )

target_compile_options(xen-pvh INTERFACE -Wall -Wextra)

install(
  TARGETS xen-pvh
  EXPORT xen-pvhTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT xen-pvhTargets
  FILE xen-pvhTargets.cmake
  DESTINATION lib/cmake/xen-pvh
  )

install(FILES include/xen-pvh.hpp DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES xen-pvhConfig.cmake DESTINATION lib/cmake/xen-pvh)

export(TARGETS xen-pvh FILE xen-pvhTargets.cmake)
