# Builds the full toyos library for the nostd-environment of the guest tests.

add_library(
  toyos STATIC
  src/boot.cpp
  src/console_serial.cpp
  src/console_serial_util.cpp
  src/init.S
  src/mm.cpp
  src/pd.cpp
  src/pdpt.cpp
  src/pml4.cpp
  src/pt.cpp
  src/string_util.cpp
  src/baretest/baretest.cpp
  src/baretest/baretest_config.cpp
  src/baretest/print.cpp
  src/printf/backend.cpp
  src/printf/xprintf.c
  src/testhelper/entry.S
  src/testhelper/irq_handler.cpp
  src/testhelper/lapic_test_tools.cpp
  src/xhci/console_base.cpp
  src/xhci/debug_device.cpp
  src/xhci/debug_device_transfers.cpp
  src/xhci/device.cpp
  src/x86/segmentation.cpp
  )

add_nostd_compile_options(toyos PRIVATE)
add_nostd_link_options(toyos PRIVATE)

target_include_directories(
  toyos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
               $<INSTALL_INTERFACE:include>
  )

target_compile_options(toyos PRIVATE -Wall -Wextra)

target_compile_options(
  toyos PUBLIC -Wall -Wextra -Wno-ignored-qualifiers # silence pprintpp code
  )

# for first-fit-heap
target_compile_definitions(toyos PUBLIC HEAP_FREESTANDING HEAP_ASSERT)

target_link_libraries(toyos PUBLIC libcxx)
