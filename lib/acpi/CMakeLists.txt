cmake_minimum_required(VERSION 3.10)

project(acpi CXX)

add_library(acpi INTERFACE)

target_include_directories(
  acpi INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:include>
  )

target_compile_options(acpi INTERFACE -Wall -Wextra)

install(
  TARGETS acpi
  EXPORT acpiTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT acpiTargets
  FILE acpiTargets.cmake
  DESTINATION lib/cmake/acpi
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES acpiConfig.cmake DESTINATION lib/cmake/acpi)

export(TARGETS acpi FILE acpiTargets.cmake)

