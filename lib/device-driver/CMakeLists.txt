cmake_minimum_required(VERSION 3.10)

project(device-driver CXX)

add_library(device-driver INTERFACE)

target_include_directories(
  device-driver INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>
  )

target_compile_options(device-driver INTERFACE -Wall -Wextra)

install(
  TARGETS device-driver
  EXPORT device-driverTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT device-driverTargets
  FILE device-driverTargets.cmake
  DESTINATION lib/cmake/device-driver
  )

install(FILES include/device-driver/adapter.hpp DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES device-driverConfig.cmake DESTINATION lib/cmake/device-driver)

export(TARGETS device-driver FILE device-driverTargets.cmake)
