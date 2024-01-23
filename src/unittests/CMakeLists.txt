find_package(Catch2 3 REQUIRED)

add_executable(
  toyos-unittests_combined toyos/cmdline.cpp toyos/cpuid_util.cpp
                           toyos/console_serial_util.cpp toyos/string_util.cpp
  )

target_link_libraries(
  toyos-unittests_combined PRIVATE toyos-host Catch2::Catch2WithMain
  )

# We consciously don't use catch_discover_tests to auto-discover
# tests, because this results in no tests being run and success being
# reported, when the test binary is not able to list its tests.
add_test(NAME toyos-unittests COMMAND toyos-unittests_combined)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Yes, this is needed twice: compile and link options
  target_compile_options(
    toyos-unittests_combined PRIVATE -fsanitize=address -fsanitize=undefined
    )
  target_link_options(
    toyos-unittests_combined PRIVATE -fsanitize=address -fsanitize=undefined
    )

  # At least some GCC versions get their linking wrong with ASAN and produce the
  # following linking warning:
  #
  # ASan runtime does not come first in initial library list; you
  # should either link runtime to your application or manually preload
  # it with LD_PRELOAD.
  #
  # Workaround by linking sanitizers statically.
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_options(
      toyos-unittests_combined PRIVATE -static-libasan -static-libubsan
      )
  endif()
endif()
