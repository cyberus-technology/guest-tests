cmake_minimum_required(VERSION 3.10)

project(toyos CXX)

add_library(toyos STATIC mm.cpp pd.cpp pdpt.cpp pml4.cpp pt.cpp)

find_package(cbl REQUIRED)
find_package(x86 REQUIRED)

target_include_directories(
  toyos PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_compile_options(toyos PRIVATE -Wall -Wextra)

target_link_libraries(toyos PUBLIC cbl x86)

install(
  TARGETS toyos
  EXPORT toyosTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT toyosTargets
  FILE toyosTargets.cmake
  DESTINATION lib/cmake/toyos
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES toyosConfig.cmake DESTINATION lib/cmake/toyos)

export(TARGETS toyos FILE toyosTargets.cmake)
