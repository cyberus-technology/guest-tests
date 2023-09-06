cmake_minimum_required(VERSION 3.10)

project(memory CXX)

find_package(cbl REQUIRED)
find_package(config REQUIRED)
find_package(logger REQUIRED)
find_package(x86 REQUIRED)

add_library(memory INTERFACE)

target_include_directories(
  memory INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>
  )

target_compile_options(memory INTERFACE -Wall -Wextra)

target_link_libraries(memory INTERFACE cbl config logger x86)

install(
  TARGETS memory
  EXPORT memoryTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT memoryTargets
  FILE memoryTargets.cmake
  DESTINATION lib/cmake/memory
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES memoryConfig.cmake DESTINATION lib/cmake/memory)

export(TARGETS memory FILE memoryTargets.cmake)
