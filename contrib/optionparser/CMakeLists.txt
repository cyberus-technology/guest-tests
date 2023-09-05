cmake_minimum_required(VERSION 3.10)

project(optionparser-contrib CXX)

add_library(optionparser-contrib INTERFACE)

target_include_directories(
    optionparser-contrib INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/optionparser/src>
                      $<INSTALL_INTERFACE:include>
  )

install(
  TARGETS optionparser-contrib
  EXPORT optionparser-contribTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT optionparser-contribTargets
  FILE optionparser-contribTargets.cmake
  DESTINATION lib/cmake/optionparser-contrib
  )

install(FILES optionparser/src/optionparser.h
        DESTINATION include
  )

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES optionparser-contribConfig.cmake DESTINATION lib/cmake/optionparser-contrib)

export(TARGETS optionparser-contrib FILE optionparser-contribTargets.cmake)
