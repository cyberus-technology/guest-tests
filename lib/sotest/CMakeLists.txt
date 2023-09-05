cmake_minimum_required(VERSION 3.10)

project(sotest CXX)

find_package(logger REQUIRED)

add_library(sotest INTERFACE)

target_include_directories(
  sotest INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>
  )

target_link_libraries(sotest INTERFACE logger)

target_compile_options(sotest INTERFACE -Wall -Wextra)

install(
  TARGETS sotest
  EXPORT sotestTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT sotestTargets
  FILE sotestTargets.cmake
  DESTINATION lib/cmake/sotest
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES sotestConfig.cmake DESTINATION lib/cmake/sotest)

export(TARGETS sotest FILE sotestTargets.cmake)
