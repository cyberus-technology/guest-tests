cmake_minimum_required(VERSION 3.10)

project(libc-tiny CXX ASM)

add_library(libc-tiny INTERFACE)

target_include_directories(
  libc-tiny INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                      $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS libc-tiny
  EXPORT libc-tinyTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(DIRECTORY include DESTINATION include)

install(
  EXPORT libc-tinyTargets
  FILE libc-tinyTargets.cmake
  DESTINATION lib/cmake/libc-tiny
  )

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES libc-tinyConfig.cmake DESTINATION lib/cmake/libc-tiny)

export(TARGETS libc-tiny FILE libc-tinyTargets.cmake)
