# This top-level CMakeLists.txt helps the CLion IDE and other tooling to
# properly index the project. It is not part of the Nix closure.

cmake_minimum_required(VERSION 3.20)
project(cyberus-guest-tests CXX C ASM)

# Must be set before including CTest so that these arguments are applied when
# running `$ ninja test` or `$ make test`.
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")
# This must be in the top-level CMakeLists.txt file so that tests are
# discovered. It is the top-level file, if developers use
# `$ cmake ..` instead of `$ cmake ../src` in their `<root>/build` directory.
include(CTest)

add_subdirectory(src)
