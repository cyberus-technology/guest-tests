cmake_minimum_required(VERSION 3.10)

project(elf CXX)
set(CMAKE_CXX_STANDARD 17)

add_library(elf INTERFACE)

find_package(cbl REQUIRED)
find_package(math REQUIRED)

target_include_directories(
  elf INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
  )

target_compile_options(elf INTERFACE -Wall -Wextra)

target_link_libraries(elf INTERFACE cbl math)

install(
  TARGETS elf
  EXPORT elfTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT elfTargets
  FILE elfTargets.cmake
  DESTINATION lib/cmake/elf
  )

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION include)

# In order to make interface libraries discoverable via find_package
# we have to separate a *Config.cmake file and export the lib.
install(FILES elfConfig.cmake DESTINATION lib/cmake/elf)

export(TARGETS elf FILE elfTargets.cmake)

