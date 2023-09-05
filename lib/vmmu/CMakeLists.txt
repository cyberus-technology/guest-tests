cmake_minimum_required(VERSION 3.10)

project(vmmu CXX)

find_package(vmmu-contrib REQUIRED)

# Unfortunately, we cannot make this an INTERFACE library. The vmmu lib depends
# on the vmmu-contrib STATIC library. Because of this dependency, cmake
# promotes the vmmu library to a STATIC library too, but isn't able to link the
# vmmu library in the end. See following error:
# x86_64-unknown-linux-musl-ld: cannot find -lvmmu
#
# We can work around the problem by declaring the vmmu lib STATIC.
add_library(vmmu STATIC empty.cpp)

target_include_directories(
  vmmu PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
              $<INSTALL_INTERFACE:include>
  )

target_compile_features(vmmu PUBLIC cxx_std_17)

target_link_libraries(vmmu PUBLIC vmmu-contrib)

install(
  TARGETS vmmu
  EXPORT vmmuTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION lib
  INCLUDES
  DESTINATION include
  )

install(
  EXPORT vmmuTargets
  FILE vmmuTargets.cmake
  DESTINATION lib/cmake/vmmu
  )

install(FILES include/vmmu_virtual_mmu.hpp include/virtual_mmu_interface.hpp
        DESTINATION include
  )

# In order to make libraries discoverable via find_package we have to
# separate a *Config.cmake file and export the lib.
install(FILES vmmuConfig.cmake DESTINATION lib/cmake/vmmu)

export(TARGETS vmmu FILE vmmuTargets.cmake)
