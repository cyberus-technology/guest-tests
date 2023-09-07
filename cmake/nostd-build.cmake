add_compile_options(
  # TODO activate again
  #-Wall
  #-Werror
  #-Wextra
  -Wfatal-errors
  "$<$<C_COMPILER_ID:GNU>:-Wlogical-op>"
  "$<$<C_COMPILER_ID:GNU>:-Wno-address-of-packed-member>"
  -Wno-unused-function
  -Wshadow
  -fcheck-new
  -fdata-sections
  -ffunction-sections
  -fno-builtin
  -fno-common
  -fno-plt
  -fpie
  "$<$<C_COMPILER_ID:GNU>:-fstack-protector-strong>"
  "$<$<C_COMPILER_ID:GNU>:-mstack-protector-guard=global>"
  -fvisibility=hidden
  -m64
  -march=westmere
  -mno-3dnow
  -mno-mmx
  -mno-sse
  # We use push/pop in inline assembly, which is generally not safe with
  # red zone enabled.
  -mno-red-zone
  -nodefaultlibs
  -nostdinc
  -nostdlib
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-deprecated-copy>"
  "$<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wuseless-cast>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>"
  )

add_link_options(
  -nostdlib -Wl,-z,max-page-size=4096 -Wl,--gc-sections -Wl,--no-relax -static
  )
