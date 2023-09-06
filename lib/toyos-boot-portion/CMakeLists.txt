cmake_minimum_required(VERSION 3.10)

project(toyos-boot-portion CXX ASM)

add_library(toyos-boot-portion STATIC src/boot.cpp src/init.S src/stackprotector.c)

find_package(console-serial REQUIRED)
find_package(config REQUIRED)
find_package(printf REQUIRED)
find_package(xhci REQUIRED)
find_package(multiboot REQUIRED)
find_package(multiboot2 REQUIRED)
find_package(optionparser REQUIRED)
find_package(first-fit-heap REQUIRED)
find_package(pci REQUIRED)
find_package(toyos REQUIRED)

target_include_directories(
  toyos-boot-portion PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                             $<INSTALL_INTERFACE:include>
  )

target_compile_options(toyos-boot-portion PRIVATE -Wall -Wextra)

target_link_libraries(toyos-boot-portion PUBLIC console-serial config printf xhci first-fit-heap multiboot multiboot2 optionparser pci toyos tstickle)

install(
  TARGETS toyos-boot-portion
  EXPORT toyos-boot-portionTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(FILES include/toyos/elfnote.hpp include/toyos/xhci_console.hpp DESTINATION include)

install(
  EXPORT toyos-boot-portionTargets
  FILE toyos-boot-portionTargets.cmake
  DESTINATION lib/cmake/toyos-boot-portion
  )

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES toyos-boot-portionConfig.cmake
        DESTINATION lib/cmake/toyos-boot-portion
  )

export(TARGETS toyos-boot-portion FILE toyos-boot-portionTargets.cmake)
