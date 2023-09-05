cmake_minimum_required(VERSION 3.10)

project(vcpu CXX)

add_library(vcpu STATIC memory_helper.cpp)

find_package(emulator REQUIRED)
find_package(hedron REQUIRED)
find_package(logger REQUIRED)
find_package(math REQUIRED)
find_package(vmmu REQUIRED)
find_package(x86 REQUIRED)

target_include_directories(
  vcpu PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:include>
  )

target_link_libraries(vcpu PUBLIC emulator hedron math logger vmmu x86)

target_compile_options(vcpu INTERFACE -Wall -Wextra)

install(
  TARGETS vcpu
  EXPORT vcpuConfig
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT vcpuConfig
  FILE vcpuConfig.cmake
  DESTINATION lib/cmake/vcpu
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

export(TARGETS vcpu FILE vcpuTargets.cmake)
