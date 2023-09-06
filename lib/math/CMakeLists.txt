cmake_minimum_required(VERSION 3.10)

project(math CXX)

add_library(math INTERFACE)

target_include_directories(
  math INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:include>
  )

target_compile_options(math INTERFACE -Wall -Wextra)

install(
  TARGETS math
  EXPORT mathTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT mathTargets
  FILE mathTargets.cmake
  DESTINATION lib/cmake/math
  )

install(FILES include/math/math.hpp DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES mathConfig.cmake DESTINATION lib/cmake/math)

export(TARGETS math FILE mathTargets.cmake)
