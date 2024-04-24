add_library(
  libcxx STATIC
  src/c_string.cpp
  src/exception.cpp
  src/locale.cpp
  src/memory.cpp
  src/setjmp.S
  src/stdexcept.cpp
  src/string.cpp
  src/vector.cpp
  )

add_nostd_compile_options(libcxx PRIVATE)
add_nostd_link_options(libcxx PRIVATE)

target_include_directories(
  libcxx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_link_libraries(libcxx PUBLIC gtbase)

# Not our code, so we should also not be annoyed by its compiler warnings.
target_compile_options(libcxx PRIVATE -Wno-all -Wno-error -Wno-useless-cast)
target_link_libraries(libcxx PUBLIC libc-tiny)
