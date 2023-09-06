cmake_minimum_required(VERSION 3.10)

project(cbl CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(cbl STATIC
        src/baretest_config.cpp
        src/baretest/baretest.cpp
        src/baretest/print.cpp
        src/printf/backend.cpp
        src/printf/xprintf.c
)

find_package(libcxx REQUIRED)
find_package(memory REQUIRED)

target_include_directories(
  cbl PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_compile_options(cbl PUBLIC -Wall -Wextra
        -Wno-ignored-qualifiers # silence pprintpp code
)

target_link_libraries(cbl PUBLIC memory libcxx)

install(
  TARGETS cbl
  EXPORT cblTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT cblTargets
  FILE cblTargets.cmake
  DESTINATION lib/cmake/cbl
  )

install(DIRECTORY include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES cblConfig.cmake DESTINATION lib/cmake/cbl)

export(TARGETS cbl FILE cblTargets.cmake)
