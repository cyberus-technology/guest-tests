cmake_minimum_required(VERSION 3.10)

project(user-api CXX)

add_library(user-api INTERFACE)

target_include_directories(
  user-api INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include>
  )

find_package(hedron REQUIRED)

target_link_libraries(user-api INTERFACE hedron)

target_compile_options(user-api INTERFACE -Wall -Wextra)

install(
  TARGETS user-api
  EXPORT user-apiTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT user-apiTargets
  FILE user-apiTargets.cmake
  DESTINATION lib/cmake/user-api
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES user-apiConfig.cmake DESTINATION lib/cmake/user-api)

export(TARGETS user-api FILE user-apiTargets.cmake)
