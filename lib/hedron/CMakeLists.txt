cmake_minimum_required(VERSION 3.10)

project(hedron CXX ASM)

add_library(hedron STATIC cap_pool.cpp delay.cpp dump.cpp ec.cpp hip.cpp sm.cpp)

find_package(acpi REQUIRED)
find_package(cbl REQUIRED)
find_package(compiler REQUIRED)
find_package(config REQUIRED)
find_package(logger REQUIRED)
find_package(libcxx REQUIRED)
find_package(math REQUIRED)
find_package(memory REQUIRED)
find_package(pprintpp REQUIRED)
find_package(x86 REQUIRED)

target_compile_features(hedron PUBLIC cxx_std_17)

target_include_directories(
  hedron PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_compile_options(hedron INTERFACE -Wall -Wextra)

target_link_libraries(
  hedron
  PRIVATE
          config
          compiler
          logger
          math
          memory
          pprintpp
  )

target_link_libraries(
  hedron
  PUBLIC
        acpi
        cbl
        libcxx
        x86
  )

install(
  TARGETS hedron
  EXPORT hedronTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT hedronTargets
  FILE hedronTargets.cmake
  DESTINATION lib/cmake/hedron
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/hedron
        DESTINATION include/hedron
  )

export(TARGETS hedron FILE hedronTargets.cmake)
