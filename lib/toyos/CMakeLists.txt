cmake_minimum_required(VERSION 3.10)

project(toyos CXX C ASM)

add_library(
  toyos STATIC
  src/boot.cpp
  src/console_serial.cpp
  src/init.S
  src/mm.cpp
  src/pd.cpp
  src/pdpt.cpp
  src/pml4.cpp
  src/pt.cpp
  src/stackprotector.c
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

target_include_directories(
  toyos PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
               $<INSTALL_INTERFACE:include>
  )

target_compile_options(toyos PRIVATE -Wall -Wextra)

target_compile_options(
  toyos PUBLIC -Wall -Wextra -Wno-ignored-qualifiers # silence pprintpp code
  )

# for first-fit-heap
target_compile_definitions(toyos PUBLIC HEAP_FREESTANDING HEAP_ASSERT)

target_link_libraries(toyos PUBLIC udis86-contrib)

install(
  TARGETS toyos
  EXPORT toyosTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(DIRECTORY include/ DESTINATION include/)

install(
  EXPORT toyosTargets
  FILE toyosTargets.cmake
  DESTINATION lib/cmake/toyos
  )

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES toyosConfig.cmake DESTINATION lib/cmake/toyos)

export(TARGETS toyos FILE toyosTargets.cmake)
