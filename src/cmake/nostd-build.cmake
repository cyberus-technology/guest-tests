# Options affecting the build output. Removing all references to the default
# standard libraries and enabling/disabling relevant CPU features, as well as
# special linker configurations.

function(add_nostd_compile_options TARGET VISIBILITY_MODIFIER)
  if(TARGET ${TARGET})
    target_compile_options(
      ${TARGET}
      ${VISIBILITY_MODIFIER}
      -fcheck-new
      -fdata-sections
      -ffunction-sections
      -fno-builtin
      -fno-common
      -fno-plt
      -fpie
      -fvisibility=hidden
      -m64
      -march=westmere # Keep in sync with README!
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
  else()
    message(WARNING "Target ${TARGET} does not exist.")
  endif()
endfunction()

function(add_nostd_link_options TARGET VISIBILITY_MODIFIER)
  if(TARGET ${TARGET})
    target_link_options(
      ${TARGET}
      ${VISIBILITY_MODIFIER}
      -nostdlib
      -Wl,-z,max-page-size=4096
      -Wl,--gc-sections
      -Wl,--no-relax
      -static
      )
  else()
    message(WARNING "Target ${TARGET} does not exist.")
  endif()
endfunction()
