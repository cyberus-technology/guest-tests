# Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
#
# SPDX-License-Identifier: GPL-2.0-or-later

function(prep_and_use_linker_script target-name source-file)
  set(resulting-ld-script ${CMAKE_CURRENT_BINARY_DIR}/${target-name}.ld)

  # linker script includes <config.h>
  set(ld-includes -I ${LIB_DIR}/gtbase/include)

  # In order to let the build system reason about the dependencies (header
  # files) of preprocessed linker scripts, cmake needs to determine those
  # dependencies so we don't have to specify them manually.
  #
  # Unfortunately, there is a catch:
  # - IMPLICIT_DEPENDS works for Makefiles and is ignored by all others
  # - DEPFILE works for Ninja, but causes an error with Makefiles (sigh)
  #
  # The only reasonable solution right now is to duplicate this custom command
  # depending on the generator.
  if(CMAKE_GENERATOR STREQUAL "Ninja")
    add_custom_command(
      OUTPUT ${resulting-ld-script}
      MAIN_DEPENDENCY ${source-file}
      DEPFILE ${resulting-ld-script}.d
      COMMAND ${CMAKE_CXX_COMPILER} ARGS -x c -E ${source-file} -P -o
              ${resulting-ld-script} ${ld-includes}
      )
  endif()
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    add_custom_command(
      OUTPUT ${resulting-ld-script}
      MAIN_DEPENDENCY ${source-file}
      IMPLICIT_DEPENDS CXX ${source-file}
      COMMAND ${CMAKE_CXX_COMPILER} ARGS -x c -E ${source-file} -P -o
              ${resulting-ld-script} ${ld-includes}
      )
  endif()

  add_custom_target(
    ${target-name}-linker-script-generated DEPENDS ${resulting-ld-script}
    )
  add_dependencies(${target-name} ${target-name}-linker-script-generated)
  set_target_properties(
    ${target-name} PROPERTIES LINK_DEPENDS ${resulting-ld-script}
    )
  target_link_options(${target-name} PRIVATE -Wl,-T ${resulting-ld-script})
endfunction()
