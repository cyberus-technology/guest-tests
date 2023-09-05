set(depgraph_directory "${CMAKE_BINARY_DIR}/depgraph")
set(depgraph_file "${depgraph_directory}/depgraph.pdf")

# The graphviz feature of CMake creates a lot of files. Put these
# files into an extra directory to avoid clutter in the top-level
# build directory.
file(MAKE_DIRECTORY "${depgraph_directory}")

add_custom_command(
  OUTPUT "${depgraph_file}"
  COMMAND ${CMAKE_COMMAND} ${CMAKE_BINARY_DIR}
          "--graphviz=${depgraph_directory}/depgraph.dot" .
  COMMAND dot -Tpdf ${depgraph_directory}/depgraph.dot -o "${depgraph_file}"
  WORKING_DIRECTORY "${depgraph_dir}"
  )

# We always allow manually building dependency graphs, even if the
# build option is enabled.
add_custom_target(depgraph DEPENDS ${depgraph_file})

# If the build option is enabled, we just automatically build it.
if(BUILD_DEPGRAPH)
  add_custom_target(depgraph-auto ALL DEPENDS ${depgraph_file})

  # If you change this directory, please update nix/build.nix as well.
  install(FILES "${depgraph_file}" DESTINATION share/uefi/doc)
endif()
