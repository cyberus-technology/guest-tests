# Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Let cmake try to build a static library to check if the compiler is working.
# The default method is to compile a complete app for the chosen architecture,
# which might fail when cross-compiling (e.g., on mac). For our purposes,
# checking if static libraries can be built is sufficient.
# Note: This has to be set _before_ calling project(), because it influences the
#       behavior.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

cmake_minimum_required(VERSION 3.21)

# The cmake documentation says: Call the project() command near the top of the
# top-level CMakeLists.txt, but after calling cmake_minimum_required(). It is
# important to establish version and policy settings before invoking other
# commands whose behavior they may affect. Do not place any settings above this
# point unless absolutely necessary and justified.
project(guest-tests CXX C ASM)

# Must be set before including CTest so that these arguments are applied when
# running `$ ninja test` or `$ make test`.
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure" "--output-junit" "junit.xml")
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED "On")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(default_build_type "Develop")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
    STATUS
      "Setting build type to '${default_build_type}' as none was specified."
    )
  set(CMAKE_BUILD_TYPE
      "${default_build_type}"
      CACHE STRING "Choose the type of build." FORCE
    )
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Develop" "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

# build type specific settings which are applied globally
set(CMAKE_CXX_FLAGS_DEVELOP "${CMAKE_CXX_FLAGS_DEVELOP} -O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE -O2")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Make function available. Doesn't change anything so far.
include(nostd-build)

add_subdirectory(lib)
add_subdirectory(tests)

if(BUILD_TESTING)
  add_subdirectory(unittests)
endif()
