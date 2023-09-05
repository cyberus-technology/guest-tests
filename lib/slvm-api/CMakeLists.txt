cmake_minimum_required(VERSION 3.10)

project(slvm-api CXX)

add_library(slvm-api INTERFACE)

target_include_directories(
  slvm-api INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include>
  )

target_compile_options(slvm-api INTERFACE -Wall -Wextra)

install(
  TARGETS slvm-api
  EXPORT slvm-apiTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT slvm-apiTargets
  FILE slvm-apiTargets.cmake
  DESTINATION lib/cmake/slvm-api
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES slvm-apiConfig.cmake DESTINATION lib/cmake/slvm-api)

export(TARGETS slvm-api FILE slvm-apiTargets.cmake)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test/unit)
endif()
