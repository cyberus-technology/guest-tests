find_package(googlemock REQUIRED)
add_executable(elf_unit_test main.cpp)
add_test(NAME elf_unit_test_bin COMMAND elf_unit_test)

target_link_libraries(elf_unit_test elf gmock_main)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(
    elf_unit_test PRIVATE -fsanitize=address -fsanitize=undefined
    )
  target_link_options(
    elf_unit_test PRIVATE -fsanitize=address -fsanitize=undefined
    )
endif()
