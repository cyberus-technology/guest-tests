cmake_minimum_required(VERSION 3.10)

project(pprintpp CXX)

add_library(pprintpp INTERFACE)

target_include_directories(
  pprintpp INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include>
  )

target_compile_options(
  pprintpp INTERFACE -Wno-format-security -Wno-ignored-qualifiers -Wall -Wextra

  )

install(
  TARGETS pprintpp
  EXPORT pprintppTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT pprintppTargets
  FILE pprintppTargets.cmake
  DESTINATION lib/cmake/pprintpp
  )

install(FILES include/charlist.hpp
        include/pprintpp.hpp
        include/stl_symbols.hpp
        include/typelist.hpp
        DESTINATION include
  )

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES pprintppConfig.cmake DESTINATION lib/cmake/pprintpp)

export(TARGETS pprintpp FILE pprintppTargets.cmake)
