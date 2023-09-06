cmake_minimum_required(VERSION 3.10)

project(optionparser CXX)

find_package(cbl REQUIRED)
find_package(logger REQUIRED)

add_library(optionparser INTERFACE)

target_include_directories(
  optionparser INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include>
  )

target_compile_options(optionparser INTERFACE -Wall -Wextra)

target_link_libraries(optionparser INTERFACE cbl logger optionparser-contrib)

install(
  TARGETS optionparser
  EXPORT optionparserTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT optionparserTargets
  FILE optionparserTargets.cmake
  DESTINATION lib/cmake/optionparser
  )

install(FILES include/optionparser/optionparser.hpp DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES optionparserConfig.cmake DESTINATION lib/cmake/optionparser)

export(TARGETS optionparser FILE optionparserTargets.cmake)
