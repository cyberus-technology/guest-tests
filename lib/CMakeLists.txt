# CMake mono-repository layout
#
# This file contains the logic to build the "standalone" directory in a single
# build when not using a package manager. Every subproject should be a
# standalone cmake project and could theoretically be in an own repository.
# This file allows to keep the "single cmake invocation build" workflow while
# also allowing a package manager based workflow, where library dependencies
# are provided by the package manager. This is achieved by adapting the
# findPackage functionality usually used to retrieve external dependencies.
#
# With a package manager, the user is able to provide the dependencies via that
# package manager. The user does not use this cmake file
# (standalone/CMakeLists.txt) and builds all the subprojects separately while
# the package manager takes care of providing the correct dependencies which
# are then found via findPackage in the subprojects.
#
# Without a package manager, the user is able to build the project as a whole
# using this cmake file. Cmake already knows and remembers the local
# dependencies (as usual) and simply ignores the findPackage calls for those.

cmake_minimum_required(VERSION 3.10)

project(standalone)
set(CMAKE_CXX_STANDARD 17)

# The variable as_subproject should list all subprojects that should be fetched
# via their local path instead of being found via findPackage.
#
# Consider following project structure:
#       HelloApp
#       /      \
#     LibA    LibB
#      |
#     LibC
#
# As LibA, LibB, and LibC are used as a dependency in HelloApp and should be
# fetched via their local path the as_subproject variable should look like:
#
# set(as_subproject LibA LibB LibC)

set(as_subproject
    acpi
    baretest
    baretest-config
    cbl
    console
    console-serial
    compiler
    config
    decoder
    device-driver
    first-fit-heap
    libcxx
    libsn
    logger
    multiboot
    multiboot2
    memory
    optionparser
    pci
    printf
    pprintpp
    sotest
    string_backend
    tstickle
    x86
    toyos
    toyos-boot-portion
    udis86
    xen-pvh
    xhci
  )

# Globally define following compile options used for all sub-projects.
#
# NOTE:
# This is a workaround to avoid duplicating the definitions of the flags in
# every sub-project. Revisit this workaround when supernova-core#749 is sorted
# out.
add_compile_options(
  -Wall

  -Wextra
  -Wfatal-errors
  "$<$<C_COMPILER_ID:GNU>:-Wlogical-op>"
  "$<$<C_COMPILER_ID:GNU>:-Wno-address-of-packed-member>"
  -Wno-unused-function
  -Wshadow
  -fcheck-new
  # Disable exceptions for MUSL apps. Our custom MUSL toolchain does not allow
  # to create threads with a valid TLS yet while exception support relies on
  # TLS. See supernova-core#774.
  -fno-exceptions
  )

# The find_package mechanism is adapted to ignore find_package calls to
# local dependencies by looking up if a package is defined as a subproject.

macro(find_package)
  message(STATUS "as_subproject is ${as_subproject}")
  message(STATUS "ARG0 is ${ARGV0}, ${ARGV1}")
  if(NOT "${ARGV0}" IN_LIST as_subproject)
    message(STATUS "using native find_package for ${ARGV0}: ${ARGV}")
    _find_package(${ARGV})
  endif()
endmacro()

add_subdirectory(acpi)
add_subdirectory(baretest)
add_subdirectory(baretest-config)
add_subdirectory(cbl)
add_subdirectory(console)
add_subdirectory(console-serial)
add_subdirectory(compiler)
add_subdirectory(config)
add_subdirectory(device-driver)
add_subdirectory(decoder)
add_subdirectory(first-fit-heap)
add_subdirectory(libcxx)
add_subdirectory(libsn)
add_subdirectory(logger)
add_subdirectory(multiboot)
add_subdirectory(multiboot2)
add_subdirectory(memory)
add_subdirectory(optionparser)
add_subdirectory(pci)
add_subdirectory(printf)
add_subdirectory(pprintpp)
add_subdirectory(sotest)
add_subdirectory(string_backend)
add_subdirectory(tstickle)
add_subdirectory(x86)
add_subdirectory(toyos)
add_subdirectory(toyos-boot-portion)
add_subdirectory(udis86)
add_subdirectory(xen-pvh)
add_subdirectory(xhci)
