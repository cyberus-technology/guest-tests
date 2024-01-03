# Builds a host-system-testable subset of the toyos library.

add_library(toyos-host STATIC src/string_util.cpp)

target_include_directories(
  toyos-host PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>
  )

# Tell the pre-processor that the code is build for the host system.
target_compile_definitions(toyos-host PUBLIC HOSTED)
