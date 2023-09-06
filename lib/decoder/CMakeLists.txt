cmake_minimum_required(VERSION 3.10)

project(decoder CXX)

add_library(decoder STATIC gdt.cpp operand.cpp udis_decoder.cpp)

find_package(cbl REQUIRED)
find_package(first-fit-heap REQUIRED)
find_package(logger REQUIRED)
find_package(udis86 REQUIRED)
find_package(x86 REQUIRED)

target_compile_features(decoder PUBLIC cxx_std_17)

target_include_directories(
  decoder PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:include>
  )

target_compile_options(decoder PRIVATE -Wall -Wextra)

target_link_libraries(decoder PUBLIC cbl first-fit-heap logger udis86-contrib x86)

install(
  TARGETS decoder
  EXPORT decoderTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT decoderTargets
  FILE decoderTargets.cmake
  DESTINATION lib/cmake/decoder
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include/)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/decoder
        DESTINATION include/decoder
  )

# In order to make libraries discoverable via find_package we have to separate
# a *Config.cmake file and export the lib.
install(FILES decoderConfig.cmake DESTINATION lib/cmake/decoder)

export(TARGETS decoder FILE decoderTargets.cmake)
