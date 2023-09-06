cmake_minimum_required(VERSION 3.10)

project(logger CXX)

find_package(pprintpp REQUIRED)
find_package(libcxx REQUIRED)

add_library(logger INTERFACE)

target_include_directories(
  logger INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>
  )

target_compile_options(logger INTERFACE -Wall -Wextra)

target_link_libraries(logger INTERFACE libcxx pprintpp)

install(
  TARGETS logger
  EXPORT loggerTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT loggerTargets
  FILE loggerTargets.cmake
  DESTINATION lib/cmake/logger
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES loggerConfig.cmake DESTINATION lib/cmake/logger)

export(TARGETS logger FILE loggerTargets.cmake)
