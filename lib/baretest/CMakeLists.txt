cmake_minimum_required(VERSION 3.10)

project(baretest CXX)

add_library(baretest INTERFACE)

target_include_directories(
  baretest INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include>
  )

target_compile_definitions(
        baretest INTERFACE HEAP_FREESTANDING HEAP_ASSERT
)

target_link_libraries(baretest INTERFACE baretest-contrib libcxx)

install(
  TARGETS baretest
  EXPORT baretestTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT baretestTargets
  FILE baretestTargets.cmake
  DESTINATION lib/cmake/baretest
  )

install(FILES include/heap_freestanding.hpp DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES baretestConfig.cmake DESTINATION lib/cmake/baretest)

export(TARGETS baretest FILE baretestTargets.cmake)
