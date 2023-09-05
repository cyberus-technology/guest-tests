find_package(GTest REQUIRED)

add_executable(
  hedron_unit_tests hip.cpp libhedron.cpp raii.cpp span.cpp typed_item.cpp
                    utcb.cpp
  )
target_link_libraries(hedron_unit_tests GTest::Main hedron)
add_test(NAME hedron COMMAND hedron_unit_tests)
target_compile_options(hedron_unit_tests PRIVATE -fexceptions)
