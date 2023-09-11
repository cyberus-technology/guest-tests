function(guesttest_linkage target-name)
  set(target-name PARENT_SCOPE)

  prep_and_use_linker_script(
    ${target-name} ${CMAKE_CURRENT_SOURCE_DIR}/kernel.lds
    )

  add_custom_command(
    TARGET ${target-name}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ARGS -SO elf32-i386 ${target-name}
            ${target-name}-elf32
    )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target-name}-elf32
    DESTINATION .
    COMPONENT "guest-integration-tests"
    )

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${target-name}
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

  add_executable(${test_name}_guesttest ${test_name}/main.cpp)

  target_link_libraries(
    ${test_name}_guesttest PRIVATE toyos ${GUESTTEST_ARGS_EXTRA_LIBS}
    )

  target_sources(${test_name}_guesttest PRIVATE ${GUESTTEST_ARGS_EXTRA_SOURCES})

  guesttest_linkage(${test_name}_guesttest)

endfunction()

add_guesttest(cpuid EXTRA_SOURCES cpuid/benchmark.cpp)
add_guesttest(cxx EXTRA_SOURCES cxx/setjmp.cpp)
add_guesttest(debugport)
add_guesttest(
  emulator
  EXTRA_SOURCES
  emulator/control_register.cpp
  emulator/controlflow.cpp
  emulator/generated.cpp
  emulator/manual.cpp
  emulator/movbe.cpp
  emulator/segment.cpp
  emulator/stack.cpp
  emulator/string_pio.cpp
  )
add_guesttest(emulator-syscall EXTRA_SOURCES emulator-syscall/compat.S)
add_guesttest(exceptions)
add_guesttest(fpu)
add_guesttest(lapic-modes EXTRA_SOURCES lapic-modes/x2apic_test_tools.cpp)
add_guesttest(lapic-priority EXTRA_SOURCES lapic-priority/benchmark.cpp)
add_guesttest(lapic-timer)
add_guesttest(msr)
add_guesttest(pagefaults)
add_guesttest(sgx)
add_guesttest(sgx-launch-control)
add_guesttest(tsc)
add_guesttest(vmx)
