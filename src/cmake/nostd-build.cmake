# Options affecting the compiler output. Removing all references to the default
# standard libraries and enabling/disabling relevant CPU features.

add_compile_options(
  -fcheck-new
  -fdata-sections
  -ffunction-sections
  -fno-builtin
  -fno-common
  -fno-plt
  -fpie
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
  "$<$<C_COMPILER_ID:GNU>:-fno-stack-protector>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>"
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"
  )

add_link_options(
  -nostdlib -Wl,-z,max-page-size=4096 -Wl,--gc-sections -Wl,--no-relax -static
  )
