find_package(GTest REQUIRED)

add_executable(slvm_api_unit_tests main.cpp)
target_link_libraries(slvm_api_unit_tests GTest::Main slvm-api)
add_test(NAME slvm-api COMMAND slvm_api_unit_tests)
target_compile_options(slvm_api_unit_tests PRIVATE -fexceptions)
