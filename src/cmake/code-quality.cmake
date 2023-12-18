# Code quality flags and options.

add_compile_options(
  -Wall
  -Werror
  -Wextra
  -Wfatal-errors
  -Wno-unused-function
  -Wshadow
  "$<$<C_COMPILER_ID:GNU>:-Wlogical-op>"
  "$<$<C_COMPILER_ID:GNU>:-Wno-address-of-packed-member>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-deprecated-copy>"
  "$<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wuseless-cast>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>"
  )
