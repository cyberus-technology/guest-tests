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

target_include_directories(
  libcxx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_compile_options(libcxx PRIVATE -Wall -Wextra)

target_link_libraries(libcxx PUBLIC libc-tiny)
