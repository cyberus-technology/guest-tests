# Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later

include(code-quality)
include(prep_and_use_linker_script)

function(guesttest_linkage target-name)
  set(target-name PARENT_SCOPE)

  prep_and_use_linker_script(
    ${target-name} ${CMAKE_CURRENT_SOURCE_DIR}/kernel.lds
    )

  add_custom_command(
    TARGET ${target-name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ARGS -O elf32-i386 ${target-name}.elf64
            ${target-name}.elf32
    )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target-name}.elf32
    DESTINATION .
    COMPONENT "guest-integration-tests"
    )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target-name}.elf64
    DESTINATION .
    COMPONENT "guest-integration-tests"
    )
endfunction()

function(add_guesttest test_name)

  cmake_parse_arguments(
    GUESTTEST_ARGS
    "" # optional arguments
    "" # single-value args
    "EXTRA_SOURCES;EXTRA_LIBS" # multi-value args
    ${ARGN}
    )

  add_executable(${test_name} ${test_name}/main.cpp)

  add_nostd_compile_options(${test_name} PRIVATE)
  add_nostd_link_options(${test_name} PRIVATE)

  set_target_properties(${test_name} PROPERTIES SUFFIX ".elf64")

  target_link_libraries(${test_name} PRIVATE toyos ${GUESTTEST_ARGS_EXTRA_LIBS})

  target_sources(${test_name} PRIVATE ${GUESTTEST_ARGS_EXTRA_SOURCES})

  guesttest_linkage(${test_name})

endfunction()

add_guesttest(cpuid EXTRA_SOURCES cpuid/benchmark.cpp)
add_guesttest(emulator-syscall)
add_guesttest(exceptions)
add_guesttest(fpu)
add_guesttest(hello-world EXTRA_SOURCES hello-world/setjmp.cpp)
add_guesttest(lapic-modes EXTRA_SOURCES lapic-modes/x2apic_test_tools.cpp)
add_guesttest(lapic-priority EXTRA_SOURCES lapic-priority/benchmark.cpp)
add_guesttest(lapic-timer)
add_guesttest(msr)
add_guesttest(pagefaults)
add_guesttest(pit-timer)
add_guesttest(sgx)
add_guesttest(sgx-launch-control)
add_guesttest(tinivisor)
add_guesttest(tsc)
add_guesttest(vmx)
add_guesttest(timing)
